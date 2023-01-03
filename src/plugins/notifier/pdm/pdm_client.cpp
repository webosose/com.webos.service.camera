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
#include "whitelist_checker.h"
#include <glib.h>
#include <pbnjson.hpp>

// TODO : Add to PDMClient member as std::vector<DEVICE_LIST_T>
DEVICE_LIST_T dev_info_[MAX_DEVICE_COUNT];

static bool deviceStateCb(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
    jerror *error       = NULL;
    const char *payload = LSMessageGetPayload(message);
    PMLOG_INFO(CONST_MODULE_PC, "payload : %s \n", payload);
    jvalue_ref jin_obj = jdom_create(j_cstr_to_buffer(payload), jschema_all(), &error);

    if (jis_valid(jin_obj))
    {
        bool retvalue;
        unsigned int camcount = 0;
        jboolean_get(jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_RETURNVALUE)), &retvalue);
        PMLOG_INFO(CONST_MODULE_PC, "retvalue : %d \n", retvalue);

        PDMClient *client = (PDMClient *)user_data;

        if (retvalue)
        {
            jvalue_ref jin_obj_devlistinfo;
            bool is_devlistinfo = jobject_get_exists(
                jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_LIST_INFO), &jin_obj_devlistinfo);
            PMLOG_INFO(CONST_MODULE_PC, "Check for deviceListInfo in the received JSON - %d",
                       is_devlistinfo);
            int devlistsize = 0, devlistindex = 0;
            if (is_devlistinfo)
            {
                jin_obj_devlistinfo =
                    jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_LIST_INFO));
                devlistsize = jarray_size(jin_obj_devlistinfo);
                PMLOG_INFO(CONST_MODULE_PC, "devlist array size  : %d \n", devlistsize);
            }

            do
            {
                jvalue_ref jin_obj_child;
                if (is_devlistinfo)
                {
                    jvalue_ref jin_array_devinfo_obj =
                        jarray_get(jin_obj_devlistinfo, devlistindex++);
                    jin_obj_child = jobject_get(jin_array_devinfo_obj,
                                                J_CSTR_TO_BUF(CONST_PARAM_NAME_VIDEO_DEVICE_LIST));
                }
                else
                    jin_obj_child =
                        jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_VIDEO_DEVICE_LIST));

                int listcount = jarray_size(jin_obj_child);
                PMLOG_INFO(CONST_MODULE_PC, "listcount : %d \n", listcount);

                for (int i = 0; i < listcount; i++)
                {
                    jvalue_ref jin_array_obj = jarray_get(jin_obj_child, i);

                    // deviceType
                    jvalue_ref jin_obj_devicetype;
                    if (!jobject_get_exists(jin_array_obj,
                                            J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_TYPE),
                                            &jin_obj_devicetype))
                        continue;
                    raw_buffer device_type     = jstring_get_fast(jin_obj_devicetype);
                    std::string str_devicetype = device_type.m_str;
                    PMLOG_INFO(CONST_MODULE_PC, "str_devicetype : %s \n", str_devicetype.c_str());
                    if (str_devicetype != cstr_cam)
                        continue;

                    // vendorName
                    jvalue_ref jin_obj_vendorname;
                    if (!jobject_get_exists(jin_array_obj,
                                            J_CSTR_TO_BUF(CONST_PARAM_NAME_VENDOR_NAME),
                                            &jin_obj_vendorname))
                        continue;
                    raw_buffer vendor_name     = jstring_get_fast(jin_obj_vendorname);
                    std::string str_vendorname = vendor_name.m_str;
                    PMLOG_INFO(CONST_MODULE_PC, "str_vendorname : %s \n", str_vendorname.c_str());

                    // productName
                    jvalue_ref jin_obj_productname;
                    if (!jobject_get_exists(jin_array_obj,
                                            J_CSTR_TO_BUF(CONST_PARAM_NAME_PRODUCT_NAME),
                                            &jin_obj_productname))
                        continue;
                    raw_buffer product_name     = jstring_get_fast(jin_obj_productname);
                    std::string str_productname = product_name.m_str;
                    PMLOG_INFO(CONST_MODULE_PC, "str_productname : %s \n", str_productname.c_str());

                    // vendorID
                    jvalue_ref jin_obj_vendorid;
                    std::string str_vendorid;
                    if (jobject_get_exists(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_VENDOR_ID),
                                           &jin_obj_vendorid))
                    {
                        raw_buffer vendor_id = jstring_get_fast(jin_obj_vendorid);
                        str_vendorid         = vendor_id.m_str ? vendor_id.m_str : "";
                    }
                    PMLOG_INFO(CONST_MODULE_PC, "str_vendorid : %s \n", str_vendorid.c_str());

                    // productID
                    jvalue_ref jin_obj_productid;
                    std::string str_productid;
                    if (jobject_get_exists(jin_array_obj,
                                           J_CSTR_TO_BUF(CONST_PARAM_NAME_PRODUCT_ID),
                                           &jin_obj_productid))
                    {
                        raw_buffer product_id = jstring_get_fast(jin_obj_productid);
                        str_productid         = product_id.m_str ? product_id.m_str : "";
                    }
                    PMLOG_INFO(CONST_MODULE_PC, "str_productid : %s \n", str_productid.c_str());

                    // USB port number
                    int32_t port_num = 0;
                    jvalue_ref jin_obj_usb_port_num;
                    if (jobject_get_exists(jin_array_obj,
                                           J_CSTR_TO_BUF(CONST_PARAM_NAME_USB_PORT_NUM),
                                           &jin_obj_usb_port_num))
                    {
                        jnumber_get_i32(jin_obj_usb_port_num, &port_num);
                        PMLOG_INFO(CONST_MODULE_PC, "portNum : %d \n", port_num);
                    }

                    // host controller interface
                    jvalue_ref jin_obj_host_controller_inf;
                    std::string str_host_controller_inf;
                    if (jobject_get_exists(
                            jin_array_obj,
                            J_CSTR_TO_BUF(CONST_PARAM_NAME_HOST_CONTROLLER_INTERFACE),
                            &jin_obj_host_controller_inf))
                    {
                        raw_buffer host_controller_inf =
                            jstring_get_fast(jin_obj_host_controller_inf);
                        str_host_controller_inf =
                            host_controller_inf.m_str ? host_controller_inf.m_str : "";
                        PMLOG_INFO(CONST_MODULE_PC, "str_host_controller_inf : %s \n",
                                   str_host_controller_inf.c_str());
                    }

                    // isPowerOnConnect
                    bool isPowerOnConnect = false;
                    jvalue_ref jin_obj_is_poweron_connect;
                    if (jobject_get_exists(jin_array_obj,
                                           J_CSTR_TO_BUF(CONST_PARAM_NAME_IS_POWERON_CONNECT),
                                           &jin_obj_is_poweron_connect))
                    {
                        jboolean_get(jin_obj_is_poweron_connect, &isPowerOnConnect);
                        PMLOG_INFO(CONST_MODULE_PC, "isPowerOnConnect : %d \n", isPowerOnConnect);
                    }

                    // devPathfull "/dev/bus/usb/00x/00x"
                    std::string str_devpath_full;
                    jvalue_ref jin_obj_devPathfull;
                    if (jobject_get_exists(jin_array_obj,
                                           J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_PATH),
                                           &jin_obj_devPathfull))
                    {
                        raw_buffer dev_pathfull = jstring_get_fast(jin_obj_devPathfull);
                        str_devpath_full        = dev_pathfull.m_str ? dev_pathfull.m_str : "";
                        PMLOG_INFO(CONST_MODULE_PC, "str_devpath_full : %s \n",
                                   str_devpath_full.c_str());
                    }

                    // subDeviceList
                    jvalue_ref jin_subdevice_list_obj;
                    int subdevice_count = 0;
                    if (!jobject_get_exists(jin_array_obj,
                                            J_CSTR_TO_BUF(CONST_PARAM_NAME_SUB_DEVICE_LIST),
                                            &jin_subdevice_list_obj))
                        continue;

                    subdevice_count = jarray_size(jin_subdevice_list_obj);
                    for (int j = 0; j < subdevice_count; j++)
                    {
                        jvalue_ref jin_subdevice_obj = jarray_get(jin_subdevice_list_obj, j);

                        jvalue_ref jin_obj_capabilities;
                        bool is_capabilities = jobject_get_exists(
                            jin_subdevice_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_CAPABILITIES),
                            &jin_obj_capabilities);
                        PMLOG_INFO(CONST_MODULE_PC,
                                   "Check for is_capabilities in the received JSON - %d",
                                   is_capabilities);
                        if (!is_capabilities)
                            continue;

                        raw_buffer capabilities      = jstring_get_fast(jin_obj_capabilities);
                        std::string str_capabilities = capabilities.m_str;
                        PMLOG_INFO(CONST_MODULE_PC, "capabilities : %s \n",
                                   str_capabilities.c_str());
                        if (str_capabilities != cstr_capture)
                            continue;

                        jvalue_ref jin_obj_devPath;
                        bool is_devPath = jobject_get_exists(
                            jin_subdevice_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_PATH),
                            &jin_obj_devPath);
                        PMLOG_INFO(CONST_MODULE_PC, "Check for devPath in the received JSON - %d",
                                   is_devPath);
                        if (!is_devPath)
                            continue;

                        raw_buffer devPath      = jstring_get_fast(jin_obj_devPath);
                        std::string str_devPath = devPath.m_str;
                        PMLOG_INFO(CONST_MODULE_PC, "devPath : %s \n", str_devPath.c_str());

                        PMLOG_INFO(CONST_MODULE_PC, "received cam device\n");
                        dev_info_[camcount].strDeviceNode              = str_devPath;
                        dev_info_[camcount].nDeviceNum                 = 0; // TBD
                        dev_info_[camcount].nPortNum                   = port_num;
                        dev_info_[camcount].isPowerOnConnect           = isPowerOnConnect;
                        dev_info_[camcount].strVendorName              = str_vendorname;
                        dev_info_[camcount].strProductName             = str_productname;
                        dev_info_[camcount].strVendorID                = str_vendorid;
                        dev_info_[camcount].strProductID               = str_productid;
                        dev_info_[camcount].strDeviceType              = str_devicetype;
                        dev_info_[camcount].strDeviceSubtype           = str_productname;
                        dev_info_[camcount].strHostControllerInterface = str_host_controller_inf;
                        dev_info_[camcount].strDeviceKey               = str_devpath_full;
                        dev_info_[camcount].strDeviceLabel             = "v4l2";

                        camcount++;
                    }
                }
            } while ((devlistsize > 0) && (devlistindex < devlistsize));

            DEVICE_EVENT_STATE_T nCamEvent = DEVICE_EVENT_NONE;
            DEVICE_EVENT_STATE_T nMicEvent = DEVICE_EVENT_NONE;
            PMLOG_INFO(CONST_MODULE_PC, "camcount : %d \n", camcount);

            DeviceManager::getInstance().updateList(dev_info_, camcount, &nCamEvent, &nMicEvent);

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
        }
        j_release(&jin_obj);

        if (NULL != client->subscribeToDeviceInfoCb_)
            client->subscribeToDeviceInfoCb_(dev_info_);
    }

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
