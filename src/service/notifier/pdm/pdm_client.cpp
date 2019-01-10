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

#include "pdm_client.h"
#include "constants.h"
#include "device_manager.h"
#include "libudev.h"
#include <glib.h>
#include <pbnjson.hpp>

DEVICE_LIST_T dev_info_[MAX_DEVICE_COUNT];
pdmhandlercb subscribeToDeviceInfoCb_;
int devicecount = 0;

static void _device_init(int devicenum, char *devicenode)
{
  struct udev *camudev;
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices, *dev_list_entry;

  camudev = udev_new();
  if (!camudev)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "FAIL: To create udev for camera\n");
  }
  enumerate = udev_enumerate_new(camudev);
  udev_enumerate_add_match_subsystem(enumerate, "video4linux");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);
  udev_list_entry_foreach(dev_list_entry, devices)
  {
    const char *path;
    const char *strDeviceNode;
    const char *devnum;
    int nDeviceNumber;
    struct udev_device *dev;

    path = udev_list_entry_get_name(dev_list_entry);
    dev = udev_device_new_from_syspath(camudev, path);
    strDeviceNode = udev_device_get_devnode(dev);
    dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if (!dev)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "Unable to find parent usb device.");
      exit(1);
    }
    devnum = udev_device_get_sysattr_value(dev, "devnum");
    nDeviceNumber = atoi(devnum);
    if (nDeviceNumber == devicenum)
      strcpy(devicenode, strDeviceNode);

    udev_device_unref(dev);
  }
  udev_enumerate_unref(enumerate);
  udev_unref(camudev);
}

static bool deviceStateCb(LSHandle *lsHandle, LSMessage *message, void *user_data)
{
  PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb callback received\n");

  jerror *error = NULL;
  const char *payload = LSMessageGetPayload(message);
  jvalue_ref jin_obj = jdom_create(j_cstr_to_buffer(payload), jschema_all(), &error);

  if (jis_valid(jin_obj))
  {
    bool retvalue;
    jboolean_get(jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_RETURNVALUE)), &retvalue);
    PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb retvalue : %d \n", retvalue);
    if (retvalue)
    {
      jvalue_ref jin_obj_child =
          jobject_get(jin_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_NON_STORAGE_DEVICE_LIST));

      int listcount = jarray_size(jin_obj_child);
      PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb listcount : %d \n", listcount);

      int camcount = 0;

      for (int i = 0; i < listcount; i++)
      {
        jvalue_ref jin_array_obj = jarray_get(jin_obj_child, i);
        int portnum = -1;
        jnumber_get_i32(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_USB_PORT_NUM)),
                        &portnum);
        PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb portnum : %d \n", portnum);

        raw_buffer serialnum = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_SERIAL_NUMBER)));
        std::string str_serialnum = serialnum.m_str;
        PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb serialnum : %s \n", str_serialnum.c_str());

        bool b_powerstatus = false;
        jboolean_get(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_IS_POWER_ON_CONNECT)),
            &b_powerstatus);
        PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb b_powerstatus : %d \n", b_powerstatus);

        int devicenum = -1;
        jnumber_get_i32(jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_NUM)),
                        &devicenum);
        PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb devicenum : %d \n", devicenum);

        raw_buffer devsubtype = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_SUBTYPE)));
        std::string str_devicesubtype = devsubtype.m_str;
        PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb str_devicesubtype : %s \n",
                   str_devicesubtype.c_str());

        raw_buffer vendor_name = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_VENDOR_NAME)));
        std::string str_vendorname = vendor_name.m_str;
        PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb str_vendorname : %s \n",
                   str_vendorname.c_str());

        raw_buffer device_type = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_DEVICE_TYPE)));
        std::string str_devicetype = device_type.m_str;
        PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb str_devicetype : %s \n",
                   str_devicetype.c_str());

        raw_buffer product_name = jstring_get_fast(
            jobject_get(jin_array_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PRODUCT_NAME)));
        std::string str_productname = product_name.m_str;
        PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb str_productname : %s \n",
                   str_productname.c_str());

        if (cstr_cam == str_devicetype)
        {
          PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb received cam device\n");
          dev_info_[camcount].nDeviceNum = devicenum;
          dev_info_[camcount].nPortNum = portnum;

          strncpy(dev_info_[camcount].strVendorName, str_vendorname.c_str(),
                  CONST_MAX_STRING_LENGTH - 1);
          dev_info_[camcount].strVendorName[CONST_MAX_STRING_LENGTH - 1] = '\0';

          strncpy(dev_info_[camcount].strProductName, str_productname.c_str(),
                  CONST_MAX_STRING_LENGTH - 1);
          dev_info_[camcount].strProductName[CONST_MAX_STRING_LENGTH - 1] = '\0';

          strncpy(dev_info_[camcount].strSerialNumber, str_serialnum.c_str(),
                  CONST_MAX_STRING_LENGTH - 1);
          dev_info_[camcount].strSerialNumber[CONST_MAX_STRING_LENGTH - 1] = '\0';

          strncpy(dev_info_[camcount].strDeviceType, str_devicetype.c_str(),
                  CONST_MAX_STRING_LENGTH - 1);
          dev_info_[camcount].strDeviceType[CONST_MAX_STRING_LENGTH - 1] = '\0';

          strncpy(dev_info_[camcount].strDeviceSubtype, str_devicesubtype.c_str(),
                  CONST_MAX_STRING_LENGTH - 1);
          dev_info_[camcount].strDeviceSubtype[CONST_MAX_STRING_LENGTH - 1] = '\0';

          dev_info_[camcount].isPowerOnConnect = b_powerstatus;

          _device_init(dev_info_[camcount].nDeviceNum, dev_info_[camcount].strDeviceNode);

          camcount++;
        }
      }

      DEVICE_EVENT_STATE_T nCamEvent = DEVICE_EVENT_NONE;
      DEVICE_EVENT_STATE_T nMicEvent = DEVICE_EVENT_NONE;
      PMLOG_INFO(CONST_MODULE_LUNA, "deviceStateCb camcount : %d \n", camcount);

      DeviceManager::getInstance().updateList(dev_info_, camcount, &nCamEvent, &nMicEvent);
    }
    j_release(&jin_obj);

    if (NULL != subscribeToDeviceInfoCb_)
      subscribeToDeviceInfoCb_(dev_info_);
  }

  return true;
}

void PDMClient::subscribeToClient(pdmhandlercb cb)
{
  // register to PDM luna service with cb to be called
  subscribeToDeviceInfoCb_ = cb;

  subscribeToPdmService();
}

void PDMClient::setLSHandle(LSHandle *handle) { lshandle_ = handle; }

int PDMClient::subscribeToPdmService()
{
  int retval = 0;
  int ret = 0;

  LSError lserror;
  LSErrorInit(&lserror);

  // get camera service handle and register cb function with pdm
  retval = LSCall(lshandle_, cstr_uri.c_str(), cstr_payload.c_str(), deviceStateCb, NULL, NULL,
                  &lserror);
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
