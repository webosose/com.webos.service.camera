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
#include "addon.h"
#include "camera_constants.h"
#include "device_manager.h"
#include <pbnjson.hpp>

static int remoteCamIdx_{0};

static bool remote_deviceStateCb(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
    PMLOG_INFO(CONST_MODULE_AC, "callback received\n");

    jerror *error       = NULL;
    const char *payload = LSMessageGetPayload(message);

    jvalue_ref jin_obj = jdom_create(j_cstr_to_buffer(payload), jschema_all(), &error);
    if (jis_valid(jin_obj))
    {
        bool retvalue;
        jboolean_get(jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_RETURNVALUE)), &retvalue);

        if (retvalue)
        {
            AppCastClient *client = (AppCastClient *)user_data;

            if (client->mState == INIT)
            {

                jvalue_ref jin_obj_clientKey;
                if (jobject_get_exists(jin_obj, J_CSTR_TO_BUF("clientKey"), &jin_obj_clientKey))
                {
                    raw_buffer clientKey          = jstring_get_fast(jin_obj_clientKey);
                    client->mDeviceInfo.clientKey = clientKey.m_str;
                    PMLOG_INFO(CONST_MODULE_AC, "clientKey : %s \n",
                               client->mDeviceInfo.clientKey.c_str());
                }

                // deviceInfo
                jvalue_ref jin_obj_deviceInfo;
                if (jobject_get_exists(jin_obj, J_CSTR_TO_BUF("deviceInfo"), &jin_obj_deviceInfo))
                {
                    PMLOG_INFO(CONST_MODULE_AC, "===== deviceInfo =====\n");

                    jvalue_ref jin_sub_obj;
                    // version
                    if (jobject_get_exists(jin_obj_deviceInfo, J_CSTR_TO_BUF("version"),
                                           &jin_sub_obj))
                    {
                        raw_buffer name      = jstring_get_fast(jin_sub_obj);
                        std::string str_name = name.m_str;
                        PMLOG_INFO(CONST_MODULE_AC, "version: %s \n", str_name.c_str());
                    }
                    // type
                    if (jobject_get_exists(jin_obj_deviceInfo, J_CSTR_TO_BUF("type"), &jin_sub_obj))
                    {
                        raw_buffer name      = jstring_get_fast(jin_sub_obj);
                        std::string str_name = name.m_str;
                        PMLOG_INFO(CONST_MODULE_AC, "type: %s \n", str_name.c_str());
                    }
                    // platform
                    if (jobject_get_exists(jin_obj_deviceInfo, J_CSTR_TO_BUF("platform"),
                                           &jin_sub_obj))
                    {
                        raw_buffer name      = jstring_get_fast(jin_sub_obj);
                        std::string str_name = name.m_str;
                        PMLOG_INFO(CONST_MODULE_AC, "platform: %s \n", str_name.c_str());
                    }
                    // manufacturer
                    if (jobject_get_exists(jin_obj_deviceInfo, J_CSTR_TO_BUF("manufacturer"),
                                           &jin_sub_obj))
                    {
                        raw_buffer name                  = jstring_get_fast(jin_sub_obj);
                        client->mDeviceInfo.manufacturer = name.m_str;
                        PMLOG_INFO(CONST_MODULE_AC, "manufacturer: %s \n",
                                   client->mDeviceInfo.manufacturer.c_str());
                    }
                    // modelName
                    if (jobject_get_exists(jin_obj_deviceInfo, J_CSTR_TO_BUF("modelName"),
                                           &jin_sub_obj))
                    {
                        raw_buffer name               = jstring_get_fast(jin_sub_obj);
                        client->mDeviceInfo.modelName = name.m_str;
                        PMLOG_INFO(CONST_MODULE_AC, "modelName: %s \n",
                                   client->mDeviceInfo.modelName.c_str());
                    }
                    // deviceName
                    if (jobject_get_exists(jin_obj_deviceInfo, J_CSTR_TO_BUF("deviceName"),
                                           &jin_sub_obj))
                    {
                        raw_buffer name                = jstring_get_fast(jin_sub_obj);
                        client->mDeviceInfo.deviceName = name.m_str;
                        PMLOG_INFO(CONST_MODULE_AC, "deviceName: %s \n",
                                   client->mDeviceInfo.deviceName.c_str());
                    }
                }
            }

            jvalue_ref jin_obj_camera;
            if (jobject_get_exists(jin_obj, J_CSTR_TO_BUF("camera"), &jin_obj_camera))
            {
                // crypto
                jvalue_ref jin_obj_crypto;
                if (jobject_get_exists(jin_obj_camera, J_CSTR_TO_BUF("crypto"), &jin_obj_crypto))
                {
                    PMLOG_INFO(CONST_MODULE_AC, "===== crypto =====\n");
                    if (client->mState == INIT)
                    {
                        PMLOG_INFO(CONST_MODULE_AC, "add camera\n");

                        client->mDeviceInfo.deviceLabel = "remote";
                        remoteCamIdx_ = DeviceManager::getInstance().addRemoteCamera(&client->mDeviceInfo);
                        client->sendConnectSoundInput(true);
                        client->sendSetSoundInput(true);
                        client->setState(READY);

                        if (AddOn::hasImplementation())
                        {
                            DEVICE_LIST_T devList      = {};
                            std::string strProductName = "ThinQ WebCam";
                            if (client->mDeviceInfo.modelName.empty() == false)
                                strProductName = client->mDeviceInfo.modelName;

                            devList.strProductName = strProductName;
                            devList.strDeviceLabel = "remote";
                            AddOn::setDeviceEvent(&devList, 1, true, true);
                        }

                        // Save connect payload of AppCastClient
                        client->connect_payload = payload;
                    }
                }
            }
            else
            {
                PMLOG_INFO(CONST_MODULE_AC, "remove camera\n");
                DeviceManager::getInstance().removeRemoteCamera(remoteCamIdx_);
                client->sendConnectSoundInput(false);
                client->setState(INIT);

                if (AddOn::hasImplementation())
                {
                    AddOn::setDeviceEvent(nullptr, 0, true, true);
                }
            }
        }
        j_release(&jin_obj);
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

    DeviceManager::getInstance().set_appcastclient(this);
}

void AppCastClient::subscribeToClient(GMainLoop *loop)
{
    LSError lsregistererror;
    LSErrorInit(&lsregistererror);

    // register to appcast luna service
    bool result = LSRegisterServerStatusEx(lshandle_, "com.webos.service.appcasting",
                                           subscribeToAppcastService, this, NULL, &lsregistererror);
    if (!result)
    {
        PMLOG_INFO(CONST_MODULE_AC, "LSRegister Server Status failed");
    }
}

void AppCastClient::setLSHandle(LSHandle *handle) { lshandle_ = handle; }

bool AppCastClient::subscribeToAppcastService(LSHandle *sh, const char *serviceName, bool connected,
                                              void *ctx)
{
    int retval = 0;
    int ret    = 0;

    PMLOG_INFO(CONST_MODULE_AC, "connected status:%d \n", connected);
    if (connected)
    {
        // Check sanity
        if (!sh)
        {
            PMLOG_ERROR(CONST_MODULE_AC, "%s LSHandle is null", __func__);
            return ret;
        }

        LSError lserror;
        LSErrorInit(&lserror);

        // get camera service handle and register cb function with appcast
        retval = LSCall(sh, "luna://com.webos.service.appcasting/getMediaInfo",
                        "{\"subscribe\":true}", remote_deviceStateCb, ctx, NULL, &lserror);
        if (!retval)
        {
            PMLOG_ERROR(CONST_MODULE_AC, "%s appcast client uUnable to unregister service",
                        __func__);
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
        PMLOG_INFO(CONST_MODULE_AC, "connected value is false");
    }

    return ret;
}

void AppCastClient::setState(APP_CAST_STATE new_state)
{
    mState = new_state;
    PMLOG_INFO(CONST_MODULE_AC, "setState : mState = %s (%d)", str_state[mState].c_str(), mState);
}

static bool sendConnectSoundInputCallback(LSHandle *sh, LSMessage *msg, void *ctx)
{
    const char *payload = LSMessageGetPayload(msg);
    PMLOG_INFO(CONST_MODULE_AC, "payload : %s\n", payload);

    jerror *error      = NULL;
    jvalue_ref jin_obj = jdom_create(j_cstr_to_buffer(payload), jschema_all(), &error);
    if (jis_valid(jin_obj))
    {
        bool retvalue;
        jboolean_get(jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_RETURNVALUE)), &retvalue);
        PMLOG_INFO(CONST_MODULE_AC, "retvalue : %d \n", retvalue);

        if (retvalue)
        {
        }
    }
    j_release(&jin_obj);
    return true;
}

bool AppCastClient::sendConnectSoundInput(bool connected)
{
    PMLOG_INFO(CONST_MODULE_AC, "connected=%d", connected);

    bool retval;
    LSError lserror;
    LSErrorInit(&lserror);

    pbnjson::JValue args = pbnjson::Object();
    args.put("soundInput", "remote.monitor");
    args.put("connect", connected);

    std::string uri = "luna://com.webos.service.audio/connectSoundInput";
    PMLOG_INFO(CONST_MODULE_AC, "%s '%s'\n", uri.c_str(), args.stringify().c_str());
    retval = LSCall(lshandle_, uri.c_str(), args.stringify().c_str(), sendConnectSoundInputCallback,
                    NULL, NULL, &lserror);
    if (!retval)
    {
        PMLOG_ERROR(CONST_MODULE_AC, "%s error", __func__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return retval;
}

bool AppCastClient::sendSetSoundInput(bool remote)
{
    PMLOG_INFO(CONST_MODULE_AC, "remote=%d", remote);

    bool retval;
    LSError lserror;
    LSErrorInit(&lserror);

    pbnjson::JValue args = pbnjson::Object();
    args.put("soundInput", "remote.monitor");

    std::string uri = "luna://com.webos.service.audio/setSoundInput";
    PMLOG_INFO(CONST_MODULE_AC, "%s '%s'\n", uri.c_str(), args.stringify().c_str());
    retval = LSCall(lshandle_, uri.c_str(), args.stringify().c_str(), sendConnectSoundInputCallback,
                    NULL, NULL, &lserror);
    if (!retval)
    {
        PMLOG_ERROR(CONST_MODULE_AC, "%s error", __func__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return retval;
}
