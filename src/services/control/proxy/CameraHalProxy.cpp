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

const std::string CameraHalProcessName      = "com.webos.service.camera2.hal";
const std::string CameraHalConnectionBaseId = "com.webos.camerahal.";
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

    std::string eventType = get_optional<std::string>(j, "eventType").value_or("");
    if (eventType == getEventNotificationString(EventType::EVENT_TYPE_DEVICE_FAULT))
    {
        CameraHalProxy *client = (CameraHalProxy *)data;

        int num_subscribers =
            LSSubscriptionGetHandleSubscribersCount(client->sh_, client->subsKey_.c_str());
        PMLOG_INFO(CONST_MODULE_CHP, "num_subscribers : %d", num_subscribers);
        if (num_subscribers > 0)
        {
            LSError lserror;
            LSErrorInit(&lserror);

            PMLOG_INFO(CONST_MODULE_CHP, "notifying device fault :  %s", msg);
            if (!LSSubscriptionReply(client->sh_, client->subsKey_.c_str(), msg, &lserror))
            {
                LSErrorPrint(&lserror, stderr);
                LSErrorFree(&lserror);
                PMLOG_INFO(CONST_MODULE_CHP, "subscription reply failed");
                return false;
            }
            PMLOG_INFO(CONST_MODULE_CHP, "notified device fault event !!");

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
    std::string uid = CameraHalConnectionBaseId + guid;
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
    jin["devPath"]  = devicenode;
    jin["cameraID"] = ndev_id;
    jin["payload"]  = payload;

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
    jin["memType"] = memtype;

    json j;
    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, to_string(jin), &j);

    if (ret == DEVICE_OK)
    {
        *pkey = get_optional<int>(j, "shmKey").value_or(0);
    }

    return ret;
}

DEVICE_RETURN_CODE_T CameraHalProxy::stopPreview(int memtype)
{
    PMLOG_INFO(CONST_MODULE_CHP, "memtype : %d", memtype);

    json jin;
    jin["memType"] = memtype;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::startCapture(CAMERA_FORMAT sformat,
                                                  const std::string &imagepat)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    // TBD
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T CameraHalProxy::stopCapture()
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    // TBD
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T CameraHalProxy::captureImage(int ncount, CAMERA_FORMAT sformat,
                                                  const std::string &imagepath,
                                                  const std::string &mode)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    // TBD
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T CameraHalProxy::createHandle(std::string subsystem)
{
    PMLOG_INFO(CONST_MODULE_CHP, "subsystem : %s", subsystem.c_str());

    json jin;
    jin["subSystem"] = subsystem;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::destroyHandle()
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    std::string payload = "{}";
    return luna_call_sync(__func__, payload);
}

DEVICE_RETURN_CODE_T CameraHalProxy::getDeviceInfo(std::string strdevicenode,
                                                   std::string deviceType,
                                                   camera_device_info_t *pinfo)
{
    PMLOG_INFO(CONST_MODULE_CHP, "device node : %s, device type : %s", strdevicenode.c_str(),
               deviceType.c_str());

    // 1. start process
    std::string serviceName = CameraHalConnectionBaseId + GenerateUniqueID()();
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
    jin["devPath"]      = strdevicenode;
    jin["subSystem"]    = deviceType;
    std::string payload = to_string(jin);
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

    bool ret = get_optional<bool>(j, "returnValue").value_or(false);
    PMLOG_INFO(CONST_MODULE_CHP, "returnValue : %d", ret);
    if (ret)
    {
        pinfo->n_devicetype =
            get_optional<device_t>(j, "deviceType").value_or(DEVICE_TYPE_UNDEFINED);
        pinfo->b_builtin = get_optional<int>(j, "builtin").value_or(0);
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

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T CameraHalProxy::getDeviceProperty(CAMERA_PROPERTIES_T *oparams)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    // TBD
    //  update resolution structure
    {
        std::vector<std::string> c_res;
        c_res.clear();
        c_res.push_back("320,240,30");
        c_res.push_back("640,480,30");
        c_res.push_back("1280,720,30");
        c_res.push_back("1920,1080,30");
        oparams->stResolution.emplace_back(c_res, CAMERA_FORMAT_JPEG);
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T CameraHalProxy::setDeviceProperty(CAMERA_PROPERTIES_T *inparams)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    // TBD
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T CameraHalProxy::setFormat(CAMERA_FORMAT sformat)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    jin["width"]        = sformat.nWidth;
    jin["height"]       = sformat.nHeight;
    jin["fps"]          = sformat.nFps;
    jin["cameraFormat"] = sformat.eFormat;

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::getFormat(CAMERA_FORMAT *pformat)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    // TBD
    return DEVICE_OK;
}

bool CameraHalProxy::registerClient(pid_t pid, int sig, int devhandle, std::string &outmsg)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    jin["pid"]       = pid;
    jin["sig"]       = sig;
    jin["devHandle"] = devhandle;

    json j;
    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, to_string(jin), &j);

    if (ret == DEVICE_OK)
    {
        outmsg = get_optional<std::string>(j, "outMsg").value_or("");
        return true;
    }

    return false;
}

bool CameraHalProxy::unregisterClient(pid_t pid, std::string &outmsg)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    jin["pid"] = pid;

    json j;
    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, to_string(jin), &j);

    if (ret == DEVICE_OK)
    {
        outmsg = get_optional<std::string>(j, "outMsg").value_or("");
        return true;
    }

    return ret;
}

bool CameraHalProxy::isRegisteredClient(int devhandle)
{
    PMLOG_INFO(CONST_MODULE_CHP, "handle %d", devhandle);

    json jin;
    jin["devHandle"] = devhandle;

    DEVICE_RETURN_CODE_T ret = luna_call_sync(__func__, to_string(jin));
    return (ret == DEVICE_OK) ? true : false;
}

void CameraHalProxy::requestPreviewCancel()
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    // TBD
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
        for (auto s : j["solutions"])
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
        for (auto s : j["solutions"])
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
    jin["solutions"] = json::array();
    for (auto s : solutions)
    {
        jin["solutions"].push_back(s);
    }

    return luna_call_sync(__func__, to_string(jin));
}

DEVICE_RETURN_CODE_T CameraHalProxy::disableCameraSolution(const std::vector<std::string> solutions)
{
    PMLOG_INFO(CONST_MODULE_CHP, "");

    json jin;
    jin["solutions"] = json::array();
    for (auto s : solutions)
    {
        jin["solutions"].push_back(s);
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
        if ((*jin).is_discarded())
        {
            PMLOG_ERROR(CONST_MODULE_CHP, "resp parsing error!");
            return DEVICE_ERROR_UNKNOWN;
        }
        ret = get_optional<bool>(*jin, "returnValue").value_or(false);
    }
    else
    {
        json j = json::parse(resp, nullptr, false);
        if (j.is_discarded())
        {
            PMLOG_ERROR(CONST_MODULE_CHP, "resp parsing error!");
            return DEVICE_ERROR_UNKNOWN;
        }
        ret = get_optional<bool>(j, "returnValue").value_or(false);
    }

    PMLOG_INFO(CONST_MODULE_CHP, "returnValue : %d", ret);
    return ret ? DEVICE_OK : DEVICE_ERROR_UNKNOWN;
}
