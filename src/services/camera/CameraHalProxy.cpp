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

#include "CameraHalProxy.h"
#include "GenerateUniqueID.h"
#include "LunaClient.h"
#include "Process.h"
#include "json_utils.h"

const std::string CameraHalProcessName = "com.webos.service.camera2.hal";
#define COMMAND_TIMEOUT_LONG 3000 // TODO : Is the minimum value sufficient?

static bool cameraHalServiceCb(const char *msg, void *data)
{
    PMLOG_INFO(CONST_MODULE_CHP, "%s", msg);

    json j = json::parse(msg, nullptr, false);
    if (j.is_discarded())
    {
        PMLOG_ERROR(CONST_MODULE_CHP, "msg parsing error!");
        return false;
    }

    std::string event_type = get_optional<std::string>(j, CONST_PARAM_NAME_EVENT).value_or("");
    if (event_type == getEventNotificationString(EventType::EVENT_TYPE_PREVIEW_FAULT))
    {
        CameraHalProxy *client = (CameraHalProxy *)data;

        int num_subscribers =
            LSSubscriptionGetHandleSubscribersCount(client->sh_, client->subsKey_.c_str());
        PMLOG_INFO(CONST_MODULE_CHP, "num_subscribers : %d", num_subscribers);
        if (num_subscribers > 0)
        {
            LSError lserror;
            LSErrorInit(&lserror);

            PMLOG_INFO(CONST_MODULE_CHP, "notifying preview fault :  %s", msg);
            if (!LSSubscriptionReply(client->sh_, client->subsKey_.c_str(), msg, &lserror))
            {
                LSErrorPrint(&lserror, stderr);
                LSErrorFree(&lserror);
                PMLOG_INFO(CONST_MODULE_CHP, "subscription reply failed");
                return false;
            }
            PMLOG_INFO(CONST_MODULE_CHP, "notified preview fault event !!");

            LSErrorFree(&lserror);
        }
    }

    return true;
}

CameraHalProxy::CameraHalProxy()
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    GMainContext *c = g_main_context_new();
    loop_           = g_main_loop_new(c, false);

    try
    {
        loopThread_ = std::make_unique<std::thread>(g_main_loop_run, loop_);
    }
    catch (const std::system_error &e)
    {
        PMLOG_INFO(CONST_MODULE_CHP, "Caught a system_error with code %d meaning %s",
                   e.code().value(), e.what());
    }

    while (!g_main_loop_is_running(loop_))
    {
    }

    std::string guid         = GenerateUniqueID()();
    std::string service_name = CameraHalProcessName + "." + guid;
    luna_client              = std::make_unique<LunaClient>(service_name.c_str(), c);
    g_main_context_unref(c);

    // start process
    std::string uid = cstr_uricamearhal + guid;
    service_uri_    = "luna://" + uid + "/";

    std::string cmd = "/usr/sbin/" + CameraHalProcessName + " -s" + uid;
    process_        = std::make_unique<Process>(cmd);
}

CameraHalProxy::~CameraHalProxy()
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    g_main_loop_quit(loop_);
    if (loopThread_->joinable())
    {
        try
        {
            loopThread_->join();
        }
        catch (const std::system_error &e)
        {
            PMLOG_INFO(CONST_MODULE_CHP, "Caught a system_error with code %d meaning %s",
                       e.code().value(), e.what());
        }
    }
    g_main_loop_unref(loop_);
}

DEVICE_RETURN_CODE_T CameraHalProxy::open(std::string devicenode, int ndev_id, std::string payload)
{
    PMLOG_INFO(CONST_MODULE_CHP, "devicenode : %s, ndev_id : %d, payload [%s]", devicenode.c_str(),
               ndev_id, payload.c_str());

    json jin;
    jin[CONST_PARAM_NAME_DEVICE_PATH] = devicenode;
    jin[CONST_PARAM_NAME_CAMERAID]    = ndev_id;
    jin[CONST_PARAM_NAME_PAYLOAD]     = payload;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::close()
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    std::string payload = "{}";
    return luna_call_sync(__func__, payload);
}

DEVICE_RETURN_CODE_T CameraHalProxy::startPreview(std::string memtype, int *pkey, LSHandle *sh,
                                                  const char *subskey)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    sh_      = sh;
    subsKey_ = subskey;

    json jin;
    jin[CONST_PARAM_NAME_MEMTYPE] = memtype;

    json j;
    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, to_string(jin), &j);

    if (ret == DEVICE_OK)
    {
        *pkey = get_optional<int>(j, CONST_PARAM_NAME_SHMKEY).value_or(0);
    }

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::stopPreview(int memtype)
{
    PMLOG_INFO(CONST_MODULE_CHP, "memtype : %d", memtype);

    json jin;
    jin[CONST_PARAM_NAME_MEMTYPE] = memtype;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::startCapture(CAMERA_FORMAT sformat,
                                                  const std::string &imagepath)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    jin[CONST_PARAM_NAME_FORMAT]     = sformat.eFormat;
    jin[CONST_PARAM_NAME_WIDTH]      = sformat.nWidth;
    jin[CONST_PARAM_NAME_HEIGHT]     = sformat.nHeight;
    jin[CONST_PARAM_NAME_IMAGE_PATH] = imagepath;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::stopCapture()
{
    PMLOG_INFO(CONST_MODULE_CHP, "");
    return luna_call_sync(__func__, "{}");
}

DEVICE_RETURN_CODE_T CameraHalProxy::captureImage(int ncount, CAMERA_FORMAT sformat,
                                                  const std::string &imagepath,
                                                  const std::string &mode)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    jin[CONST_PARAM_NAME_NCOUNT]     = ncount;
    jin[CONST_PARAM_NAME_FORMAT]     = sformat.eFormat;
    jin[CONST_PARAM_NAME_WIDTH]      = sformat.nWidth;
    jin[CONST_PARAM_NAME_HEIGHT]     = sformat.nHeight;
    jin[CONST_PARAM_NAME_IMAGE_PATH] = imagepath;
    jin[CONST_PARAM_NAME_MODE]       = mode;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::createHandle(std::string subsystem)
{
    PMLOG_INFO(CONST_MODULE_CHP, "subsystem : %s", subsystem.c_str());

    json jin;
    jin[CONST_PARAM_NAME_SUBSYSTEM] = subsystem;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::destroyHandle()
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    std::string payload = "{}";
    return luna_call_sync(__func__, payload);
}

DEVICE_RETURN_CODE_T CameraHalProxy::getDeviceInfo(std::string strdevicenode,
                                                   std::string strdevicetype,
                                                   camera_device_info_t *pinfo)
{
    PMLOG_INFO(CONST_MODULE_CHP, "device node : %s, device type : %s", strdevicenode.c_str(),
               strdevicetype.c_str());

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
        PMLOG_INFO(CONST_MODULE_CHP, "Caught a system_error with code %d meaning %s",
                   e.code().value(), e.what());
    }

    while (!g_main_loop_is_running(lp))
    {
    }

    std::string ls_service_name    = CameraHalProcessName + "." + __func__;
    std::unique_ptr<LunaClient> lc = std::make_unique<LunaClient>(ls_service_name.c_str(), c);
    g_main_context_unref(c);

    // 3. sendMessage
    std::string uri = "luna://" + serviceName + "/" + __func__;

    json jin;
    jin[CONST_PARAM_NAME_DEVICE_PATH] = strdevicenode;
    jin[CONST_PARAM_NAME_SUBSYSTEM]   = strdevicetype;
    std::string payload               = to_string(jin);
    PMLOG_INFO(CONST_MODULE_CHP, "%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    lc->callSync(uri.c_str(), payload.c_str(), &resp, COMMAND_TIMEOUT_LONG);
    PMLOG_INFO(CONST_MODULE_CHP, "resp : %s", resp.c_str());

    auto j = json::parse(resp, nullptr, false);
    if (j.is_discarded())
    {
        PMLOG_ERROR(CONST_MODULE_CHP, "resp parsing error!");
        return DEVICE_ERROR_UNKNOWN;
    }

    DEVICE_RETURN_CODE_T ret = get_optional<DEVICE_RETURN_CODE_T>(j, CONST_PARAM_NAME_RETURNCODE)
                                   .value_or(DEVICE_RETURN_UNDEFINED);
    PMLOG_INFO(CONST_MODULE_CHP, "%s : %d", CONST_PARAM_NAME_RETURNCODE, ret);
    if (ret == DEVICE_OK)
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

    g_main_loop_quit(lp);
    if (lpthd->joinable())
    {
        try
        {
            lpthd->join();
        }
        catch (const std::system_error &e)
        {
            PMLOG_INFO(CONST_MODULE_CHP, "Caught a system_error with code %d meaning %s",
                       e.code().value(), e.what());
        }
    }
    g_main_loop_unref(lp);

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::getDeviceProperty(CAMERA_PROPERTIES_T *oparams)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json j;
    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, "{}", &j);

    if (ret == DEVICE_OK)
    {
        for (json::iterator it = j.begin(); it != j.end(); ++it)
        {
            if (it.value().is_object() == false)
                continue;

            int i = getParamNumFromString(it.key());
            if (i != -1)
            {
                json queries = j[it.key()];
                for (json::iterator q = queries.begin(); q != queries.end(); ++q)
                {
                    oparams->stGetData.data[i][getQueryNumFromString(q.key())] = q.value();
                }
            }
        }
    }

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::setDeviceProperty(CAMERA_PROPERTIES_T *inparams)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    for (int i = 0; i < PROPERTY_END; i++)
    {
        if (inparams->stGetData.data[i][QUERY_VALUE] == CONST_PARAM_DEFAULT_VALUE)
            continue;
        jin[getParamString(i)] = inparams->stGetData.data[i][QUERY_VALUE];
    }

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::setFormat(CAMERA_FORMAT sformat)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    jin[CONST_PARAM_NAME_WIDTH]  = sformat.nWidth;
    jin[CONST_PARAM_NAME_HEIGHT] = sformat.nHeight;
    jin[CONST_PARAM_NAME_FPS]    = sformat.nFps;
    jin[CONST_PARAM_NAME_FORMAT] = sformat.eFormat;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::getFormat(CAMERA_FORMAT *pformat)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json j;
    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, "{}", &j);

    if (ret == DEVICE_OK)
    {
        pformat->nWidth  = get_optional<int>(j, CONST_PARAM_NAME_WIDTH).value_or(0);
        pformat->nHeight = get_optional<int>(j, CONST_PARAM_NAME_HEIGHT).value_or(0);
        pformat->nFps    = get_optional<int>(j, CONST_PARAM_NAME_FPS).value_or(0);
        pformat->eFormat = get_optional<camera_format_t>(j, CONST_PARAM_NAME_FORMAT)
                               .value_or(CAMERA_FORMAT_UNDEFINED);
    }

    return ret;
}

bool CameraHalProxy::registerClient(pid_t pid, int sig, int devhandle, std::string &outmsg)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    jin[CONST_CLIENT_PROCESS_ID]    = pid;
    jin[CONST_CLIENT_SIGNAL_NUM]    = sig;
    jin[CONST_PARAM_NAME_DEVHANDLE] = devhandle;

    json j;
    bool ret = luna_call_sync_bool(__func__, to_string(jin), &j);

    outmsg = get_optional<std::string>(j, CONST_PARAM_NAME_OUTMSG).value_or("");

    return ret;
}

bool CameraHalProxy::unregisterClient(pid_t pid, std::string &outmsg)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    jin[CONST_CLIENT_PROCESS_ID] = pid;

    json j;
    bool ret = luna_call_sync_bool(__func__, to_string(jin), &j);

    outmsg = get_optional<std::string>(j, CONST_PARAM_NAME_OUTMSG).value_or("");

    return ret;
}

bool CameraHalProxy::isRegisteredClient(int devhandle)
{
    PMLOG_INFO(CONST_MODULE_CHP, "handle %d", devhandle);

    json jin;
    jin[CONST_PARAM_NAME_DEVHANDLE] = devhandle;

    return luna_call_sync_bool(__func__, to_string(jin));
}

void CameraHalProxy::requestPreviewCancel()
{
    PMLOG_INFO(CONST_MODULE_CHP, "");
    luna_call_sync_bool(__func__, "{}");
}

//[Camera Solution Manager] interfaces start
DEVICE_RETURN_CODE_T
CameraHalProxy::getSupportedCameraSolutionInfo(std::vector<std::string> &solutionsInfo)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    std::string payload = "{}";
    json j;
    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, payload, &j);

    if (ret == DEVICE_OK)
    {
        for (auto s : j[CONST_PARAM_NAME_SOLUTIONS])
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
    PMLOG_INFO(CONST_MODULE_CHP, "");

    std::string payload = "{}";
    json j;
    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, payload, &j);

    if (ret == DEVICE_OK)
    {
        for (auto s : j[CONST_PARAM_NAME_SOLUTIONS])
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
    PMLOG_INFO(CONST_MODULE_CHP, "");

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
    PMLOG_INFO(CONST_MODULE_CHP, "");

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
    std::string uri = service_uri_ + __func__;
    bool ret        = luna_client->subscribe(uri.c_str(), "{\"subscribe\":true}", &subscribeKey_,
                                             cameraHalServiceCb, this);
    PMLOG_INFO(CONST_MODULE_CHP, "subscribeKey_ %ld, %d ", subscribeKey_, ret);
    return ret;
}

bool CameraHalProxy::unsubscribe()
{
    bool ret = true;
    if (subscribeKey_)
    {
        PMLOG_INFO(CONST_MODULE_CHP, "remove subscribeKey_ %ld", subscribeKey_);
        ret           = luna_client->unsubscribe(subscribeKey_);
        subscribeKey_ = 0;
    }
    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::luna_call_sync(const char *func, const std::string &payload,
                                                    json *jin)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    if (process_ == nullptr)
    {
        PMLOG_INFO(CONST_MODULE_CHP, "hal process is not ready");
        return DEVICE_ERROR_UNKNOWN;
    }

    // send message
    std::string uri = service_uri_ + func;
    PMLOG_INFO(CONST_MODULE_CHP, "%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), payload.c_str(), &resp, COMMAND_TIMEOUT_LONG);
    PMLOG_INFO(CONST_MODULE_CHP, "resp : %s", resp.c_str());

    DEVICE_RETURN_CODE_T ret;
    if (jin)
    {
        *jin = json::parse(resp, nullptr, false);
        ret  = get_optional<DEVICE_RETURN_CODE_T>(*jin, CONST_PARAM_NAME_RETURNCODE)
                  .value_or(DEVICE_RETURN_UNDEFINED);
    }
    else
    {
        json j = json::parse(resp, nullptr, false);
        ret    = get_optional<DEVICE_RETURN_CODE_T>(j, CONST_PARAM_NAME_RETURNCODE)
                  .value_or(DEVICE_RETURN_UNDEFINED);
    }

    PMLOG_INFO(CONST_MODULE_CHP, "%s : %d", CONST_PARAM_NAME_RETURNCODE, ret);
    return ret;
}

bool CameraHalProxy::luna_call_sync_bool(const char *func, const std::string &payload, json *jin)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    if (process_ == nullptr)
    {
        PMLOG_INFO(CONST_MODULE_CHP, "hal process is not ready");
        return false;
    }

    // send message
    std::string uri = service_uri_ + func;
    PMLOG_INFO(CONST_MODULE_CHP, "%s '%s'", uri.c_str(), payload.c_str());

    std::string resp;
    luna_client->callSync(uri.c_str(), payload.c_str(), &resp, COMMAND_TIMEOUT_LONG);
    PMLOG_INFO(CONST_MODULE_CHP, "resp : %s", resp.c_str());

    bool ret;
    if (jin)
    {
        *jin = json::parse(resp, nullptr, false);
        ret  = get_optional<bool>(*jin, CONST_PARAM_NAME_RETURNVALUE).value_or(false);
    }
    else
    {
        json j = json::parse(resp, nullptr, false);
        ret    = get_optional<bool>(j, CONST_PARAM_NAME_RETURNVALUE).value_or(false);
    }

    PMLOG_INFO(CONST_MODULE_CHP, "%s : %d", CONST_PARAM_NAME_RETURNVALUE, ret);
    return ret;
}
