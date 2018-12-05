// Copyright (c) 2018 LG Electronics, Inc.
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

#include <glib.h>
#include <pbnjson.hpp>
#include "pdm_client.h"
#include "pdm_client_constants.h"

device_info_t dev_info_[MAX_DEVICE_COUNT];
pdmhandlercb subscribeToDeviceInfoCb_;
int devicecount = 0;

static bool deviceStateCb(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
    SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb callback received\n");

    jerror *error = NULL;
    const char *payload = LSMessageGetPayload(message);
    jvalue_ref jin_obj = jdom_create(j_cstr_to_buffer(payload), jschema_all(), &error);

    if (jis_valid(jin_obj))
    {
        bool retvalue;
        jboolean_get(jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_RETURNVALUE)), &retvalue);
        SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb retvalue : %d \n", retvalue);
        if (retvalue)
        {
            jvalue_ref jin_obj_child = jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_NON_STORAGE_DEVICE_LIST));

            int listcount = jarray_size(jin_obj_child);
            SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb listcount : %d \n", listcount);

            int camcount = 0;

            for (int i = 0; i < listcount; i++)
            {
                jvalue_ref jin_array_obj = jarray_get(jin_obj_child, i);
                int portnum = -1;
                jnumber_get_i32(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_USB_PORT_NUM)), &portnum);
                SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb portnum : %d \n", portnum);

                raw_buffer serialnum = jstring_get_fast(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_SERIAL_NUMBER)));
                std::string str_serialnum = serialnum.m_str;
                SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb serialnum : %s \n", serialnum);

                bool b_powerstatus = false;
                jboolean_get(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_IS_POWER_ON_CONNECT)), &b_powerstatus);
                SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb b_powerstatus : %d \n", b_powerstatus);

                int devicenum = -1;
                jnumber_get_i32(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_NUM)), &devicenum);
                SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb devicenum : %d \n", devicenum);

                raw_buffer devsubtype = jstring_get_fast(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_SUBTYPE)));
                std::string str_devicesubtype = devsubtype.m_str;
                SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb str_devicesubtype : %s \n", str_devicesubtype);

                raw_buffer vendor_name = jstring_get_fast(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_VENDOR_NAME)));
                std::string str_vendorname = vendor_name.m_str;
                SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb str_vendorname : %s \n", str_vendorname);

                raw_buffer device_type = jstring_get_fast(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_TYPE)));
                std::string str_devicetype = device_type.m_str;
                SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb str_devicetype : %s \n", str_devicetype);

                raw_buffer product_name = jstring_get_fast(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PRODUCT_NAME)));
                std::string str_productname = product_name.m_str;
                SRV_LOG_INFO(CONST_MODULE_LUNA, "deviceStateCb str_productname : %s \n", str_productname);

                if (cam == str_devicetype)
                {
                    dev_info_[camcount].device_num = devicenum;
                    dev_info_[camcount].port_num = portnum;
                    dev_info_[camcount].vendor_name = str_vendorname;
                    dev_info_[camcount].product_name = str_productname;
                    dev_info_[camcount].serial_number = str_serialnum;
                    dev_info_[camcount].device_type = str_devicetype;
                    dev_info_[camcount].device_subtype = str_devicesubtype;
                    dev_info_[camcount].device_state = DeviceEvent::DEVICE_EVENT_STATE_PLUGGED;

                    camcount++;
                    devicecount = camcount;
                }
                if (camcount >= MAX_DEVICE_COUNT)
                    break;
            }
            if (devicecount > camcount)
                dev_info_[camcount].device_state = DeviceEvent::DEVICE_EVENT_STATE_UNPLUGGED;
        }
        j_release(&jin_obj);

        if (NULL != subscribeToDeviceInfoCb_)
            subscribeToDeviceInfoCb_(dev_info_);
    }

    return true;
}

void PDMClient::subscribeToClient(pdmhandlercb cb)
{
    //register to PDM luna service with cb to be called
    subscribeToDeviceInfoCb_ = cb;

    subscribeToPdmService();
}

void PDMClient::setLSHandle(LSHandle *handle)
{
    lshandle_ = handle;
}

int PDMClient::subscribeToPdmService()
{
    int retval = 0;
    int ret = 0;

    LSError lserror;
    LSErrorInit(&lserror);

    //get camera service handle and register cb function with pdm
    retval = LSCall(lshandle_, uri.c_str(), payload.c_str(), deviceStateCb, NULL, NULL, &lserror);
    if (!retval)
    {
        g_print("PDM client uUnable to unregister service\n");
        ret = -1;
    }

    if (LSErrorIsSet(&lserror))
    {
        LSErrorPrint(&lserror, stderr);
    }
    LSErrorFree(&lserror);

    return ret;
}
