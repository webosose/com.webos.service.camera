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

#include "camera_solution_proxy.h"
#include "camera_constants.h"
#include "camera_log.h"
#include "generate_unique_id.h"
#include "json_utils.h"
#include "luna_client.h"
#include "process.h"
#include <nlohmann/json.hpp>
#include <system_error>

using namespace nlohmann;

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
        (client->pEvent_.load())
            ->onDone(jvalue_stringify(jsonFaceInfo)); // Send face info to DeviceController
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
        PMLOG_INFO(CONST_MODULE_CSP, "same as current value %d", enableValue);
        return;
    }

    if (shmKey_ == 0)
    {
        PMLOG_INFO(CONST_MODULE_CSP, "shared memory key is not ready");
        return;
    }

    PMLOG_INFO(CONST_MODULE_CSP, "start : enableValue = %d", enableValue);

    enableStatus_ = enableValue;
    json jin;
    jin[CONST_PARAM_NAME_ENABLE] = enableStatus_;

    if (enableValue)
    {
        startProcess();

        createSolution();
        initSolution();

        luna_call_sync(__func__, to_string(jin));

        subscribe();
    }
    else
    {
        luna_call_sync(__func__, to_string(jin));

        // Call this after setEnableValue false to get postProcessing callback
        unsubscribe();

        luna_call_sync("release", "{}");

        stopProcess();
    }

    PMLOG_INFO(CONST_MODULE_CSP, "end :  enableStatus_ = %d", enableStatus_);
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
        PMLOG_ERROR(CONST_MODULE_CSP, "Caught a system_error with code %d meaning %s",
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
            PMLOG_ERROR(CONST_MODULE_CSP, "Caught a system_error with code %d meaning %s",
                        e.code().value(), e.what());
        }
    }
    g_main_loop_unref(loop_);

    process_.reset();

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
                    std::string uri = self->service_uri_ + "subscribe";
                    bool ret = self->luna_client->subscribe(uri.c_str(), "{\"subscribe\":true}",
                                                            &(self->subscribeKey_),
                                                            cameraSolutionServiceCb, self);
                    PMLOG_INFO(CONST_MODULE_CSP, "[ServerStatus cb] subscribeKey_ %ld, %d ",
                               self->subscribeKey_, ret);
                }
                else
                {
                    PMLOG_INFO(CONST_MODULE_CSP, "[ServerStatus cb] cancel server status");
                    if (!LSCancelServerStatus(handle, self->cookie, nullptr))
                    {
                        PMLOG_ERROR(CONST_MODULE_CSP,
                                    "[ServerStatus cb] error LSCancelServerStatus\n");
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

bool CameraSolutionProxy::luna_call_sync(const char *func, const std::string &payload)
{
    PMLOG_INFO(CONST_MODULE_CSP, "");

    if (process_ == nullptr)
    {
        PMLOG_INFO(CONST_MODULE_CSP, "solution process is not ready");
        return false;
    }

    if (func == nullptr)
    {
        PMLOG_INFO(CONST_MODULE_CSP, "no method name");
        return false;
    }

    // send message
    std::string uri = service_uri_ + func;
    PMLOG_INFO(CONST_MODULE_CSP, "%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), payload.c_str(), &resp, COMMAND_TIMEOUT);
    PMLOG_INFO(CONST_MODULE_CSP, "resp : %s", resp.c_str());

    json j = json::parse(resp, nullptr, false);
    if (j.is_discarded())
    {
        PMLOG_ERROR(CONST_MODULE_CSP, "resp parsing error!");
        return false;
    }
    bool ret = get_optional<bool>(j, CONST_PARAM_NAME_RETURNVALUE).value_or(false);

    PMLOG_INFO(CONST_MODULE_CSP, "returnValue : %d", ret);
    return ret;
}
