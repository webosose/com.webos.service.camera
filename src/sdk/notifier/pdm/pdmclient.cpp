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
#include "pdmclient.h"
#include "camera_hal_types.h"
#include "constants.h"
#include <glib.h>
#include <pbnjson.hpp>

camera_details_t dev_info_[MAX_DEVICE_COUNT];
pdmhandler_cb subscribeToDeviceStateCb_;
int devicecount = 0;

static bool deviceStateCb(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
  DLOG_SDK(std::cout << "***********deviceStateCb************" << std::endl;);

  jerror *error = NULL;
  const char *payload = LSMessageGetPayload(message);
  jvalue_ref jin_obj = jdom_create(j_cstr_to_buffer(payload), jschema_all(), &error);

  if (jis_valid(jin_obj))
  {
    bool retvalue;
    jboolean_get(jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_RETURNVALUE)), &retvalue);
    if (retvalue)
    {
      jvalue_ref jin_obj_child =
          jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_NON_STORAGE_DEVICE_LIST));

      int listcount = jarray_size(jin_obj_child);
      int camcount = 0;

      for (int i = 0; i < listcount; i++)
      {
        jvalue_ref jin_array_obj = jarray_get(jin_obj_child, i);
        int portnum = -1;
        jnumber_get_i32(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_USB_PORT_NUM)),
                        &portnum);

        raw_buffer serialnum = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_SERIAL_NUMBER)));
        std::string str_serialnum = serialnum.m_str;

        int devicenum = -1;
        jnumber_get_i32(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_NUM)),
                        &devicenum);

        raw_buffer devsubtype = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_SUBTYPE)));
        std::string str_devicesubtype = devsubtype.m_str;

        raw_buffer vendor_name = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_VENDOR_NAME)));
        std::string str_vendorname = vendor_name.m_str;

        raw_buffer device_type = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_TYPE)));
        std::string str_devicetype = device_type.m_str;

        raw_buffer product_name = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PRODUCT_NAME)));
        std::string str_productname = product_name.m_str;

        if (cstr_cam == str_devicetype)
        {
          dev_info_[camcount].device_num = devicenum;
          dev_info_[camcount].port_num = portnum;
          dev_info_[camcount].vendor_name = str_vendorname;
          dev_info_[camcount].product_name = str_productname;
          dev_info_[camcount].serial_number = str_serialnum;
          dev_info_[camcount].device_type = str_devicetype;
          dev_info_[camcount].device_subtype = str_devicesubtype;
          dev_info_[camcount].cam_state = DEV_EVENT_STATE_PLUGGED;

          camcount++;
        }
        if (camcount >= MAX_DEVICE_COUNT)
          break;
      }
      if (devicecount > camcount)
        dev_info_[camcount].cam_state = DEV_EVENT_STATE_UNPLUGGED;
    }
    j_release(&jin_obj);

    if (NULL != subscribeToDeviceStateCb_)
      subscribeToDeviceStateCb_(dev_info_);
  }

  return true;
}

int PdmClient::subscribeToClient(pdmhandler_cb cb)
{
  // register to PDM luna service with cb to be called
  DLOG_SDK(std::cout << "subscribeToClient PDM" << std::endl;);
  subscribeToDeviceStateCb_ = cb;

  subscribeToPdmService();
  return CAMERA_ERROR_NONE;
}

int PdmClient::subscribeToPdmService()
{
  int ret = CAMERA_ERROR_NONE;
  LSError lserror;
  LSErrorInit(&lserror);

  GMainLoop *gMainLoop = g_main_loop_new(NULL, FALSE);

  LSHandle *handle = NULL;
  int retval = LSRegister(cstr_pdmclient.c_str(), &handle, &lserror);
  if (!retval)
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
  retval =
      LSCall(handle, cstr_uri.c_str(), cstr_payload.c_str(), deviceStateCb, NULL, NULL, &lserror);
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
