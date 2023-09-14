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

#define LOG_TAG "CameraSolutionProxy"
#include "camera_solution_proxy.h"
#include "camera_constants.h"
#include "camera_log.h"
#include "generate_unique_id.h"
#include "json_utils.h"
#include "luna_client.h"
#include "process.h"
#include <nlohmann/json.hpp>
#include <system_error>

using json = nlohmann::json;

const std::string CameraSolutionProcessName      = "com.webos.service.camera2.solution";
const std::string CameraSolutionConnectionBaseId = "com.webos.camerasolution.";

#define COMMAND_TIMEOUT 8000 // ms

static bool cameraSolutionServiceCb(const char *msg, void *data)
{
    PLOGI("%s", msg);

    json j = json::parse(msg, nullptr, false);
    if (j.is_discarded())
    {
        PLOGE("msg parsing error!");
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
    PLOGI("%s", solution_name_.c_str());
}

CameraSolutionProxy::~CameraSolutionProxy()
{
    PLOGI("");

    stopThread();

    // If release() has not been called before
    if (process_)
    {
        try
        {
            release();
        }
        catch (const std::exception &e)
        {
            PLOGE("Error: %s", e.what());
        }
    }
}

int32_t CameraSolutionProxy::getMetaSizeHint(void)
{
    // 10 * 1 : {"faces":[
    // 56 * n : {"w":xxx,"confidence":xxx,"y":xxxx,"h":xxxx,"x":xxxx},
    // 02 * 1 : ]}
    // n <-- 100
    // size = 10 + 56*100 + 2 = 572
    // size + padding -> 1024
    int metaSizeHint = 1024;
    // TODO : The size of the metaSizeHint should be returned by the solution.

    // In case of OSE, remove the comment below.
    // metaSizeHint = 0;

    PLOGI("metaSizeHint = %d", metaSizeHint);
    return metaSizeHint;
}

void CameraSolutionProxy::initialize(stream_format_t streamFormat, int shmKey, LSHandle *sh)
{
    PLOGI("shmKey : %d", shmKey);

    // keep informations
    streamFormat_ = streamFormat;
    shmKey_       = shmKey;
    sh_           = sh;

    startThread();
}

void CameraSolutionProxy::setEnableValue(bool enableValue)
{
    PLOGI("enableValue %d", enableValue);
    pushJob(enableValue);
}

void CameraSolutionProxy::processing(bool enableValue)
{
    if (enableStatus_ == enableValue)
    {
        PLOGI("same as current value %d", enableValue);
        return;
    }

    if (shmKey_ == 0)
    {
        PLOGI("shared memory key is not ready");
        return;
    }

    PLOGI("start : enableValue = %d", enableValue);

    enableStatus_ = enableValue;
    json jin;
    jin[CONST_PARAM_NAME_ENABLE] = enableStatus_;

    if (enableValue)
    {
        startProcess();

        create();
        init();

        luna_call_sync("enable", to_string(jin));

        subscribe();
    }
    else
    {
        luna_call_sync("enable", to_string(jin));

        // Call this after enable false to get postProcessing callback
        unsubscribe();

        luna_call_sync("release", "{}");

        stopProcess();
    }

    PLOGI("end :  enableStatus_ = %d", enableStatus_);
}

void CameraSolutionProxy::release()
{
    PLOGI("");

    stopThread();
    processing(false);
    unsubscribe();
}

bool CameraSolutionProxy::startProcess()
{
    PLOGI("");

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
        PLOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
    }

    while (!g_main_loop_is_running(loop_))
    {
    }

    pthread_setname_np(loopThread_->native_handle(), "solproxy_luna");

    std::string service_name = cstr_uricamearhal + guid;
    luna_client              = std::make_unique<LunaClient>(service_name.c_str(), c);
    g_main_context_unref(c);

    return true;
}

bool CameraSolutionProxy::stopProcess()
{
    PLOGI("");

    g_main_loop_quit(loop_);
    if (loopThread_->joinable())
    {
        try
        {
            loopThread_->join();
        }
        catch (const std::system_error &e)
        {
            PLOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
        }
    }
    g_main_loop_unref(loop_);

    process_.reset();

    return true;
}

bool CameraSolutionProxy::create()
{
    PLOGI("");

    // Send message
    json jin;
    jin[CONST_PARAM_NAME_NAME] = solution_name_;

    return luna_call_sync(__func__, to_string(jin));
}

bool CameraSolutionProxy::init()
{
    PLOGI("");

    // Send message
    json jin;
    jin[CONST_PARAM_NAME_FORMAT]     = streamFormat_.pixel_format;
    jin[CONST_PARAM_NAME_WIDTH]      = streamFormat_.stream_width;
    jin[CONST_PARAM_NAME_HEIGHT]     = streamFormat_.stream_height;
    jin[CONST_PARAM_NAME_FPS]        = streamFormat_.stream_fps;
    jin[CONST_PARAM_NAME_BUFFERSIZE] = streamFormat_.buffer_size;
    jin[CONST_PARAM_NAME_SHMKEY]     = shmKey_;

    return luna_call_sync(__func__, to_string(jin));
}

bool CameraSolutionProxy::subscribe()
{
    PLOGI("");

    if (!LSRegisterServerStatusEx(
            sh_, uid_.c_str(),
            [](LSHandle *handle, const char *svc_name, bool connected, void *ctx) -> bool
            {
                PLOGI("[ServerStatus cb] connected=%d, name=%s\n", connected, svc_name);

                CameraSolutionProxy *self = static_cast<CameraSolutionProxy *>(ctx);
                if (connected)
                {
                    std::string uri = self->service_uri_ + "subscribe";
                    bool ret = self->luna_client->subscribe(uri.c_str(), "{\"subscribe\":true}",
                                                            &(self->subscribeKey_),
                                                            cameraSolutionServiceCb, self);
                    PLOGI("[ServerStatus cb] subscribeKey_ %ld, %d ", self->subscribeKey_, ret);
                }
                else
                {
                    PLOGI("[ServerStatus cb] cancel server status");
                    if (self != nullptr && self->cookie != nullptr)
                    {
                        self->unsubscribe();
                    }
                }
                return true;
            },
            static_cast<void *>(this), &cookie, nullptr))
    {
        PLOGE("[ServerStatus cb] LSRegisterServerStatusEx FAILED");
    }

    return true;
}

bool CameraSolutionProxy::unsubscribe()
{
    PLOGI("");

    bool ret = true;
    if (subscribeKey_)
    {
        PLOGI("remove subscribeKey_ %ld", subscribeKey_);
        ret           = luna_client->unsubscribe(subscribeKey_);
        subscribeKey_ = 0;
    }

    if (sh_ != nullptr && cookie != nullptr)
    {
        PLOGI("LSCancelServerStatus");
        if (!LSCancelServerStatus(sh_, cookie, nullptr))
        {
            PLOGE("error LSCancelServerStatus");
        }
        cookie = nullptr;
    }

    return ret;
}

bool CameraSolutionProxy::luna_call_sync(const char *func, const std::string &payload)
{
    if (process_ == nullptr)
    {
        PLOGE("solution process is not ready");
        return false;
    }

    if (func == nullptr)
    {
        PLOGE("no method name");
        return false;
    }

    // send message
    std::string uri = service_uri_ + func;
    PLOGI("%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    int64_t startClk = g_get_monotonic_time();
    luna_client->callSync(uri.c_str(), payload.c_str(), &resp, COMMAND_TIMEOUT);
    int64_t endClk = g_get_monotonic_time();

    (startClk > endClk) ? PLOGE("diffClk is error")
                        : PLOGI("response %s, runtime %lld", resp.c_str(),
                                (long long int)((endClk - startClk) / 1000));

    json j = json::parse(resp);
    if (j.is_discarded())
    {
        PLOGE("resp parsing error!");
        return false;
    }
    bool ret = get_optional<bool>(j, CONST_PARAM_NAME_RETURNVALUE).value_or(false);
    return ret;
}

void CameraSolutionProxy::run()
{
    PLOGI("thread start");

    pthread_setname_np(pthread_self(), "solproxy");
    bThreadStarted_ = true;

    while (checkAlive())
    {
        PLOGI("wait for job");
        bool resWait = wait();
        if (resWait == false)
        {
            setAlive(false);
            break;
        }
        if (checkAlive())
        {
            processing(queueJob_.front());
            popJob();
        }
    }

    PLOGI("thread end");
}

void CameraSolutionProxy::startThread()
{
    if (threadJob_ == nullptr)
    {
        PLOGI("Thread Start");
        try
        {
            bThreadStarted_ = false;
            setAlive(true);
            threadJob_ = std::make_unique<std::thread>([&](void) { run(); });

            int timeout = 1000; // 1ms * 1000
            while (!bThreadStarted_ && timeout--)
            {
                g_usleep(1000);
            }
            PLOGI("Thread Started - %d", timeout);
        }
        catch (const std::system_error &e)
        {
            PLOGE("Caught a system error with code %d meaning %s", e.code().value(), e.what());
        }
    }
}

void CameraSolutionProxy::stopThread()
{
    if (threadJob_ != nullptr && threadJob_->joinable())
    {
        PLOGI("Thread Closing");
        try
        {
            setAlive(false);
            notify();
            threadJob_->join();
            unsubscribe();
        }
        catch (const std::system_error &e)
        {
            PLOGE("Caught a system error with code %d meaning %s", e.code().value(), e.what());
        }
        threadJob_.reset();
        PLOGI("Thread Closed. queue job size %zd", queueJob_.size());
    }
}

void CameraSolutionProxy::notify(void) { cv_.notify_all(); }

bool CameraSolutionProxy::wait()
{
    bool res = true;
    try
    {
        std::unique_lock<std::mutex> lock(m_);
        cv_.wait(lock);
    }
    catch (std::system_error &e)
    {
        PLOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
        res = false;
    }

    return res;
}

void CameraSolutionProxy::pushJob(int inValue)
{
    if (queueJob_.empty())
    {
        std::lock_guard<std::mutex> lg(mtxJob_);
        queueJob_.push(inValue);
        notify();
    }
}

void CameraSolutionProxy::popJob()
{
    std::lock_guard<std::mutex> lg(mtxJob_);
    if (!queueJob_.empty())
    {
        queueJob_.pop();
    }
}
