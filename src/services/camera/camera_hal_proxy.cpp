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

#define LOG_TAG "CameraHalProxy"
#include "camera_hal_proxy.h"
#include "generate_unique_id.h"
#include "json_utils.h"
#include "luna-service2/lunaservice.hpp"
#include "luna_client.h"
#include "process.h"
#include <ios>
#include <system_error>
#include "command_manager.h"

const std::string CameraHalProcessName = "com.webos.service.camera2.hal";

static bool cameraHalServiceCb(const char *msg, void *data)
{
    PLOGI("%s", msg);

    json j = json::parse(msg, nullptr, false);
    if (j.is_discarded())
    {
        PLOGE("msg parsing error!");
        return false;
    }

    CameraHalProxy *client = (CameraHalProxy *)data;

    LSError lserror;
    LSErrorInit(&lserror);

    std::string event_key;
    std::string event_type = get_optional<std::string>(j, CONST_PARAM_NAME_EVENT).value_or("");

    if (event_type == getEventNotificationString(EventType::EVENT_TYPE_PREVIEW_FAULT))
    {
        event_key = client->subsKey_;
    }
    else if (event_type == getEventNotificationString(EventType::EVENT_TYPE_CAPTURE_FAULT))
    {
        event_key = CONST_EVENT_KEY_CAPTURE_FAULT;
        for (const auto& handle : client->devHandles_)
            CommandManager::getInstance().stopCapture(handle, false);
        client->devHandles_.clear();
    }
    else
    {
        PLOGE("Invalid event %s", event_type.c_str());
        LSErrorFree(&lserror);
        return false;
    }

    unsigned int num_subscribers =
        LSSubscriptionGetHandleSubscribersCount(client->sh_, event_key.c_str());
    PLOGI("num_subscribers : %u", num_subscribers);
    if (num_subscribers > 0)
    {
        PLOGI("notifying %s : %s", event_type.c_str(), msg);
        if (!LSSubscriptionReply(client->sh_, event_key.c_str(), msg, &lserror))
        {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
            PLOGI("subscription reply failed");
            return false;
        }
        PLOGI("notified %s event !!", event_type.c_str());
    }

    LSErrorFree(&lserror);

    return true;
}

CameraHalProxy::CameraHalProxy() : state_(State::INIT)
{
    PLOGI("");

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

    pthread_setname_np(loopThread_->native_handle(), "halproxy_luna");

    std::string guid         = GenerateUniqueID()();
    std::string service_name = cstr_uricameramain + "." + guid;
    luna_client              = std::make_unique<LunaClient>(service_name.c_str(), c);
    g_main_context_unref(c);

    // start process
    uid_         = cstr_uricamearhal + guid;
    service_uri_ = "luna://" + uid_ + "/";

    std::string cmd = "/usr/sbin/" + CameraHalProcessName + " -s" + uid_;
    process_        = std::make_unique<Process>(cmd);
}

CameraHalProxy::~CameraHalProxy()
{
    PLOGI("state_ %d", static_cast<int>(state_));

    try
    {
        unsubscribe();
        if (state_ != State::DESTROY)
        {
            destroyHal();
        }
    }
    catch (const std::logic_error &e)
    {
        PLOGE("Caught a std::logic_error meaning %s", e.what());
    }

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
}

DEVICE_RETURN_CODE_T CameraHalProxy::open(std::string devicenode, int ndev_id, std::string payload)
{
    PLOGI("devicenode : %s, ndev_id : %d, payload [%s]", devicenode.c_str(), ndev_id,
          payload.c_str());

    json jin;
    jin[CONST_PARAM_NAME_DEVICE_PATH] = devicenode;
    jin[CONST_PARAM_NAME_CAMERAID]    = ndev_id;
    jin[CONST_PARAM_NAME_PAYLOAD]     = payload;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::close()
{
    PLOGI("");
    return luna_call_sync(__func__, "{}");
}

DEVICE_RETURN_CODE_T CameraHalProxy::startPreview(std::string memtype, int *pkey, LSHandle *sh,
                                                  const char *subskey)
{
    PLOGI("");

    sh_      = sh;
    subsKey_ = subskey ? subskey : "";

    json jin;
    jin[CONST_PARAM_NAME_MEMTYPE] = memtype;

    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, to_string(jin), COMMAND_TIMEOUT_LONG);

    if (ret == DEVICE_OK)
    {
        *pkey = get_optional<int>(jOut, CONST_PARAM_NAME_SHMKEY).value_or(0);
    }

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::stopPreview(int memtype)
{
    PLOGI("memtype : %d", memtype);

    json jin;
    jin[CONST_PARAM_NAME_MEMTYPE] = memtype;

    return luna_call_sync(__func__, to_string(jin), COMMAND_TIMEOUT_LONG);
}

DEVICE_RETURN_CODE_T CameraHalProxy::startCapture(CAMERA_FORMAT sformat,
                                                  const std::string &imagepath,
                                                  const std::string &mode, int ncount,
                                                  const int devHandle)
{
    PLOGI("");

    json jin;
    jin[CONST_PARAM_NAME_NCOUNT]     = ncount;
    jin[CONST_PARAM_NAME_FORMAT]     = sformat.eFormat;
    jin[CONST_PARAM_NAME_WIDTH]      = sformat.nWidth;
    jin[CONST_PARAM_NAME_HEIGHT]     = sformat.nHeight;
    jin[CONST_PARAM_NAME_IMAGE_PATH] = imagepath;
    jin[CONST_PARAM_NAME_MODE]       = mode;

    if (devHandle) {
        auto itr = std::find(devHandles_.begin(), devHandles_.end(), devHandle);
        if (itr == devHandles_.end())
            devHandles_.push_back(devHandle);
    }

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::stopCapture(const int devHandle)
{
    PLOGI("");
    auto itr = std::find(devHandles_.begin(), devHandles_.end(), devHandle);
    if (itr != devHandles_.end())
        devHandles_.erase(itr);
    return luna_call_sync(__func__, "{}");
}

DEVICE_RETURN_CODE_T CameraHalProxy::createHal(std::string subsystem)
{
    PLOGI("subsystem : %s", subsystem.c_str());
    state_ = State::CREATE;

    json jin;
    jin[CONST_PARAM_NAME_SUBSYSTEM] = subsystem;
    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::destroyHal()
{
    PLOGI("");
    state_ = State::DESTROY;

    return luna_call_sync(__func__, "{}");
}

DEVICE_RETURN_CODE_T CameraHalProxy::getDeviceInfo(std::string strdevicenode,
                                                   std::string strdevicetype,
                                                   camera_device_info_t *pinfo)
{
    PLOGI("device node : %s, device type : %s", strdevicenode.c_str(), strdevicetype.c_str());

    // 1. start process
    std::string serviceName = cstr_uricamearhal + GenerateUniqueID()();
    std::string cmd         = "/usr/sbin/" + CameraHalProcessName + " -s" + serviceName;

    std::unique_ptr<Process> proc_ = std::make_unique<Process>(cmd);

    // 2. LunaClient
    GMainContext *c = g_main_context_new();
    GMainLoop *lp   = g_main_loop_new(c, false);
    std::unique_ptr<std::thread> lpthd;

    try
    {
        lpthd = std::make_unique<std::thread>(g_main_loop_run, lp);
    }
    catch (const std::system_error &e)
    {
        PLOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
    }

    while (!g_main_loop_is_running(lp))
    {
    }

    pthread_setname_np(lpthd->native_handle(), "getinfo_luna");

    std::string ls_service_name    = CameraHalProcessName + "." + __func__;
    std::unique_ptr<LunaClient> lc = std::make_unique<LunaClient>(ls_service_name.c_str(), c);
    g_main_context_unref(c);

    // 3. sendMessage
    std::string uri = "luna://" + serviceName + "/" + __func__;

    json jin;
    jin[CONST_PARAM_NAME_DEVICE_PATH] = strdevicenode;
    jin[CONST_PARAM_NAME_SUBSYSTEM]   = strdevicetype;
    std::string payload               = to_string(jin);
    PLOGI("%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    lc->callSync(uri.c_str(), payload.c_str(), &resp, COMMAND_TIMEOUT);
    PLOGI("resp : %s", resp.c_str());

    auto j = json::parse(resp, nullptr, false);
    if (j.is_discarded())
    {
        PLOGE("resp parsing error!");
        return DEVICE_ERROR_UNKNOWN;
    }

    DEVICE_RETURN_CODE_T ret_code = DEVICE_OK;
    bool ret_value = get_optional<bool>(j, CONST_PARAM_NAME_RETURNVALUE).value_or(false);
    PLOGI("%s : %d", CONST_PARAM_NAME_RETURNVALUE, ret_value);
    if (ret_value)
    {
        pinfo->n_devicetype =
            get_optional<device_t>(j, CONST_PARAM_NAME_DEVICE_TYPE).value_or(DEVICE_TYPE_UNDEFINED);
        pinfo->b_builtin = get_optional<int>(j, CONST_PARAM_NAME_BUILTIN).value_or(0);

        auto r = j[CONST_PARAM_NAME_RESOLUTION];
        for (json::iterator it = r.begin(); it != r.end(); ++it)
        {
            std::vector<std::string> v_res;
            for (const auto &item : it.value())
            {
                v_res.emplace_back(item);
            }
            camera_format_t eformat;
            convertFormatToCode(it.key(), &eformat);
            pinfo->stResolution.emplace_back(v_res, eformat);
        }
    }
    else
    {
        ret_code = get_optional<DEVICE_RETURN_CODE_T>(j, CONST_PARAM_NAME_ERROR_CODE)
                       .value_or(DEVICE_RETURN_UNDEFINED);
    }

    g_main_loop_quit(lp);
    if (lpthd->joinable())
    {
        try
        {
            lpthd->join();
        }
        catch (const std::system_error &e)
        {
            PLOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
        }
    }
    g_main_loop_unref(lp);

    return ret_code;
}

DEVICE_RETURN_CODE_T CameraHalProxy::getDeviceProperty(CAMERA_PROPERTIES_T *oparams)
{
    PLOGI("");

    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, "{}");

    if (ret == DEVICE_OK)
    {
        auto jobj_params = jOut[CONST_PARAM_NAME_PARAMS];

        for (json::iterator it = jobj_params.begin(); it != jobj_params.end(); ++it)
        {
            if (it.value().is_object() == false)
                continue;

            int i = getParamNumFromString(it.key());
            if (i >= 0)
            {
                json queries = jobj_params[it.key()];
                for (json::iterator q = queries.begin(); q != queries.end(); ++q)
                {
                    int n = getQueryNumFromString(q.key());
                    if (n >= 0)
                        oparams->stGetData.data[i][n] = q.value();
                }
            }
        }
    }

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::setDeviceProperty(CAMERA_PROPERTIES_T *inparams)
{
    PLOGI("");

    json jin;
    for (int i = 0; i < PROPERTY_END; i++)
    {
        if (inparams->stGetData.data[i][QUERY_VALUE] == CONST_PARAM_DEFAULT_VALUE)
            continue;
        jin[CONST_PARAM_NAME_PARAMS][getParamString(i)] = inparams->stGetData.data[i][QUERY_VALUE];
    }

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::setFormat(CAMERA_FORMAT sformat)
{
    PLOGI("");

    json jin;
    jin[CONST_PARAM_NAME_WIDTH]  = sformat.nWidth;
    jin[CONST_PARAM_NAME_HEIGHT] = sformat.nHeight;
    jin[CONST_PARAM_NAME_FPS]    = sformat.nFps;
    jin[CONST_PARAM_NAME_FORMAT] = sformat.eFormat;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::getFormat(CAMERA_FORMAT *pformat)
{
    PLOGI("");

    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, "{}");

    if (ret == DEVICE_OK)
    {
        int w            = get_optional<int>(jOut, CONST_PARAM_NAME_WIDTH).value_or(0);
        int h            = get_optional<int>(jOut, CONST_PARAM_NAME_HEIGHT).value_or(0);
        pformat->nWidth  = (w > 0) ? w : 0;
        pformat->nHeight = (h > 0) ? h : 0;
        pformat->nFps    = get_optional<int>(jOut, CONST_PARAM_NAME_FPS).value_or(0);
        pformat->eFormat = get_optional<camera_format_t>(jOut, CONST_PARAM_NAME_FORMAT)
                               .value_or(CAMERA_FORMAT_UNDEFINED);
    }

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::getFd(int *shmfd)
{
    PLOGI("");
    LSMessageToken tok = 0;
    LSError lserror;
    LSErrorInit(&lserror);
    GMainContext *context = g_main_loop_get_context(loop_);
    std::string uri       = service_uri_ + __func__;
    struct FdTaker
    {
        const char *response;
        int fd;
        bool done;
    } worker{"", 0, false};
    bool ret = false;

    ret = LSCall(
        luna_client->get(), uri.c_str(), "{}",
        +[](LSHandle *sh, LSMessage *msg, void *data)
        {
            FdTaker *taker  = (FdTaker *)data;
            taker->response = LSMessageGetPayload(msg);
            LS::Message ls_message(msg);
            LS::PayloadRef payload_ref = ls_message.accessPayload();
            int fd                     = payload_ref.getFd();
            if (fd)
            {
                taker->fd = dup(fd);
            }
            taker->done = true;
            return true;
        },
        &worker, &tok, &lserror);

    if (ret == true)
    {
        ret = LSCallSetTimeout(luna_client->get(), tok, 30, &lserror);
        if (ret == true)
        {
            while (!worker.done)
            {
                g_main_context_iteration(context, false);
                usleep(500);
            }

            if (get_optional<bool>(jOut, CONST_PARAM_NAME_RETURNVALUE).value_or(false))
            {
                *shmfd = worker.fd;
                return DEVICE_OK;
            }
            else
            {
                *shmfd = -1;
                return get_optional<DEVICE_RETURN_CODE_T>(jOut, CONST_PARAM_NAME_ERROR_CODE)
                    .value_or(DEVICE_RETURN_UNDEFINED);
            }
        }

        *shmfd = -1;
        return DEVICE_ERROR_UNKNOWN;
    }

    LSErrorPrint(&lserror, stderr);
    LSErrorFree(&lserror);
    *shmfd = -1;
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CameraHalProxy::registerClient(pid_t pid, int sig, int devhandle,
                                                    std::string &outmsg)
{
    PLOGI("");

    json jin;
    jin[CONST_CLIENT_PROCESS_ID]    = pid;
    jin[CONST_CLIENT_SIGNAL_NUM]    = sig;
    jin[CONST_PARAM_NAME_DEVHANDLE] = devhandle;

    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, to_string(jin));

    outmsg = get_optional<std::string>(jOut, CONST_PARAM_NAME_OUTMSG).value_or("");

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::unregisterClient(pid_t pid, std::string &outmsg)
{
    PLOGI("");

    json jin;
    jin[CONST_CLIENT_PROCESS_ID] = pid;

    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, to_string(jin));

    outmsg = get_optional<std::string>(jOut, CONST_PARAM_NAME_OUTMSG).value_or("");

    return ret;
}

bool CameraHalProxy::isRegisteredClient(int devhandle)
{
    PLOGI("handle %d", devhandle);

    json jin;
    jin[CONST_PARAM_NAME_DEVHANDLE] = devhandle;

    luna_call_sync(__func__, to_string(jin));

    return get_optional<bool>(jOut, CONST_PARAM_NAME_REGISTER).value_or(false);
}

void CameraHalProxy::requestPreviewCancel()
{
    PLOGI("");
    luna_call_sync(__func__, "{}");
}

//[Camera Solution Manager] interfaces start
DEVICE_RETURN_CODE_T
CameraHalProxy::getSupportedCameraSolutionInfo(std::vector<std::string> &solutionsInfo)
{
    PLOGI("");

    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, "{}");

    if (ret == DEVICE_OK)
    {
        for (auto s : jOut[CONST_PARAM_NAME_SOLUTIONS])
        {
            if (!s.is_string())
                continue;
            solutionsInfo.push_back(s);
        }
    }

    return ret;
}

DEVICE_RETURN_CODE_T
CameraHalProxy::getEnabledCameraSolutionInfo(std::vector<std::string> &solutionsInfo)
{
    PLOGI("");

    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, "{}");

    if (ret == DEVICE_OK)
    {
        for (auto s : jOut[CONST_PARAM_NAME_SOLUTIONS])
        {
            if (!s.is_string())
                continue;
            solutionsInfo.push_back(s);
        }
    }

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::enableCameraSolution(const std::vector<std::string> solutions)
{
    PLOGI("");

    json jin;
    jin[CONST_PARAM_NAME_SOLUTIONS] = json::array();
    for (auto s : solutions)
    {
        jin[CONST_PARAM_NAME_SOLUTIONS].push_back(s);
    }

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::disableCameraSolution(const std::vector<std::string> solutions)
{
    PLOGI("");

    json jin;
    jin[CONST_PARAM_NAME_SOLUTIONS] = json::array();
    for (auto s : solutions)
    {
        jin[CONST_PARAM_NAME_SOLUTIONS].push_back(s);
    }

    return luna_call_sync(__func__, to_string(jin));
}
//[Camera Solution Manager] interfaces end

bool CameraHalProxy::subscribe()
{
    PLOGI("");

    auto func = [](LSHandle *input_handle, const char *service_name, bool connected,
                   void *ctx) -> bool
    {
        PLOGI("[ServerStatus cb] connected=%d, name=%s\n", connected, service_name);

        CameraHalProxy *self = static_cast<CameraHalProxy *>(ctx);
        if (connected)
        {
            std::string uri = self->service_uri_ + "subscribe";
            bool ret        = self->luna_client->subscribe(uri.c_str(), "{\"subscribe\":true}",
                                                           &self->subscribeKey_, cameraHalServiceCb, self);
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
    };

    if (!LSRegisterServerStatusEx(sh_, uid_.c_str(), func, static_cast<void *>(this), &cookie,
                                  nullptr))
    {
        PLOGE("[ServerStatus cb] error LSRegisterServerStatusEx\n");
    }

    return true;
}

bool CameraHalProxy::unsubscribe()
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
        try
        {
            if (!LSCancelServerStatus(sh_, cookie, nullptr))
            {
                PLOGE("error LSCancelServerStatus");
            }
        }
        catch (const std::ios::failure &e)
        {
            PLOGE("Caught a std::ios::failure %s", e.what());
        }
        cookie = nullptr;
    }

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::luna_call_sync(const char *func, const std::string &payload,
                                                    int timeout)
{
    if (process_ == nullptr)
    {
        PLOGE("hal process is not ready");
        return DEVICE_ERROR_UNKNOWN;
    }

    if (func == nullptr)
    {
        PLOGE("no method name");
        return DEVICE_ERROR_UNKNOWN;
    }

    // send message
    std::string uri = service_uri_ + func;
    PLOGI("%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    int64_t startClk = g_get_monotonic_time();
    luna_client->callSync(uri.c_str(), payload.c_str(), &resp, timeout);
    int64_t endClk = g_get_monotonic_time();

    (startClk > endClk) ? PLOGE("diffClk is error")
                        : PLOGI("response %s, runtime %lld", resp.c_str(),
                                (long long int)((endClk - startClk) / 1000));

    try
    {
        jOut = json::parse(resp);
    }
    catch (const std::exception &e)
    {
        PLOGE("Error parsing JSON: %s", e.what());
    }

    if (jOut.is_discarded())
    {
        PLOGE("payload parsing error!");
        return DEVICE_ERROR_JSON_PARSING;
    }

    bool ret = get_optional<bool>(jOut, CONST_PARAM_NAME_RETURNVALUE).value_or(false);
    if (ret)
    {
        return DEVICE_OK;
    }
    else
    {
        return get_optional<DEVICE_RETURN_CODE_T>(jOut, CONST_PARAM_NAME_ERROR_CODE)
            .value_or(DEVICE_RETURN_UNDEFINED);
    }
}
