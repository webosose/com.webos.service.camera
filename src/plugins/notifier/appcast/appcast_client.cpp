// Copyright (c) 2022 LG Electronics, Inc.
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

#include "appcast_client.h"
#include "camera_constants.h"
#include "camera_device_types.h"
#include "camera_log.h"
#include "json_utils.h"
#include "plugin.hpp"
#include <nlohmann/json.hpp>

#define LOG_TAG "NOTIFIER:AppCastClient"

using namespace nlohmann;

struct AppcastMessage
{
    deviceInfo_t dev;
    bool crypto;
    bool connect;
    bool returnValue;
};

void from_json(const json &j, AppcastMessage &a)
{
    a.dev.clientKey = get_optional<std::string>(j, "clientKey").value_or("");
    if (j.contains("deviceInfo"))
    {
        auto jdi       = j["deviceInfo"];
        a.dev.version  = get_optional<std::string>(jdi, "version").value_or("");
        a.dev.type     = get_optional<std::string>(jdi, "type").value_or("");
        a.dev.platform = get_optional<std::string>(jdi, "platform").value_or("");
        a.dev.manufacturer =
            get_optional<std::string>(jdi, "manufacturer").value_or("LG Electronics");
        a.dev.modelName  = get_optional<std::string>(jdi, "modelName").value_or("ThinQ WebCam");
        a.dev.deviceName = get_optional<std::string>(jdi, "deviceName").value_or("");
    }
    a.crypto      = false;
    a.connect     = false;
    a.returnValue = false;
    if (j.contains("camera"))
    {
        a.connect = true;
        if (j["camera"].contains("crypto"))
            a.crypto = true;
    }
    a.returnValue = get_optional<bool>(j, "returnValue").value_or(false);
}

static bool remote_deviceStateCb(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
    PMLOG_INFO(LOG_TAG, "callback received");

    const char *payload = LSMessageGetPayload(message);
    if (!payload)
    {
        PMLOG_ERROR(LOG_TAG, "payload is null");
        return false;
    }

    json jdata = json::parse(payload, nullptr, false);
    if (jdata.is_discarded())
    {
        PMLOG_ERROR(LOG_TAG, "payload parsing error!");
        return false;
    }

    AppcastMessage appcast = jdata;
    if (!appcast.returnValue)
    {
        PMLOG_ERROR(LOG_TAG, "returnValue is not true!");
        return false;
    }

    AppCastClient *client = (AppCastClient *)user_data;
    if (!appcast.connect)
    {
        PMLOG_INFO(LOG_TAG, "remove camera");
        if (client->updateDeviceList)
        {
            std::vector<DEVICE_LIST_T> empty{};
            client->updateDeviceList("remote", (const void *)&empty);
        }
        client->sendConnectSoundInput(false);
        client->setState(INIT);
        return true;
    }

    if (client->mState == INIT && appcast.crypto)
    {
        PMLOG_INFO(LOG_TAG, "add camera");
        PMLOG_INFO(LOG_TAG, "clientKey    : %s", appcast.dev.clientKey.c_str());
        PMLOG_INFO(LOG_TAG, "version      : %s", appcast.dev.version.c_str());
        PMLOG_INFO(LOG_TAG, "type         : %s", appcast.dev.type.c_str());
        PMLOG_INFO(LOG_TAG, "platform     : %s", appcast.dev.platform.c_str());
        PMLOG_INFO(LOG_TAG, "manufacturer : %s", appcast.dev.manufacturer.c_str());
        PMLOG_INFO(LOG_TAG, "modelName    : %s", appcast.dev.modelName.c_str());
        PMLOG_INFO(LOG_TAG, "deviceName   : %s", appcast.dev.deviceName.c_str());

        client->mDeviceInfo = appcast.dev;
        DEVICE_LIST_T devInfo;
        devInfo.nDeviceNum                 = 0;
        devInfo.nPortNum                   = 0;
        devInfo.isPowerOnConnect           = true;
        devInfo.strVendorName              = client->mDeviceInfo.manufacturer;
        devInfo.strProductName             = client->mDeviceInfo.modelName;
        devInfo.strVendorID                = "RemoteCamera";
        devInfo.strProductID               = "RemoteCamera";
        devInfo.strDeviceType              = "remote";
        devInfo.strDeviceSubtype           = "IP-CAM JPEG";
        devInfo.strDeviceNode              = "udpsrc=" + client->mDeviceInfo.clientKey;
        devInfo.strHostControllerInterface = "";
        devInfo.strDeviceKey               = client->mDeviceInfo.clientKey;
        devInfo.strUserData                = payload;
        std::vector<DEVICE_LIST_T> devList;
        devList.push_back(devInfo);

        if (NULL != client->updateDeviceList)
            client->updateDeviceList("remote", &devList);

        client->sendConnectSoundInput(true);
        client->sendSetSoundInput(true);
        client->setState(READY);
    }

    return true;
}

AppCastClient::AppCastClient()
{
    lshandle_ = nullptr;
    mState    = INIT;

    str_state[INIT]  = "INIT";
    str_state[READY] = "READY";
    str_state[PLAY]  = "PLAY";
}

AppCastClient::~AppCastClient() {}

void AppCastClient::subscribeToClient(handlercb cb, void *mainLoop)
{
    LSError lsregistererror;
    LSErrorInit(&lsregistererror);

    // register to appcast luna service
    this->updateDeviceList = cb;

    bool result = LSRegisterServerStatusEx(lshandle_, "com.webos.service.appcasting",
                                           subscribeToAppcastService, this, NULL, &lsregistererror);
    if (!result)
    {
        PMLOG_INFO(LOG_TAG, "LSRegister Server Status failed");
    }
}

void AppCastClient::setLSHandle(void *handle) { lshandle_ = (LSHandle *)handle; }

bool AppCastClient::subscribeToAppcastService(LSHandle *sh, const char *serviceName, bool connected,
                                              void *ctx)
{
    int retval = 0;
    int ret    = 0;

    PMLOG_INFO(LOG_TAG, "connected status:%d \n", connected);
    if (connected)
    {
        // Check sanity
        if (!sh)
        {
            PMLOG_ERROR(LOG_TAG, "%s LSHandle is null", __func__);
            return ret;
        }

        LSError lserror;
        LSErrorInit(&lserror);

        // get camera service handle and register cb function with appcast
        retval = LSCall(sh, "luna://com.webos.service.appcasting/getMediaInfo",
                        R"({"subscribe":true})", remote_deviceStateCb, ctx, NULL, &lserror);
        if (!retval)
        {
            PMLOG_ERROR(LOG_TAG, "%s appcast client uUnable to unregister service", __func__);
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
        PMLOG_INFO(LOG_TAG, "connected value is false");
    }

    return ret;
}

void AppCastClient::setState(APP_CAST_STATE new_state)
{
    mState = new_state;
    PMLOG_INFO(LOG_TAG, "setState : mState = %s (%d)",
               str_state[static_cast<unsigned int>(mState)].c_str(), mState);
}

static bool sendConnectSoundInputCallback(LSHandle *sh, LSMessage *msg, void *ctx)
{
    const char *payload = LSMessageGetPayload(msg);
    PMLOG_INFO(LOG_TAG, "payload : %s\n", payload);

    json jdata = json::parse(payload);
    if (jdata.is_discarded())
    {
        PMLOG_ERROR(LOG_TAG, "payload parsing error!");
        return false;
    }

    if (jdata.contains(CONST_PARAM_NAME_RETURNVALUE) &&
        jdata[CONST_PARAM_NAME_RETURNVALUE].is_boolean())
    {
        bool retvalue = jdata[CONST_PARAM_NAME_RETURNVALUE].get<bool>();
        PMLOG_INFO(LOG_TAG, "retvalue : %d", retvalue);
    }

    return true;
}

bool AppCastClient::sendConnectSoundInput(bool connected)
{
    PMLOG_INFO(LOG_TAG, "connected=%d", connected);

    bool retval;
    LSError lserror;
    LSErrorInit(&lserror);

    auto args       = json{{"soundInput", "remote.monitor"}, {"connect", connected}}.dump();
    std::string uri = "luna://com.webos.service.audio/connectSoundInput";
    PMLOG_INFO(LOG_TAG, "%s '%s'\n", uri.c_str(), args.c_str());
    retval = LSCall(lshandle_, uri.c_str(), args.c_str(), sendConnectSoundInputCallback, NULL, NULL,
                    &lserror);
    if (!retval)
    {
        PMLOG_ERROR(LOG_TAG, "%s error", __func__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return retval;
}

bool AppCastClient::sendSetSoundInput(bool remote)
{
    PMLOG_INFO(LOG_TAG, "remote=%d", remote);

    bool retval;
    LSError lserror;
    LSErrorInit(&lserror);

    auto args       = json{{"soundInput", "remote.monitor"}}.dump();
    std::string uri = "luna://com.webos.service.audio/setSoundInput";
    PMLOG_INFO(LOG_TAG, "%s '%s'\n", uri.c_str(), args.c_str());
    retval = LSCall(lshandle_, uri.c_str(), args.c_str(), sendConnectSoundInputCallback, NULL, NULL,
                    &lserror);
    if (!retval)
    {
        PMLOG_ERROR(LOG_TAG, "%s error", __func__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return retval;
}

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
extern "C"
{
    IPlugin *plugin_init(void)
    {
        Plugin *plg = new Plugin();
        plg->setName("AppCast Notifier");
        plg->setDescription("AppCast Client for Camera Notifier");
        plg->setCategory("NOTIFIER");
        plg->setVersion("1.0.0");
        plg->setOrganization("LG Electronics.");
        plg->registerFeature<AppCastClient>("appcast");

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