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

#define LOG_CONTEXT "notifier.pdm"
#define LOG_TAG "PDMClient"
#include "pdm_client.h"
#include "camera_device_types.h"
#include "camera_log.h"
#include "json_utils.h"
#include "plugin.hpp"
#include <glib.h>
#include <list>
#include <nlohmann/json.hpp>
#include <optional>

#define LUNA_CALLBACK(NAME)                                                                        \
    +[](const char *m, void *c) -> bool { return ((PDMClient *)c)->NAME(m); }
#define REGISTER_CALLBACK(NAME)                                                                    \
    +[](const char *m, bool b, void *c) -> bool { return ((PDMClient *)c)->NAME(m, b); }

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
    v.devInfo.strDeviceType  = get_optional<std::string>(j, "deviceType").value_or("");
    v.devInfo.strHostControllerInterface =
        get_optional<std::string>(j, "hostControllerInterface").value_or("");
    v.devInfo.isPowerOnConnect = get_optional<bool>(j, "isPowerOnConnect").value_or(false);
    v.devInfo.strDeviceKey     = get_optional<std::string>(j, "devPath").value_or("");

    if (j.contains("subDeviceList") && j["subDeviceList"].is_array())
    {
        for (auto jsub : j["subDeviceList"])
        {
            if (jsub.contains("capabilities") && jsub["capabilities"] == ":capture:")
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

PDMClient::PDMClient() { PLOGI(""); }

PDMClient::~PDMClient() { PLOGI(""); }

void PDMClient::subscribeToClient(handlercb cb, void *mainLoop)
{
    PLOGI("");
    this->updateDeviceList = cb;

    if (!lunaClient_)
        return;

    // register to PDM luna service with cb to be called
    PLOGI("registerToService : com.webos.service.pdm");
    lunaClient_->registerToService("com.webos.service.pdm",
                                   REGISTER_CALLBACK(registerToServiceCallback), this);
}

void PDMClient::setLSHandle(void *handle)
{
    PLOGI("");
    lunaClient_ = std::make_unique<LunaClient>(static_cast<LSHandle *>(handle));
}

bool PDMClient::registerToServiceCallback(const char *serviceName, bool connected)
{
    bool retVal = true;

    PLOGI("connected status:%d \n", connected);
    if (!lunaClient_)
    {
        PLOGE("lunaClient_ is nullptr!");
        return false;
    }

    if (connected)
    {
        json jpayload;
        jpayload["subscribe"] = true;
        jpayload["category"]  = "Video";
        // In case of OSE, remove the comment below.
        // jpayload["groupSubDevices"] = true;

        // get camera service handle and register cb function with pdm
        retVal = lunaClient_->subscribe(
            "luna://com.webos.service.pdm/getAttachedNonStorageDeviceList", jpayload.dump().c_str(),
            &subscribeKey_, LUNA_CALLBACK(getDeviceListCallback), this);
        if (!retVal)
        {
            PLOGE("%s appcast client uUnable to unregister service", __func__);
        }
    }
    else
    {
        if (subscribeKey_ != 0UL)
        {
            PLOGI("Unsubscribe to the %s service", serviceName);
            lunaClient_->unsubscribe(subscribeKey_);
            subscribeKey_ = 0UL;
        }
    }

    return retVal;
}

bool PDMClient::getDeviceListCallback(const char *message)
{
    PLOGI("payload : %s", message);

    json jPayload = json::parse(message, nullptr, false);
    if (jPayload.is_discarded())
    {
        PLOGI("payload parsing fail!");
        return false;
    }

    PdmResponse pdm = jPayload;
    if (!pdm.returnValue)
    {
        PLOGI("retvalue fail!");
        return false;
    }
    if (!pdm.videoDeviceList)
    {
        PLOGI("deviceListInfo empty!");
        return false;
    }

    std::vector<DEVICE_LIST_T> devList;
    for (auto &jDevice : jPayload["videoDeviceList"])
    {
        const VideoDevice &device = jDevice;

        if (device.devInfo.strDeviceType.empty())
            continue;

        unsigned long subdeviceCount = device.subDeviceList.size();
        for (auto &subdevice : device.subDeviceList)
        {
            if (subdevice->devPath.find("/dev/video") == std::string::npos)
                continue;
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

            PLOGI("Vendor ID,Name  : %s, %s", devInfo.strVendorID.c_str(),
                  devInfo.strVendorName.c_str());
            PLOGI("Product ID,Name : %s, %s", devInfo.strProductID.c_str(),
                  devInfo.strProductName.c_str());
            PLOGI("strDeviceKey    : %s", devInfo.strDeviceKey.c_str());
            PLOGI("strDeviceNode   : %s", devInfo.strDeviceNode.c_str());

            devList.push_back(devInfo);
        }
    }

    if (NULL != updateDeviceList)
        updateDeviceList("v4l2", &devList);

    return true;
}

extern "C"
{
    IPlugin *plugin_init(void)
    {
        Plugin *plg = new Plugin();
        plg->setName("PDM Notifier");
        plg->setDescription("PDM Client for Camera Notifier");
        plg->setCategory("NOTIFIER");
        plg->setVersion("1.0.0");
        plg->setOrganization("LG Electronics.");
        plg->registerFeature<PDMClient>("pdm");

        return plg;
    }

    void __attribute__((constructor)) plugin_load(void)
    {
        printf("%s:%s\n", __FILENAME__, __PRETTY_FUNCTION__);
    }

    void __attribute__((destructor)) plugin_unload(void)
    {
        printf("%s:%s\n", __FILENAME__, __PRETTY_FUNCTION__);
    }
}
