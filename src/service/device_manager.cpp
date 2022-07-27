// Copyright (c) 2019-2021 LG Electronics, Inc.
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
 (File Inclusions)
 ----------------------------------------------------------------------------*/
#include "device_manager.h"
#include "command_manager.h"
#include "device_controller.h"
#include <string.h>
#include <fstream>

DeviceManager::DeviceManager() {}

int DeviceManager::findDevNum(int ndevicehandle)
{
  int nDeviceID = n_invalid_id;
  PMLOG_DEBUG("ndevicehandle : %d", ndevicehandle);
  PMLOG_DEBUG("deviceMap_.count : %d", deviceMap_.size());

  for (auto iter : deviceMap_)
  {
    PMLOG_DEBUG("iter.second.nDevIndex : %d", iter.second.nDevIndex);
    PMLOG_DEBUG("iter.second.nDeviceID : %d", iter.second.nDeviceID);

    if ((iter.second.nDevIndex == ndevicehandle) || (iter.second.nDeviceID == ndevicehandle))
    {
      nDeviceID = iter.first;
      PMLOG_DEBUG("dev_num is :%d", nDeviceID);
      break;
    }
  }
  return nDeviceID;
}

bool DeviceManager::deviceStatus(int deviceID, DEVICE_TYPE_T devType, bool status)
{
  PMLOG_INFO(CONST_MODULE_DM, "deviceID %d status : %d \n!!", deviceID, status);
  int dev_num = findDevNum(deviceID);
  PMLOG_INFO(CONST_MODULE_DM, "dev_num : %d \n!!", dev_num);

  if (n_invalid_id == dev_num)
    return CONST_PARAM_VALUE_FALSE;
  if (status)
  {
    deviceMap_[dev_num].devType = devType;
    deviceMap_[dev_num].isDeviceOpen = true;
  }
  else
  {
    deviceMap_[dev_num].devType = DEVICE_DEVICE_UNDEFINED;
    deviceMap_[dev_num].isDeviceOpen = false;
  }

  return CONST_PARAM_VALUE_TRUE;
}

bool DeviceManager::isDeviceOpen(int *deviceID)
{
  PMLOG_INFO(CONST_MODULE_DM, "started!");
  int dev_num = findDevNum(*deviceID);
  if (n_invalid_id == dev_num)
  {
    *deviceID = dev_num;
    return CONST_PARAM_VALUE_FALSE;
  }

  PMLOG_DEBUG("isDeviceOpen :  *deviceID : %d\n", *deviceID);
  PMLOG_DEBUG("isDeviceOpen :  deviceMap_[%d].nDeviceID : %d\n", dev_num,
             deviceMap_[dev_num].nDeviceID);

  if (*deviceID == deviceMap_[dev_num].nDeviceID)
  {
    PMLOG_DEBUG("isDeviceOpen : deviceMap_[%d].isDeviceOpen : %d\n", dev_num,
               deviceMap_[dev_num].isDeviceOpen);
    if (deviceMap_[dev_num].isDeviceOpen)
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
  if (n_invalid_id == dev_num)
  {
    *deviceID = dev_num;
    return CONST_PARAM_VALUE_TRUE;
  }

  if (TRUE == deviceMap_[dev_num].isDeviceOpen)
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
  if (n_invalid_id == dev_num)
  {
    *device_id = dev_num;
    return;
  }
  strdevicenode = deviceMap_[dev_num].stList.strDeviceNode;
  return;
}

void DeviceManager::getDeviceHandle(int *device_id, void **devicehandle)
{
  int dev_num = findDevNum(*device_id);
  if (n_invalid_id == dev_num)
  {
    *device_id = dev_num;
    return;
  }
  *devicehandle = deviceMap_[dev_num].pcamhandle;
  return;
}

int DeviceManager::getDeviceId(int *device_id)
{
  int dev_num = findDevNum(*device_id);
  if (n_invalid_id == dev_num)
  {
    *device_id = dev_num;
    return 0;
  }

  return deviceMap_[dev_num].nDeviceID;
}

bool DeviceManager::addVirtualHandle(int devid, int virtualHandle)
{
  PMLOG_INFO(CONST_MODULE_DM, "devid: %d, virualHandle: %d", devid, virtualHandle);
  if (deviceMap_.find(devid) != deviceMap_.end())
  {
    deviceMap_[devid].handleList.push_back(virtualHandle);
    PMLOG_INFO(CONST_MODULE_DM, "deviceMap_[%d].handleList.size : %d",
               devid, deviceMap_[devid].handleList.size());
    return true;
  }

  return false;
}

bool DeviceManager::eraseVirtualHandle(int deviceId, int virtualHandle)
{
  PMLOG_INFO(CONST_MODULE_DM, "deviceId: %d, virtualHandle: %d", deviceId, virtualHandle);

  // find devid
  int devid = 0;
  for (auto iter : deviceMap_)
  {
    PMLOG_INFO(CONST_MODULE_DM, "first: %d, nDeviceID: %d", iter.first, iter.second.nDeviceID);
    if (iter.second.nDeviceID == deviceId)
    {
      devid = iter.first;
      break;
    }
  }
  if (devid == 0) {
    PMLOG_INFO(CONST_MODULE_DM, "Cannot found devid");
    return false;
  }

  PMLOG_INFO(CONST_MODULE_DM, "devid: %d", devid);
  if (deviceMap_.find(devid) != deviceMap_.end())
  {
    for (auto it = deviceMap_[devid].handleList.begin();
         it != deviceMap_[devid].handleList.end(); it++)
    {
      PMLOG_INFO(CONST_MODULE_DM, "cur.handleList : %d", *it);
      if (*it == virtualHandle)
      {
        deviceMap_[devid].handleList.erase(it);
        PMLOG_INFO(CONST_MODULE_DM, "deviceMap_[%d].handleList.size : %d",
                   devid, deviceMap_[devid].handleList.size());
        return true;
      }
    }
  }

  return false;
}

DEVICE_RETURN_CODE_T DeviceManager::getList(int *pCamDev, int *pMicDev, int *pCamSupport,
                                            int *pMicSupport) const
{
  PMLOG_INFO(CONST_MODULE_DM, "started!");

  int devCount = deviceMap_.size();
  if (devCount)
  {
    int i = 0;
    for (auto iter : deviceMap_)
    {
      pCamDev[i] = iter.first;
      pCamSupport[i] = 1;
      i++;
    }
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_DM, "No device detected by PDM!!!\n");
    return DEVICE_OK;
  }

  return DEVICE_OK;
}

bool DeviceManager::addDevice(DEVICE_LIST_T *pList)
{
  DEVICE_STATUS devStatus;
  devStatus.devType = DEVICE_CAMERA;
  devStatus.isDeviceOpen = false;
  devStatus.pcamhandle = nullptr;
  devStatus.nDeviceID = n_invalid_id;
  strncpy(devStatus.stList.strVendorName, pList->strVendorName,
          (CONST_MAX_STRING_LENGTH - 1));
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.strVendorName : %s",
              devStatus.stList.strVendorName);
  strncpy(devStatus.stList.strProductName, pList->strProductName,
          (CONST_MAX_STRING_LENGTH - 1));
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.strProductName : %s",
              devStatus.stList.strProductName);
  strncpy(devStatus.stList.strVendorID, pList->strVendorID,
          (CONST_MAX_STRING_LENGTH - 1));
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.strVendorID : %s",
              devStatus.stList.strVendorID);
  strncpy(devStatus.stList.strProductID, pList->strProductID,
          (CONST_MAX_STRING_LENGTH - 1));
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.strProductID : %s",
              devStatus.stList.strProductID);
  strncpy(devStatus.stList.strDeviceSubtype, pList->strDeviceSubtype,
          CONST_MAX_STRING_LENGTH - 1);
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.strDeviceSubtype : %s",
              devStatus.stList.strDeviceSubtype);
  strncpy(devStatus.stList.strDeviceType, pList->strDeviceType,
          (CONST_MAX_STRING_LENGTH - 1));
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.strDeviceType : %s",
              devStatus.stList.strDeviceType);
  devStatus.stList.nDeviceNum = pList->nDeviceNum;
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.nDeviceNum : %d",
              devStatus.stList.nDeviceNum);
  devStatus.stList.nPortNum = pList->nPortNum;
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.nPortNum : %d",
              devStatus.stList.nPortNum);
  devStatus.stList.isPowerOnConnect = pList->isPowerOnConnect;
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.isPowerOnConnect : %d",
              devStatus.stList.isPowerOnConnect);
  devStatus.nDevCount = deviceMap_.size() + 1;
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.nDevCount : %d", devStatus.nDevCount);

  int devidx = 0;
  for (int i = 1 ; i <= MAX_DEVICE_COUNT ; i++ )
  {
    bool idx_avaible = true;
    for (auto iter : deviceMap_)
    {
      if (iter.first == i)
      {
        idx_avaible = false;
        break;
      }
    }
    if (idx_avaible)
    {
      devidx = i;
      break;
    }
  }
  if (devidx == 0)
    return false;

  devStatus.nDevIndex = devidx;
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.nDevIndex : %d \n", devStatus.nDevIndex);

  /* double-check device path */ 
  if (strstr(pList->strDeviceNode, "/dev/video") != NULL)
  {
      strncpy(devStatus.stList.strDeviceNode, pList->strDeviceNode,
              CONST_MAX_STRING_LENGTH - 1);
  }
  else if (strstr(pList->strDeviceNode, "video") != NULL)
  {
      strncpy(devStatus.stList.strDeviceNode, "/dev/", CONST_MAX_STRING_LENGTH-1);
      strncat(devStatus.stList.strDeviceNode, pList->strDeviceNode, CONST_MAX_STRING_LENGTH-1);
  }
  else
  {
      PMLOG_INFO(CONST_MODULE_DM, "fail to add device:  %s is not a valid device node!!", pList->strDeviceNode);
      return false;
  }
  
  PMLOG_INFO(CONST_MODULE_DM, "devStatus.stList.strDeviceNode : %s \n",
              devStatus.stList.strDeviceNode);
  deviceMap_[devidx] = devStatus;
  PMLOG_INFO(CONST_MODULE_DM, "devidx : %d, deviceMap_.size : %d \n", devidx, deviceMap_.size());
  return true;
}

bool DeviceManager::removeDevice(int devid)
{
  PMLOG_INFO(CONST_MODULE_DM, "devid : %d", devid);
  auto dev = deviceMap_.find(devid);
  if (dev != deviceMap_.end())
  {
    deviceMap_.erase(dev);
    PMLOG_INFO(CONST_MODULE_DM, "erase OK, deviceMap_.size : %d", deviceMap_.size());
    return true;
  }
  PMLOG_INFO(CONST_MODULE_DM, "can not found device for devid : %d", devid);
  return false;
}

DEVICE_RETURN_CODE_T DeviceManager::updateList(DEVICE_LIST_T *pList, int nDevCount,
                                               DEVICE_EVENT_STATE_T *pCamEvent,
                                               DEVICE_EVENT_STATE_T *pMicEvent)
{
  PMLOG_INFO(CONST_MODULE_DM, "started! nDevCount : %d \n", nDevCount);

  if (deviceMap_.size() < nDevCount) // Plugged
  {
    *pCamEvent = DEVICE_EVENT_STATE_PLUGGED;
    for (int i=0 ; i<nDevCount ; i++)
    {
      int id = 0;
      // find exist device
      for (auto iter : deviceMap_)
      {
        if ( strcmp(iter.second.stList.strDeviceNode, pList[i].strDeviceNode) == 0 )
        {
          id = iter.first;
          break;
        }
      }
      // insert new camera device
      if (!id)
      {
        addDevice(&pList[i]);
      }
    }
  }
  else if (deviceMap_.size() > nDevCount) // Unpluged
  {
    *pCamEvent = DEVICE_EVENT_STATE_UNPLUGGED;
    for (auto iter = deviceMap_.begin() ; iter != deviceMap_.end();)
    {
      bool unplugged = true;
      // Find out which camera is unplugged
      for (int i=0 ; i<nDevCount ; i++)
      {
        if (strcmp(iter->second.stList.strDeviceNode, pList[i].strDeviceNode) == 0)
        {
          unplugged = false;
          break;
        }
      }
      if (unplugged)
      {
        if (iter->second.isDeviceOpen && iter->second.handleList.size() > 0)
        {
          PMLOG_INFO(CONST_MODULE_DM, "start cleaning the unplugged device!");
          CommandManager::getInstance().requestPreviewCancel(iter->first);
          for (int i = 0 ; i<iter->second.handleList.size() ; i++)
          {
            CommandManager::getInstance().stopPreview(iter->second.handleList[i]);
            CommandManager::getInstance().close(iter->second.handleList[i]);
          }
          PMLOG_INFO(CONST_MODULE_DM, "end cleaning the unplugged device!");
        }
        removeDevice(iter++->first);
      }
      else
      {
        iter++;
      }
    }
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_DM, "No event changed!!\n");
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceManager::getInfo(int ndev_id, camera_device_info_t *p_info)
{
  PMLOG_INFO(CONST_MODULE_DM, "started ! ndev_id : %d \n", ndev_id);

  int ncam_id = findDevNum(ndev_id);
  if (n_invalid_id == ncam_id)
    return DEVICE_ERROR_NODEVICE;

  PMLOG_INFO(CONST_MODULE_DM, "deviceMap_[%d].nDevIndex : %d \n", ncam_id,
             deviceMap_[ncam_id].nDevIndex);

  std::string strdevicenode;
  if (deviceMap_[ncam_id].nDevIndex == ndev_id)
  {
    strdevicenode = deviceMap_[ncam_id].stList.strDeviceNode;
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_DM, "Failed to get device number\n");
    return DEVICE_ERROR_NODEVICE;
  }

  DEVICE_RETURN_CODE_T ret = DeviceControl::getDeviceInfo(strdevicenode, p_info);
  if (DEVICE_OK != ret)
  {
    PMLOG_INFO(CONST_MODULE_DM, "Failed to get device info\n");
  }
  memset(p_info->str_devicename, '\0', sizeof(p_info->str_devicename));
  strncpy(p_info->str_devicename, deviceMap_[ncam_id].stList.strProductName,
          sizeof(p_info->str_devicename)-1);
  memset(p_info->str_vendorid, '\0', sizeof(p_info->str_vendorid));
  strncpy(p_info->str_vendorid, deviceMap_[ncam_id].stList.strVendorID,
          sizeof(p_info->str_vendorid)-1);
  memset(p_info->str_productid, '\0', sizeof(p_info->str_productid));
  strncpy(p_info->str_productid, deviceMap_[ncam_id].stList.strProductID,
          sizeof(p_info->str_productid)-1);

  return ret;
}

DEVICE_RETURN_CODE_T DeviceManager::updateHandle(int deviceid, void *handle)
{
  PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d \n", deviceid);
  int devicehandle = getRandomNumber();
  int dev_num = findDevNum(deviceid);
  if (n_invalid_id == dev_num)
    return DEVICE_ERROR_NODEVICE;

  deviceMap_[dev_num].nDeviceID = devicehandle;
  deviceMap_[dev_num].pcamhandle = handle;

  return DEVICE_OK;
}
