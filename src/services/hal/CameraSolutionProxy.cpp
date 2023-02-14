// Copyright (c) 2023 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "CameraSolutionProxy.h"
#include "GenerateUniqueID.h"
#include "LunaClient.h"
#include "Process.h"
#include "camera_constants.h"
#include "camera_log.h"
#include "camera_solution_event.h"
#include "json_utils.h"

const std::string CameraSolutionProcessName      = "com.webos.service.camera2.solution";
const std::string CameraSolutionConnectionBaseId = "com.webos.camerasolution.";
const char *const CONST_MODULE_CSP               = "CameraSolutionProxy";

#define COMMAND_TIMEOUT 2000 // ms

static bool cameraSolutionServiceCb(const char *msg, void *data)
{
    PMLOG_INFO(CONST_MODULE_CSP, "%s", msg);

    json j = json::parse(msg, nullptr, false);
    if (j.is_discarded())
    {
        PMLOG_ERROR(CONST_MODULE_CSP, "msg parsing error!");
        return false;
    }

    CameraSolutionProxy *client = (CameraSolutionProxy *)data;
    if (client->pEvent_)
    {
        jvalue_ref jsonFaceInfo = jdom_create(j_cstr_to_buffer(msg), jschema_all(), NULL);
        (client->pEvent_.load())->onDone(jsonFaceInfo); // Send face info to DeviceController
        j_release(&jsonFaceInfo);
    }

    return true;
}

CameraSolutionProxy::CameraSolutionProxy(const std::string solution_name)
    : solution_name_(solution_name)
{
    PMLOG_INFO(CONST_MODULE_CSP, "%s", solution_name_.c_str());
}

CameraSolutionProxy::~CameraSolutionProxy() { PMLOG_INFO(CONST_MODULE_CSP, ""); }

int32_t CameraSolutionProxy::getMetaSizeHint(void)
{
    // 10 * 1 : {"faces":[
    // 56 * n : {"w":xxx,"confidence":xxx,"y":xxxx,"h":xxxx,"x":xxxx},
    // 02 * 1 : ]}
    // n <-- 100
    // size = 10 + 56*100 + 2 = 572
    // size + padding -> 1024
    int metaSizeHint = 1024;
    // TODO : Check CameraSolutionProxy managing getMetaSizeHint of CameraSolutions

#ifdef PLATFORM_OSE
    metaSizeHint = 0;
    // OSE does not write meta data to shm
#endif

    PMLOG_INFO(CONST_MODULE_CSP, "metaSizeHint = %d", metaSizeHint);
    return metaSizeHint;
}

void CameraSolutionProxy::initialize(stream_format_t streamFormat, int shmKey, LSHandle *sh)
{
    PMLOG_INFO(CONST_MODULE_CSP, "shmKey : %d", shmKey);

    // keep informations
    streamFormat_ = streamFormat;
    shmKey_       = shmKey;
    sh_           = sh;
}

void CameraSolutionProxy::setEnableValue(bool enableValue)
{
    if (enableStatus_ == enableValue)
    {
        PMLOG_INFO(CONST_MODULE_CSP, "same value %d", enableValue);
        return;
    }

    if (shmKey_ == 0)
    {
        PMLOG_INFO(CONST_MODULE_CSP, "shared memory key is not ready");
        return;
    }

    enableStatus_ = enableValue;
    PMLOG_INFO(CONST_MODULE_CSP, "start : enable = %d", enableStatus_);

    if (enableStatus_)
    {
        // 1. Start process
        startProcess();

        // When the process starts,
        // 2. Create solution
        // 3. Initialize solution
        // 4. Enable solution
        // 5. Subscribe to CameraSolutionService
        prepareSolution();
    }
    else
    {
        // 1. Disable solution
        json jin;
        jin[CONST_PARAM_NAME_ENABLE] = false;
        luna_call_sync(__func__, to_string(jin));

        // 2. Unsubscribe to CameraSolutionService
        unsubscribe();

        // 3. Release solution
        luna_call_sync("release", "{}");

        // 4. Stop process
        stopProcess();
    }
}

void CameraSolutionProxy::release()
{
    PMLOG_INFO(CONST_MODULE_CSP, "");

    if (enableStatus_)
    {
        setEnableValue(false);
    }
}

bool CameraSolutionProxy::startProcess()
{
    PMLOG_INFO(CONST_MODULE_CSP, "");

    // start process
    std::string guid = GenerateUniqueID()();
    uid_             = CameraSolutionConnectionBaseId + guid;
    service_uri_     = "luna://" + uid_ + "/";

    std::string cmd = "/usr/sbin/" + CameraSolutionProcessName + " -s" + uid_;
    process_        = std::make_unique<Process>(cmd);

    // Luna Client
    GMainContext *c = g_main_context_new();
    loop_           = g_main_loop_new(c, false);

    try
    {
        loopThread_ = std::make_unique<std::thread>(g_main_loop_run, loop_);
    }
    catch (const std::system_error &e)
    {
        PMLOG_INFO(CONST_MODULE_CSP, "Caught a system_error with code %d meaning %s",
                   e.code().value(), e.what());
    }

    while (!g_main_loop_is_running(loop_))
    {
    }

    std::string service_name = cstr_uricamearhal + guid;
    luna_client              = std::make_unique<LunaClient>(service_name.c_str(), c);
    g_main_context_unref(c);

    return true;
}

bool CameraSolutionProxy::stopProcess()
{
    PMLOG_INFO(CONST_MODULE_CSP, "");

    g_main_loop_quit(loop_);
    if (loopThread_->joinable())
    {
        try
        {
            loopThread_->join();
        }
        catch (const std::system_error &e)
        {
            PMLOG_INFO(CONST_MODULE_CSP, "Caught a system_error with code %d meaning %s",
                       e.code().value(), e.what());
        }
    }
    g_main_loop_unref(loop_);

    return true;
}

bool CameraSolutionProxy::prepareSolution()
{
    PMLOG_INFO(CONST_MODULE_CSP, "");

    if (!LSRegisterServerStatusEx(
            sh_, uid_.c_str(),
            [](LSHandle *handle, const char *svc_name, bool connected, void *ctx) -> bool
            {
                PMLOG_INFO(CONST_MODULE_CSP, "[ServerStatus cb] connected=%d, name=%s\n", connected,
                           svc_name);

                CameraSolutionProxy *self = static_cast<CameraSolutionProxy *>(ctx);
                if (connected)
                {
                    // When the process starts,
                    // 1. Create solution
                    self->createSolution();

                    // 2. Initialize solution
                    self->initSolution();

                    // 3. Enable solution
                    json jin;
                    jin[CONST_PARAM_NAME_ENABLE] = true;
                    self->luna_call_sync("setEnableValue", to_string(jin));

                    // 4. Subscribe to CameraSolutionService
                    self->subscribe();

                    self->serverConnected_ = true;
                    PMLOG_INFO(CONST_MODULE_CSP, "[ServerStatus cb] end");
                }
                else
                {
                    if (self->serverConnected_)
                    {
                        PMLOG_INFO(CONST_MODULE_CSP, "[ServerStatus cb] cancel server status");
                        if (!LSCancelServerStatus(handle, self->cookie, nullptr))
                        {
                            PMLOG_ERROR(CONST_MODULE_CSP,
                                        "[ServerStatus cb] error LSCancelServerStatus\n");
                        }

                        self->serverConnected_ = false;
                        self->process_.reset();
                    }
                }
                return true;
            },
            this, &cookie, nullptr))
    {
        PMLOG_ERROR(CONST_MODULE_CSP, "[ServerStatus cb] LSRegisterServerStatusEx FAILED");
    }

    return true;
}

bool CameraSolutionProxy::createSolution()
{
    PMLOG_INFO(CONST_MODULE_CSP, "");

    // Send message
    json jin;
    jin[CONST_PARAM_NAME_NAME] = solution_name_;

    return luna_call_sync(__func__, to_string(jin));
}

bool CameraSolutionProxy::initSolution()
{
    PMLOG_INFO(CONST_MODULE_CSP, "");

    // Send message
    json jin;
    jin[CONST_PARAM_NAME_FORMAT]     = streamFormat_.pixel_format;
    jin[CONST_PARAM_NAME_WIDTH]      = streamFormat_.stream_width;
    jin[CONST_PARAM_NAME_HEIGHT]     = streamFormat_.stream_height;
    jin[CONST_PARAM_NAME_FPS]        = streamFormat_.stream_fps;
    jin[CONST_PARAM_NAME_BUFFERSIZE] = streamFormat_.buffer_size;
    jin[CONST_PARAM_NAME_SHMKEY]     = shmKey_;

    return luna_call_sync("initialize", to_string(jin));
}

bool CameraSolutionProxy::subscribe()
{
    std::string uri = service_uri_ + __func__;
    bool ret        = luna_client->subscribe(uri.c_str(), "{\"subscribe\":true}", &subscribeKey_,
                                             cameraSolutionServiceCb, this);
    PMLOG_INFO(CONST_MODULE_CSP, "subscribeKey_ %ld, %d ", subscribeKey_, ret);
    return ret;
}

bool CameraSolutionProxy::unsubscribe()
{
    bool ret = true;
    if (subscribeKey_)
    {
        PMLOG_INFO(CONST_MODULE_CSP, "remove subscribeKey_ %ld", subscribeKey_);
        ret           = luna_client->unsubscribe(subscribeKey_);
        subscribeKey_ = 0;
    }
    return ret;
}

bool CameraSolutionProxy::luna_call_sync(const char *func, const std::string &payload, json *jin)
{
    PMLOG_INFO(CONST_MODULE_CSP, "");

    if (process_ == nullptr)
    {
        PMLOG_INFO(CONST_MODULE_CSP, "solution process is not ready");
        return false;
    }

    // send message
    std::string uri = service_uri_ + func;
    PMLOG_INFO(CONST_MODULE_CSP, "%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), payload.c_str(), &resp, COMMAND_TIMEOUT);
    PMLOG_INFO(CONST_MODULE_CSP, "resp : %s", resp.c_str());

    bool ret;
    if (jin)
    {
        *jin = json::parse(resp, nullptr, false);
        if ((*jin).is_discarded())
        {
            PMLOG_ERROR(CONST_MODULE_CSP, "resp parsing error!");
            return false;
        }
        ret = get_optional<bool>(*jin, "returnValue").value_or(false);
    }
    else
    {
        json j = json::parse(resp, nullptr, false);
        if (j.is_discarded())
        {
            PMLOG_ERROR(CONST_MODULE_CSP, "resp parsing error!");
            return false;
        }
        ret = get_optional<bool>(j, "returnValue").value_or(false);
    }

    PMLOG_INFO(CONST_MODULE_CSP, "returnValue : %d", ret);
    return ret;
}
