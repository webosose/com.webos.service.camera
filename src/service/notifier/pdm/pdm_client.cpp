// Copyright (c) 2019-2023 LG Electronics, Inc.
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
#include "constants.h"
#include "device_manager.h"
#include <glib.h>
#include <pbnjson.hpp>
#include "whitelist_checker.h"
#include "event_notification.h"

DEVICE_LIST_T dev_info_[MAX_DEVICE_COUNT];

void getDevPaths(jvalue_ref &array_obj, std::vector<std::string> &devPaths)
{
    // subDeviceList
    jvalue_ref jin_subdevice_list_obj;
    jvalue_ref jin_obj_devPath;
    raw_buffer devPath;
    if (jobject_get_exists(array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_SUB_DEVICE_LIST), &jin_subdevice_list_obj))
    {
        int subdevice_count = 0;
        subdevice_count = jarray_size(jin_subdevice_list_obj);
        for (int i = 0; i < subdevice_count; i++)
        {
            jvalue_ref jin_subdevice_obj = jarray_get(jin_subdevice_list_obj, i);

            jvalue_ref jin_obj_capabilities;
            bool is_capabilities = jobject_get_exists(jin_subdevice_obj,
                                                      J_CSTR_TO_BUF(CONST_PARAM_NAME_CAPABILITIES),
                                                      &jin_obj_capabilities);
            PMLOG_INFO(CONST_MODULE_PDMCLIENT, "Check for is_capabilities in the received JSON - %d", is_capabilities);
            if (!is_capabilities)
            {
                continue;
            }
            raw_buffer capabilities = jstring_get_fast(jin_obj_capabilities);
            std::string str_capabilities = capabilities.m_str;
            PMLOG_INFO(CONST_MODULE_PDMCLIENT, "capabilities : %s \n", str_capabilities.c_str());
            if (str_capabilities != cstr_capture)
            {
                continue;
            }
            bool is_devPath = jobject_get_exists(jin_subdevice_obj,
                                                 J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_PATH),
                                                 &jin_obj_devPath);
            PMLOG_INFO(CONST_MODULE_PDMCLIENT, "Check for devPath in the received JSON - %d", is_devPath);
            if (!is_devPath)
            {
                continue;
            }
            devPath = jstring_get_fast(jin_obj_devPath);
            devPaths.push_back(devPath.m_str);
        }
    }
    else
    {
        jvalue_ref jin_obj_kernel;
        if (jobject_get_exists(array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_KERNEL), &jin_obj_kernel))
        {
            devPath = jstring_get_fast(jin_obj_kernel);
            devPaths.push_back(devPath.m_str);
        }
    }
}

static bool deviceStateCb(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
    jerror *error = NULL;
    const char *payload = LSMessageGetPayload(message);
    PMLOG_INFO(CONST_MODULE_PDMCLIENT, "payload : %s \n", payload);
    jvalue_ref jin_obj = jdom_create(j_cstr_to_buffer(payload), jschema_all(), &error);

    if (jis_valid(jin_obj))
    {
        bool retvalue;
        unsigned int camcount = 0;
        jboolean_get(jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_RETURNVALUE)), &retvalue);
        PMLOG_INFO(CONST_MODULE_PDMCLIENT, "retvalue : %d \n", retvalue);
        if (retvalue)
        {
            jvalue_ref jin_obj_devlistinfo;
            bool is_devlistinfo = jobject_get_exists(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_LIST_INFO), &jin_obj_devlistinfo);
            PMLOG_INFO(CONST_MODULE_PDMCLIENT, "Check for deviceListInfo in the received JSON - %d", is_devlistinfo);
            int devlistsize = 0, devlistindex = 0;
            if (is_devlistinfo)
            {
                jin_obj_devlistinfo = jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_LIST_INFO));
                devlistsize = jarray_size(jin_obj_devlistinfo);
                PMLOG_INFO(CONST_MODULE_PDMCLIENT, "devlist array size  : %d \n", devlistsize);
            }

            do
            {
                jvalue_ref jin_obj_child;
                if (is_devlistinfo)
                {
                    jvalue_ref jin_array_devinfo_obj = jarray_get(jin_obj_devlistinfo, devlistindex++);
                    jin_obj_child = jobject_get(jin_array_devinfo_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_VIDEO_DEVICE_LIST));
                }
                else
                {
                    jin_obj_child = jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_VIDEO_DEVICE_LIST));
                }
                int listcount = jarray_size(jin_obj_child);
                PMLOG_INFO(CONST_MODULE_PDMCLIENT, "listcount : %d \n", listcount);

                for (int i = 0; i < listcount; i++)
                {
                    jvalue_ref jin_array_obj = jarray_get(jin_obj_child, i);

                    // deviceType
                    jvalue_ref jin_obj_devicetype;
                    std::string str_devicetype;
                    if (jobject_get_exists(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_TYPE), &jin_obj_devicetype))
                    {
                        raw_buffer device_type = jstring_get_fast(jin_obj_devicetype);
                        str_devicetype = (device_type.m_str) ? device_type.m_str : "";
                    }
                    else
                    {
                        jvalue_ref jin_obj_subsystem;
                        if (jobject_get_exists(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_SUBSYSTEM), &jin_obj_subsystem))
                        {
                            raw_buffer subsystem = jstring_get_fast(jin_obj_subsystem);
                            if (strcmp(subsystem.m_str, "video4linux") == 0)
                            {
                                str_devicetype = cstr_cam;
                            }
                        }
                    }
                    PMLOG_INFO(CONST_MODULE_PDMCLIENT, "str_devicetype : %s \n", str_devicetype.c_str());
                    if (str_devicetype != cstr_cam)
                    {
                        continue;
                    }
                    // vendorName
                    jvalue_ref jin_obj_vendorname;
                    if (!jobject_get_exists(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_VENDOR_NAME), &jin_obj_vendorname))
                    {
                        continue;
                    }

                    raw_buffer vendor_name = jstring_get_fast(jin_obj_vendorname);
                    std::string str_vendorname = (vendor_name.m_str) ? vendor_name.m_str : "";
                    PMLOG_INFO(CONST_MODULE_PDMCLIENT, "str_vendorname : %s \n", str_vendorname.c_str());

                    // productName
                    jvalue_ref jin_obj_productname;
                    if (!jobject_get_exists(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PRODUCT_NAME), &jin_obj_productname))
                        continue;
                    raw_buffer product_name = jstring_get_fast(jin_obj_productname);
                    std::string str_productname = (product_name.m_str) ? product_name.m_str : "";
                    PMLOG_INFO(CONST_MODULE_PDMCLIENT, "str_productname : %s \n", str_productname.c_str());

                    // vendorID
                    jvalue_ref jin_obj_vendorid;
                    std::string str_vendorid;
                    if (jobject_get_exists(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_VENDOR_ID), &jin_obj_vendorid))
                    {
                        raw_buffer vendor_id = jstring_get_fast(jin_obj_vendorid);
                        str_vendorid = (vendor_id.m_str) ? vendor_id.m_str : "";
                    }
                    PMLOG_INFO(CONST_MODULE_PDMCLIENT, "str_vendorid : %s \n", str_vendorid.c_str());

                    // productID
                    jvalue_ref jin_obj_productid;
                    std::string str_productid;
                    if (jobject_get_exists(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PRODUCT_ID), &jin_obj_productid))
                    {
                        raw_buffer product_id = jstring_get_fast(jin_obj_productid);
                        str_productid = (product_id.m_str) ? product_id.m_str : "";
                    }
                    PMLOG_INFO(CONST_MODULE_PDMCLIENT, "str_productid : %s \n", str_productid.c_str());

                    // devPath
                    std::vector<std::string> str_devPaths;
                    getDevPaths(jin_array_obj, str_devPaths);
                    for (auto str_devPath : str_devPaths)
                    {
                        PMLOG_INFO(CONST_MODULE_PDMCLIENT, "devPath : %s \n", str_devPath.c_str());
                        strncpy(dev_info_[camcount].strDeviceNode, str_devPath.c_str(), CONST_MAX_STRING_LENGTH - 1);

                        PMLOG_INFO(CONST_MODULE_PDMCLIENT, "received cam device\n");
                        dev_info_[camcount].nDeviceNum = 0;           // TBD
                        dev_info_[camcount].nPortNum = 0;             // TBD
                        dev_info_[camcount].isPowerOnConnect = false; // TBD

                        strncpy(dev_info_[camcount].strVendorName, str_vendorname.c_str(),
                                CONST_MAX_STRING_LENGTH - 1);
                        dev_info_[camcount].strVendorName[CONST_MAX_STRING_LENGTH - 1] = '\0';

                        strncpy(dev_info_[camcount].strProductName, str_productname.c_str(),
                                CONST_MAX_STRING_LENGTH - 1);
                        dev_info_[camcount].strProductName[CONST_MAX_STRING_LENGTH - 1] = '\0';

                        strncpy(dev_info_[camcount].strVendorID, str_vendorid.c_str(),
                                CONST_MAX_STRING_LENGTH - 1);
                        dev_info_[camcount].strVendorID[CONST_MAX_STRING_LENGTH - 1] = '\0';

                        strncpy(dev_info_[camcount].strProductID, str_productid.c_str(),
                                CONST_MAX_STRING_LENGTH - 1);
                        dev_info_[camcount].strProductID[CONST_MAX_STRING_LENGTH - 1] = '\0';

                        strncpy(dev_info_[camcount].strDeviceType, str_devicetype.c_str(),
                                CONST_MAX_STRING_LENGTH - 1);
                        dev_info_[camcount].strDeviceType[CONST_MAX_STRING_LENGTH - 1] = '\0';

                        strncpy(dev_info_[camcount].strDeviceSubtype, str_productname.c_str(), // TBD
                                CONST_MAX_STRING_LENGTH - 1);
                        dev_info_[camcount].strDeviceSubtype[CONST_MAX_STRING_LENGTH - 1] = '\0';

                        camcount++;
                    }
                }
            } while ((devlistsize > 0) && (devlistindex < devlistsize));

            DEVICE_EVENT_STATE_T nCamEvent = DEVICE_EVENT_NONE;
            DEVICE_EVENT_STATE_T nMicEvent = DEVICE_EVENT_NONE;
            PMLOG_INFO(CONST_MODULE_PDMCLIENT, "camcount : %d \n", camcount);

            DeviceManager::getInstance().updateList(dev_info_, camcount, &nCamEvent, &nMicEvent);

            if (nCamEvent == DEVICE_EVENT_STATE_PLUGGED)
            {
                PMLOG_INFO(CONST_MODULE_PDMCLIENT, "PLUGGED CamEvent type: %d \n", nCamEvent);
                EventNotification obj;
                obj.eventReply(lsHandle, CONST_EVENT_NOTIFICATION, nullptr, nullptr, EventType::EVENT_TYPE_CONNECT);
            }
            else if (nCamEvent == DEVICE_EVENT_STATE_UNPLUGGED)
            {
                PMLOG_INFO(CONST_MODULE_PDMCLIENT, "UNPLUGGED CamEvent type: %d \n", nCamEvent);
                EventNotification obj;
                obj.eventReply(lsHandle, CONST_EVENT_NOTIFICATION, nullptr, nullptr, EventType::EVENT_TYPE_DISCONNECT);
            }

            // no need of whitelist check if no device found
            if (nCamEvent == DEVICE_EVENT_STATE_PLUGGED && camcount > 0)
            {
                WhitelistChecker::check(dev_info_[camcount - 1].strProductName, dev_info_[camcount - 1].strVendorName);
            }
        }
        j_release(&jin_obj);
    }

    return true;
}

void PDMClient::subscribeToClient(GMainLoop *loop)
{
    LSError lsregistererror;
    LSErrorInit(&lsregistererror);

    // register to PDM luna service with cb to be called
    bool result = LSRegisterServerStatusEx(lshandle_, "com.webos.service.pdm",
                                           subscribeToPdmService, loop, NULL, &lsregistererror);
    if (!result)
    {
        PMLOG_INFO(CONST_MODULE_PDMCLIENT, "LSRegister Server Status failed");
    }
}

void PDMClient::setLSHandle(LSHandle *handle)
{
    lshandle_ = handle;
}

bool PDMClient::subscribeToPdmService(LSHandle *sh,
                                      const char *serviceName,
                                      bool connected,
                                      void *ctx)
{
    int retval = 0;
    int ret = 0;

    LSError lserror;
    LSErrorInit(&lserror);

    PMLOG_INFO(CONST_MODULE_PDMCLIENT, "connected status:%d \n", connected);
    if (connected)
    {
        // get camera service handle and register cb function with pdm
        retval = LSCall(sh, cstr_uri.c_str(), cstr_payload.c_str(), deviceStateCb, NULL, NULL,
                        &lserror);
        if (!retval)
        {
            PMLOG_INFO(CONST_MODULE_PDMCLIENT, "PDM client Unable to unregister service\n");
            ret = -1;
        }
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_PDMCLIENT, "connected value is false");
    }

    if (LSErrorIsSet(&lserror))
    {
        LSErrorPrint(&lserror, stderr);
    }
    LSErrorFree(&lserror);

    return ret;
}
