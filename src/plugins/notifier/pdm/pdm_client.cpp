// Copyright (c) 2019-2020 LG Electronics, Inc.
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

#include "pdm_client.h"
#include "addon.h"
#include "camera_constants.h"
#include "device_manager.h"
#include "event_notification.h"
#include "json_utils.h"
#include "whitelist_checker.h"
#include <glib.h>
#include <list>
#include <nlohmann/json.hpp>
#include <optional>
#include <pbnjson.hpp>

using namespace nlohmann;

struct PdmResponse
{
    bool returnValue;
    bool videoDeviceList;
};

struct SubDevice
{
    std::string productName;
    std::string capabilities;
    std::string version;
    std::string devPath;
};

struct VideoDevice
{
    DEVICE_LIST_T devInfo;
    std::list<std::shared_ptr<SubDevice>> subDeviceList;
};

void from_json(const json &j, PdmResponse &p)
{
    p.returnValue = get_optional<bool>(j, "returnValue").value_or(false);
    if (j.contains("videoDeviceList") && j["videoDeviceList"].is_array())
        p.videoDeviceList = true;
    else
        p.videoDeviceList = false;
}

void from_json(const json &j, VideoDevice &v)
{
    v.devInfo.strVendorName  = get_optional<std::string>(j, "vendorName").value_or("");
    v.devInfo.strProductName = get_optional<std::string>(j, "productName").value_or("");
    v.devInfo.strVendorID    = get_optional<std::string>(j, "vendorID").value_or("");
    v.devInfo.strProductID   = get_optional<std::string>(j, "productID").value_or("");
    v.devInfo.nPortNum       = get_optional<int>(j, "usbPortNum").value_or(0);
    v.devInfo.strHostControllerInterface =
        get_optional<std::string>(j, "hostControllerInterface").value_or("");
    v.devInfo.isPowerOnConnect = get_optional<bool>(j, "isPowerOnConnect").value_or(false);
    v.devInfo.strDeviceKey     = get_optional<std::string>(j, "devPath").value_or("");

    if (j.contains("subDeviceList") && j["subDeviceList"].is_array())
    {
        for (auto jsub : j["subDeviceList"])
        {
            if (jsub.contains("capabilities") && jsub["capabilities"] == cstr_capture)
            {
                auto subDevice = std::make_shared<SubDevice>();
                subDevice->productName =
                    get_optional<std::string>(jsub, "productName").value_or("");
                subDevice->capabilities =
                    get_optional<std::string>(jsub, "capabilities").value_or("");
                subDevice->version = get_optional<std::string>(jsub, "version").value_or("");
                subDevice->devPath = get_optional<std::string>(jsub, "devPath").value_or("");
                v.subDeviceList.push_back(subDevice);
            }
        }
    }
}

// TODO : Add to PDMClient member as std::vector<DEVICE_LIST_T>
DEVICE_LIST_T dev_info_[MAX_DEVICE_COUNT];

static bool deviceStateCb(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
    const char *payload = LSMessageGetPayload(message);
    PDMClient *client   = (PDMClient *)user_data;
    PMLOG_INFO(CONST_MODULE_PC, "payload : %s \n", payload);

    json jPayload = json::parse(payload, nullptr, false);
    if (jPayload.is_discarded())
    {
        PMLOG_INFO(CONST_MODULE_PC, "payload parsing fail!");
        return false;
    }

    PdmResponse pdm = jPayload;
    if (!pdm.returnValue)
    {
        PMLOG_INFO(CONST_MODULE_PC, "retvalue fail!");
        return false;
    }
    if (!pdm.videoDeviceList)
    {
        PMLOG_INFO(CONST_MODULE_PC, "deviceListInfo empty!");
        return false;
    }

    unsigned int camcount = 0;
    std::vector<std::string> newNodeList;
    for (auto jDevice : jPayload["videoDeviceList"])
    {
        VideoDevice device = jDevice;
        for (auto subdevice : device.subDeviceList)
        {
            dev_info_[camcount]               = device.devInfo;
            dev_info_[camcount].strDeviceNode = subdevice->devPath;
            dev_info_[camcount].strDeviceType = "v4l2";

            PMLOG_INFO(CONST_MODULE_PC, "Vendor ID,Name  : %s, %s",
                       dev_info_[camcount].strVendorID.c_str(),
                       dev_info_[camcount].strVendorName.c_str());
            PMLOG_INFO(CONST_MODULE_PC, "Product ID,Name : %s, %s",
                       dev_info_[camcount].strProductID.c_str(),
                       dev_info_[camcount].strProductName.c_str());
            PMLOG_INFO(CONST_MODULE_PC, "strDeviceKey    : %s",
                       dev_info_[camcount].strDeviceKey.c_str());
            PMLOG_INFO(CONST_MODULE_PC, "strDeviceNode   : %s",
                       dev_info_[camcount].strDeviceNode.c_str());

            newNodeList.push_back(subdevice->devPath);
            camcount++;
        }
    }

    DEVICE_EVENT_STATE_T nCamEvent = DEVICE_EVENT_NONE;
    std::vector<int> idList;
    DeviceManager::getInstance().getDeviceIdList(idList);
    std::map<std::string, int> curNodeMap;

    for (auto id : idList)
    {
        if (DeviceManager::getInstance().getDeviceType(id) == "v4l2")
        {
            std::string node;
            DeviceManager::getInstance().getDeviceNode(id, node);
            curNodeMap[node] = id;
        }
    }

    PMLOG_INFO(CONST_MODULE_PC, "camcount %d, map.size %zd", camcount, curNodeMap.size());
    if (camcount > (unsigned int)curNodeMap.size()) // add Device
    {
        nCamEvent = DEVICE_EVENT_STATE_PLUGGED;
        for (unsigned int i = 0; i < camcount; i++)
        {
            if (curNodeMap.find(dev_info_[i].strDeviceNode) == curNodeMap.end())
            {
                DeviceManager::getInstance().addDevice(&dev_info_[i]);
            }
        }
    }
    else if (camcount < (unsigned int)curNodeMap.size()) // remove Device
    {
        nCamEvent = DEVICE_EVENT_STATE_UNPLUGGED;
        for (auto it : curNodeMap)
        {
            auto node = find(newNodeList.begin(), newNodeList.end(), it.first);
            if (node == newNodeList.end())
            {
                DeviceManager::getInstance().removeDevice(it.second);
            }
        }
    }

    if (AddOn::hasImplementation())
    {
        AddOn::setDeviceEvent(dev_info_, camcount, AddOn::isResumeDone());
    }

    if (nCamEvent == DEVICE_EVENT_STATE_PLUGGED)
    {
        PMLOG_INFO(CONST_MODULE_PC, "PLUGGED CamEvent type: %d \n", nCamEvent);
        EventNotification obj;
        obj.eventReply(lsHandle, CONST_EVENT_NOTIFICATION, nullptr, nullptr,
                       EventType::EVENT_TYPE_CONNECT);
    }
    else if (nCamEvent == DEVICE_EVENT_STATE_UNPLUGGED)
    {
        PMLOG_INFO(CONST_MODULE_PC, "UNPLUGGED CamEvent type: %d \n", nCamEvent);
        EventNotification obj;
        obj.eventReply(lsHandle, CONST_EVENT_NOTIFICATION, nullptr, nullptr,
                       EventType::EVENT_TYPE_DISCONNECT);
    }

    if (false == AddOn::hasImplementation())
    {
        if (nCamEvent == DEVICE_EVENT_STATE_PLUGGED && camcount)
        {
            WhitelistChecker::check(dev_info_[camcount - 1].strProductName,
                                    dev_info_[camcount - 1].strVendorName);
        }
    }

    if (NULL != client->subscribeToDeviceInfoCb_)
        client->subscribeToDeviceInfoCb_(dev_info_);

    return true;
}

void PDMClient::subscribeToClient(handlercb cb, GMainLoop *loop)
{
    LSError lsregistererror;
    LSErrorInit(&lsregistererror);

    // register to PDM luna service with cb to be called
    subscribeToDeviceInfoCb_ = cb;
    bool result              = LSRegisterServerStatusEx(lshandle_, "com.webos.service.pdm",
                                                        subscribeToPdmService, this, NULL, &lsregistererror);
    if (!result)
    {
        PMLOG_INFO(CONST_MODULE_PC, "LSRegister Server Status failed");
    }
}

void PDMClient::setLSHandle(LSHandle *handle) { lshandle_ = handle; }

bool PDMClient::subscribeToPdmService(LSHandle *sh, const char *serviceName, bool connected,
                                      void *ctx)
{
    int ret = 0;

    PMLOG_INFO(CONST_MODULE_PC, "connected status:%d \n", connected);
    if (connected)
    {
        LSError lserror;
        LSErrorInit(&lserror);

        std::string payload = "{\"subscribe\":true,\"category\":\"Video\"";
#ifdef USE_GROUP_SUB_DEVICES
        payload += ",\"groupSubDevices\":true";
#endif
        payload += "}";

        // get camera service handle and register cb function with pdm
        int retval =
            LSCall(sh, cstr_uri.c_str(), payload.c_str(), deviceStateCb, ctx, NULL, &lserror);

        if (!retval)
        {
            PMLOG_INFO(CONST_MODULE_PC, "PDM client Unable to unregister service\n");
            ret = -1;
        }

        if (LSErrorIsSet(&lserror))
        {
            LSErrorPrint(&lserror, stderr);
        }
        LSErrorFree(&lserror);
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_PC, "connected value is false");
    }

    return ret;
}
