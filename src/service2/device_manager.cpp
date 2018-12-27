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

/*-----------------------------------------------------------------------------
 #include
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "device_manager.h"
#include "command_manager.h"
#include "device_controller.h"

#include <string.h>

/*-----------------------------------------------------------------------------
 Static  Static prototype
 (Static Variables & Function Prototypes Declarations)
 ------------------------------------------------------------------------------*/
typedef struct _DEVICE_STATUS
{
  // Device
  char strDeviceName[256];
  int nDeviceID;
  int nDevIndex;
  int nDevCount;
  void *pcamhandle;
  bool isDeviceOpen;
  int nSessionID;
  bool isSessionOwner;
  DEVICE_TYPE_T devType;
  DEVICE_LIST_T stList;
} DEVICE_STATUS;

static DEVICE_STATUS gdev_status[MAX_DEVICE];

DeviceManager::DeviceManager() : ndevcount_(0) {}

int DeviceManager::findDevNum(int ndevicehandle)
{
  int nDeviceID = INVALID_ID;
  PMLOG_INFO(CONST_MODULE_DM, "find_devnum : ndevcount_: %d \n", ndevcount_);

  for (int i = 0; i < ndevcount_; i++)
  {
    PMLOG_INFO(CONST_MODULE_DM, "find_devnum : gdev_status[%d].nDevIndex : %d \n", i,
               gdev_status[i].nDevIndex);
    PMLOG_INFO(CONST_MODULE_DM, "find_devnum : gdev_status[%d].nDeviceID : %d \n", i,
               gdev_status[i].nDeviceID);
    PMLOG_INFO(CONST_MODULE_DM, "find_devnum : ndevicehandle : %d \n", ndevicehandle);

    if ((gdev_status[i].nDevIndex == ndevicehandle) || (gdev_status[i].nDeviceID == ndevicehandle))
    {
      nDeviceID = i;
      PMLOG_INFO(CONST_MODULE_DM, "dev_num is :%d\n", i);
    }
  }

  return nDeviceID;
}

bool DeviceManager::deviceStatus(int deviceID, DEVICE_TYPE_T devType, bool status)
{
  PMLOG_INFO(CONST_MODULE_LUNA, "deviceStatus : deviceID %d status : %d \n!!", deviceID, status);
  int dev_num = findDevNum(deviceID);
  PMLOG_INFO(CONST_MODULE_LUNA, "deviceStatus : dev_num : %d \n!!", dev_num);

  if (INVALID_ID == dev_num)
    return CONST_PARAM_VALUE_FALSE;
  if (status)
  {
    gdev_status[dev_num].devType = devType;
    gdev_status[dev_num].isDeviceOpen = true;
  }
  else
  {
    gdev_status[dev_num].devType = DEVICE_DEVICE_UNDEFINED;
    gdev_status[dev_num].isDeviceOpen = false;
  }

  return CONST_PARAM_VALUE_TRUE;
}

bool DeviceManager::isDeviceOpen(int *deviceID)
{
  PMLOG_INFO(CONST_MODULE_LUNA, "DeviceManager::isDeviceOpen !!\n");
  int dev_num = findDevNum(*deviceID);
  if (INVALID_ID == dev_num)
  {
    *deviceID = dev_num;
    return CONST_PARAM_VALUE_FALSE;
  }

  PMLOG_INFO(CONST_MODULE_LUNA, "isDeviceOpen :  *deviceID : %d\n", *deviceID);
  PMLOG_INFO(CONST_MODULE_LUNA, "isDeviceOpen :  *gdev_status[%d].nDeviceID : %d\n", dev_num,
             gdev_status[dev_num].nDeviceID);

  if (*deviceID == gdev_status[dev_num].nDeviceID)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "isDeviceOpen : gdev_status[%d].isDeviceOpen : %d\n", dev_num,
               gdev_status[dev_num].isDeviceOpen);
    if (gdev_status[dev_num].isDeviceOpen)
    {
      PMLOG_INFO(CONST_MODULE_DM, "Device is open\n!!");
      return CONST_PARAM_VALUE_TRUE;
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_DM, "Device is not open\n!!");
      return CONST_PARAM_VALUE_FALSE;
    }
  }
  else
    return CONST_PARAM_VALUE_FALSE;
}

bool DeviceManager::isDeviceValid(DEVICE_TYPE_T devType, int *deviceID)
{
  int dev_num = findDevNum(*deviceID);
  if (INVALID_ID == dev_num)
  {
    *deviceID = dev_num;
    return CONST_PARAM_VALUE_TRUE;
  }

  if (TRUE == gdev_status[dev_num].isDeviceOpen)
  {
    return CONST_PARAM_VALUE_TRUE;
  }
  else
  {
    return CONST_PARAM_VALUE_FALSE;
  }
}

void DeviceManager::getDeviceNode(int *device_id, std::string &strdevicenode)
{
  int dev_num = findDevNum(*device_id);
  if (INVALID_ID == dev_num)
  {
    *device_id = dev_num;
    return;
  }
  strdevicenode = gdev_status[dev_num].stList.strDeviceNode;
  return;
}

void DeviceManager::getDeviceHandle(int *device_id, void **devicehandle)
{
  int dev_num = findDevNum(*device_id);
  if (INVALID_ID == dev_num)
  {
    *device_id = dev_num;
    return;
  }
  *devicehandle = gdev_status[dev_num].pcamhandle;
  return;
}

int DeviceManager::getDeviceId(int *device_id)
{
  int dev_num = findDevNum(*device_id);
  if (INVALID_ID == dev_num)
  {
    *device_id = dev_num;
    return 0;
  }

  return gdev_status[dev_num].nDeviceID;
}

DEVICE_RETURN_CODE_T DeviceManager::getList(int *pCamDev, int *pMicDev, int *pCamSupport,
                                            int *pMicSupport)
{
  PMLOG_INFO(CONST_MODULE_DM, "DeviceManager::getList!!\n");

  DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
  int devCount = ndevcount_;
  DEVICE_LIST_T pList[devCount];

  if (ndevcount_)
  {
    for (int i = 0; i < ndevcount_; i++)
    {
      pList[i] = (gdev_status[i].stList);
    }
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_DM, "No device detected by PDM!!!\n");
    return DEVICE_OK;
  }
  ret = DeviceControl::getInstance().getDeviceList(pList, pCamDev, pMicDev, pCamSupport,
                                                   pMicSupport, devCount);
  if (DEVICE_OK != ret)
  {
    PMLOG_INFO(CONST_MODULE_DM, "Failed at control function\n");
    return ret;
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceManager::updateList(DEVICE_LIST_T *pList, int nDevCount,
                                               DEVICE_EVENT_STATE_T *pCamEvent,
                                               DEVICE_EVENT_STATE_T *pMicEvent)
{
  PMLOG_INFO(CONST_MODULE_DM, "DeviceManager::updateList started! nDevCount : %d \n", nDevCount);

  DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
  int nCamDev = 0;
  int nMicDev = 0;
  int nCamSupport = 0;
  int nMicSupport = 0;

  if (ndevcount_ < nDevCount)
    *pCamEvent = DEVICE_EVENT_STATE_PLUGGED;
  else if (ndevcount_ > nDevCount)
    *pCamEvent = DEVICE_EVENT_STATE_UNPLUGGED;
  else
    PMLOG_INFO(CONST_MODULE_DM, "No event changed!!\n");

  ndevcount_ = nDevCount;
  for (int i = 0; i < ndevcount_; i++)
  {
    strncpy(gdev_status[i].stList.strVendorName, pList[i].strVendorName,
            (CONST_MAX_STRING_LENGTH - 1));
    PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].stList.strVendorName : %s \n", i,
               gdev_status[i].stList.strVendorName);
    strncpy(gdev_status[i].stList.strProductName, pList[i].strProductName,
            (CONST_MAX_STRING_LENGTH - 1));
    PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].stList.strProductName : %s \n", i,
               gdev_status[i].stList.strProductName);
    strncpy(gdev_status[i].stList.strSerialNumber, pList[i].strSerialNumber,
            (CONST_MAX_STRING_LENGTH - 1));
    PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].stList.strSerialNumber : %s \n", i,
               gdev_status[i].stList.strSerialNumber);
    strncpy(gdev_status[i].stList.strDeviceSubtype, pList[i].strDeviceSubtype,
            CONST_MAX_STRING_LENGTH - 1);
    PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].stList.strDeviceSubtype : %s \n", i,
               gdev_status[i].stList.strDeviceSubtype);
    strncpy(gdev_status[i].stList.strDeviceType, pList[i].strDeviceType,
            (CONST_MAX_STRING_LENGTH - 1));
    PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].stList.strDeviceType : %s \n", i,
               gdev_status[i].stList.strDeviceType);
    gdev_status[i].stList.nDeviceNum = pList[i].nDeviceNum;
    PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].stList.nDeviceNum : %d \n", i,
               gdev_status[i].stList.nDeviceNum);
    gdev_status[i].nDevCount = nDevCount;
    PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].nDevCount : %d \n", i, gdev_status[i].nDevCount);
    gdev_status[i].nDevIndex = i + 1;
    PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].nDevIndex : %d \n", i, gdev_status[i].nDevIndex);
    if (strcmp(pList[i].strDeviceType, "CAM") == 0)
      gdev_status[i].devType = DEVICE_CAMERA;
    strncpy(gdev_status[i].stList.strDeviceNode, pList[i].strDeviceNode,
            CONST_MAX_STRING_LENGTH - 1);
    PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].stList.strDeviceNode : %s \n", i,
               gdev_status[i].stList.strDeviceNode);
  }

  ret = DeviceControl::getInstance().getDeviceList(pList, &nCamDev, &nMicDev, &nCamSupport,
                                                   &nMicSupport, nDevCount);
  if (DEVICE_OK == ret)
    PMLOG_INFO(CONST_MODULE_LUNA, "%s:%d] ended!", __FUNCTION__, __LINE__);
  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceManager::getInfo(int ndev_id, CAMERA_INFO_T *p_info)
{
  PMLOG_INFO(CONST_MODULE_DM, "getInfo started ! ndev_id : %d \n", ndev_id);

  int ncam_id = findDevNum(ndev_id);
  if (INVALID_ID == ncam_id)
    return DEVICE_ERROR_NODEVICE;

  PMLOG_INFO(CONST_MODULE_DM, "gdev_status[%d].nDevIndex : %d \n", ncam_id,
             gdev_status[ncam_id].nDevIndex);

  DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
  std::string strdevicenode;
  if (gdev_status[ncam_id].nDevIndex == ndev_id)
  {
    strdevicenode = gdev_status[ncam_id].stList.strDeviceNode;
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_DM, "Failed to get device number\n");
    return DEVICE_ERROR_NODEVICE;
  }

  ret = DeviceControl::getInstance().getDeviceInfo(strdevicenode, p_info);
  if (DEVICE_OK != ret)
  {
    PMLOG_INFO(CONST_MODULE_DM, "Failed to get device info\n");
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceManager::createHandle(int deviceid, int *devicehandle,
                                                 std::string subsystem)
{
  PMLOG_INFO(CONST_MODULE_DM, "createHandle started! deviceid : %d \n", deviceid);

  int dev_num = findDevNum(deviceid);
  if (INVALID_ID == dev_num)
    return DEVICE_ERROR_NODEVICE;

  void *p_cam_handle;
  DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().createHandle(&p_cam_handle, subsystem);
  if (DEVICE_OK == ret)
  {
    *devicehandle = rand() % 10000;
    gdev_status[dev_num].nDeviceID = *devicehandle;
    gdev_status[dev_num].pcamhandle = p_cam_handle;
  }
  else
    gdev_status[dev_num].pcamhandle = NULL;

  PMLOG_INFO(CONST_MODULE_DM, "createHandle ended \n");
  return DEVICE_OK;
}
