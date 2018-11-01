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

/******************************************************************************
 File Inclusions
 ******************************************************************************/
#include <glib.h>
#include "pdm_client.h"
#include "camera_hal_types.h"
#include "json.h"
#include "pdm_client_constants.h"

camera_info_t dev_info_[MAX_DEVICE_COUNT];
pdmhandler_cb subscribeToDeviceStateCb_;
int devicecount = 0;

static bool deviceStateCb(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
    DLOG_SDK(std::cout << "***********deviceStateCb************" << std::endl;);

    const char *payload;
    struct json_object *in_json = NULL, *in_json_child1 = NULL, *in_json_child2 = NULL,
            *in_json_child3 = NULL;
    // json parsing
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    in_json = json_tokener_parse(payload);

    if (!in_json)
    {
        DLOG_SDK(std::cout << "deviceStateCb : json parsing error" << std::endl;);
    }
    else
    {
        DLOG_SDK(std::cout << "Device List update from PDM" << std::endl;);

        if (json_object_object_get_ex(in_json,returnval.c_str(), &in_json_child1))
        {
            int retvalue = json_object_get_boolean(in_json_child1);

            if (retvalue <= 0)
            {
                DLOG_SDK(std::cout << "PDM is not a good state! SDK will also stop" << std::endl;);
            }
            else
            {
                if (json_object_object_get_ex(in_json,nonstoragedevlist.c_str(), &in_json_child1))
                {
                    int listcount = json_object_array_length(in_json_child1);
                    int camcount = 0;
                    int devicenum = -1,portnum = -1;
                    std::string str_productname,str_vendorname;
                    std::string str_serialnum,str_devicetype,str_devicesubtype;

                    for (int i = 0; i < listcount; i++)
                    {
                        in_json_child2 = json_object_array_get_idx(in_json_child1, i);

                        if (json_object_object_get_ex(in_json_child2,strdevicenum.c_str(), &in_json_child3))
                            devicenum = json_object_get_int(in_json_child3);

                        if (json_object_object_get_ex(in_json_child2,usbportnum.c_str(), &in_json_child3))
                            portnum = json_object_get_int(in_json_child3);

                        if (json_object_object_get_ex(in_json_child2,vendorname.c_str(), &in_json_child3))
                            str_vendorname = json_object_get_string(in_json_child3);

                        if (json_object_object_get_ex(in_json_child2,productname.c_str(), &in_json_child3))
                            str_productname = json_object_get_string(in_json_child3);

                        if (json_object_object_get_ex(in_json_child2,serialnumber.c_str(), &in_json_child3))
                            str_serialnum = json_object_get_string(in_json_child3);

                        if (json_object_object_get_ex(in_json_child2,devicetype.c_str(), &in_json_child3))
                            str_devicetype = json_object_get_string(in_json_child3);

                        if (json_object_object_get_ex(in_json_child2,devicesubtype.c_str(), &in_json_child3))
                            str_devicesubtype = json_object_get_string(in_json_child3);

                        if (cam == str_devicetype)
                        {
                            dev_info_[camcount].device_num = devicenum;
                            dev_info_[camcount].port_num = portnum;
                            dev_info_[camcount].vendor_name = str_vendorname;
                            dev_info_[camcount].product_name = str_productname;
                            dev_info_[camcount].serial_number = str_serialnum;
                            dev_info_[camcount].device_type = str_devicetype;
                            dev_info_[camcount].device_subtype = str_devicesubtype;
                            dev_info_[camcount].cam_state = DEVICE_EVENT_STATE_PLUGGED;

                            camcount++;
                            devicecount = camcount;
                        }

                        if(camcount >= MAX_DEVICE_COUNT)
                            break;
                    }
                    if(devicecount > camcount)
                        dev_info_[camcount].cam_state = DEVICE_EVENT_STATE_UNPLUGGED;
                }
            }
        }
    }

    if(NULL != subscribeToDeviceStateCb_)
        subscribeToDeviceStateCb_(dev_info_);

    if (in_json)
        json_object_put(in_json);
    in_json = NULL;

    LSMessageUnref(message);

    return true;
}

int PDMClient::subscribeToClient(pdmhandler_cb cb)
{
    //register to PDM luna service with cb to be called
    DLOG_SDK(std::cout << "subscribeToClient PDM" << std::endl;);
    subscribeToDeviceStateCb_ = cb;

    subscribeToPdmService();
    return CAMERA_ERROR_NONE;
}

int PDMClient::subscribeToPdmService()
{
    int ret = CAMERA_ERROR_NONE;
    LSError lserror;
    LSErrorInit(&lserror);

    GMainLoop *gMainLoop = g_main_loop_new(NULL,FALSE);

    LSHandle *handle = NULL;
    int retval  = LSRegister(clientsrv.c_str(),&handle,&lserror);
    if(!retval)
    {
        g_print("PDM client unable to register to luna-bus\n");
        ret = CAMERA_ERROR_UNKNOWN;
        goto exit;
    }

    if (!LSGmainAttach(handle, gMainLoop, &lserror))
    {
        g_print("PDM client unable to attach service\n");
        ret = CAMERA_ERROR_UNKNOWN;
        goto exit;
    }
     retval = LSCall(handle,uri.c_str(),payload.c_str(),deviceStateCb,NULL,NULL,&lserror);
     if (!retval)
     {
         g_print("PDM client uUnable to unregister service\n");
         ret = CAMERA_ERROR_UNKNOWN;
         goto exit;
     }
     g_main_loop_run(gMainLoop);

exit:
    if (LSErrorIsSet(&lserror))
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
    if (handle)
    {
        LSErrorInit(&lserror);
        if (false == LSUnregister(handle, &lserror))
        {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }
    }
    g_main_loop_unref(gMainLoop);
    gMainLoop = NULL;
    return ret;
}
