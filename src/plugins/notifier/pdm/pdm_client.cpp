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
#include <glib.h>
#include <list>
#include <nlohmann/json.hpp>
#include <optional>

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

    std::vector<DEVICE_LIST_T> devList;
    for (auto jDevice : jPayload["videoDeviceList"])
    {
        VideoDevice device          = jDevice;
        unsigned int subdeviceCount = device.subDeviceList.size();
        for (auto subdevice : device.subDeviceList)
        {
            DEVICE_LIST_T devInfo;
            devInfo               = device.devInfo;
            devInfo.strDeviceNode = subdevice->devPath;
            devInfo.strDeviceType = "v4l2";
            devInfo.strUserData   = "";

            devInfo.strDeviceKey += "/" + devInfo.strVendorID + "/" + devInfo.strProductID;
            if (subdeviceCount > 1)
            {
                devInfo.strDeviceKey += devInfo.strDeviceNode;
                // NOTE : It is not perfect. strDeviceNode can be changed if another camera is
                // plugged or unplugged while the TV is off.
            }

            PMLOG_INFO(CONST_MODULE_PC, "Vendor ID,Name  : %s, %s", devInfo.strVendorID.c_str(),
                       devInfo.strVendorName.c_str());
            PMLOG_INFO(CONST_MODULE_PC, "Product ID,Name : %s, %s", devInfo.strProductID.c_str(),
                       devInfo.strProductName.c_str());
            PMLOG_INFO(CONST_MODULE_PC, "strDeviceKey    : %s", devInfo.strDeviceKey.c_str());
            PMLOG_INFO(CONST_MODULE_PC, "strDeviceNode   : %s", devInfo.strDeviceNode.c_str());

            devList.push_back(devInfo);
        }
    }

    DeviceManager::getInstance().updateDeviceList("v4l2", devList);

    if (NULL != client->subscribeToDeviceInfoCb_)
        client->subscribeToDeviceInfoCb_(nullptr);

    return true;
}

void PDMClient::subscribeToClient(handlercb cb, GMainLoop *loop)
{
    LSError lsregistererror;
    LSErrorInit(&lsregistererror);

    // register to PDM luna service with cb to be called
    subscribeToDeviceInfoCb_ = cb;

    bool result = LSRegisterServerStatusEx(lshandle_, "com.webos.service.pdm",
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
#ifdef PLATFORM_OSE
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
