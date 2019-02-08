// Copyright (c) 2019 LG Electronics, Inc.
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
#include "command_manager.h"
#include "constants.h"
#include "device_controller.h"
#include "device_manager.h"

VirtualDeviceManager *CommandManager::getVirtualDeviceMgrObj(int devhandle)
{
  std::multimap<std::string, Device>::iterator it;
  for (it = virtualdevmgrobj_map_.begin(); it != virtualdevmgrobj_map_.end(); ++it)
  {
    Device obj = it->second;
    if (devhandle == obj.devicehandle)
      return obj.ptr;
  }
  return nullptr;
}

void CommandManager::removeVirtualDevMgrObj(int devhandle)
{
  std::multimap<std::string, Device>::iterator it;
  for (it = virtualdevmgrobj_map_.begin(); it != virtualdevmgrobj_map_.end(); ++it)
  {
    Device obj = it->second;
    if (devhandle == obj.devicehandle)
    {
      PMLOG_INFO(CONST_MODULE_CM, "removeVirtualDevMgrObj ! ptr : %p \n", obj.ptr);
      int count = virtualdevmgrobj_map_.count(it->first);
      PMLOG_INFO(CONST_MODULE_CM, "removeVirtualDevMgrObj ! count : %d \n", count);
      if (count == 1)
        delete obj.ptr;
      virtualdevmgrobj_map_.erase(it);
      return;
    }
  }
}

DEVICE_RETURN_CODE_T CommandManager::open(int deviceid, int *devicehandle, std::string apppriority)
{
  PMLOG_INFO(CONST_MODULE_CM, "open ! deviceid : %d \n", deviceid);

  std::string devicenode;
  DeviceManager::getInstance().getDeviceNode(&deviceid, devicenode);
  PMLOG_INFO(CONST_MODULE_CM, "open ! devicenode : %s \n", devicenode.c_str());

  std::multimap<std::string, Device>::iterator it;
  it = virtualdevmgrobj_map_.find(devicenode);

  Device obj;
  if (it == virtualdevmgrobj_map_.end())
  {
    obj.ptr = new VirtualDeviceManager;
    PMLOG_INFO(CONST_MODULE_CM, "open 1 ! ptr : %p \n", obj.ptr);
  }
  else
    obj = it->second;

  PMLOG_INFO(CONST_MODULE_CM, "open 2 ! ptr : %p \n", obj.ptr);

  if (nullptr != obj.ptr)
  {
    // open device and return devicehandle
    DEVICE_RETURN_CODE_T ret = obj.ptr->open(deviceid, devicehandle, apppriority);
    if (DEVICE_OK == ret)
    {
      PMLOG_INFO(CONST_MODULE_CM, "open ! devicehandle : %d \n", *devicehandle);
      obj.devicehandle = *devicehandle;
      obj.deviceid = deviceid;
      virtualdevmgrobj_map_.insert(std::make_pair(devicenode, obj));
    }
    return ret;
  }
  else
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::close(int devhandle)
{
  PMLOG_INFO(CONST_MODULE_CM, "close ! devhandle : %d \n", devhandle);

  if (n_invalid_id == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  PMLOG_INFO(CONST_MODULE_CM, "close ! ptr : %p \n", ptr);
  if (nullptr != ptr)
  {
    // send request to close the device
    DEVICE_RETURN_CODE_T ret = ptr->close(devhandle);
    if (DEVICE_OK == ret)
    {
      removeVirtualDevMgrObj(devhandle);
    }
    return ret;
  }
  else
    return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
}

DEVICE_RETURN_CODE_T CommandManager::getDeviceInfo(int deviceid, camera_device_info_t *pinfo)
{
  PMLOG_INFO(CONST_MODULE_CM, "getDeviceInfo ! deviceid : %d\n", deviceid);

  if (n_invalid_id == deviceid)
    return DEVICE_ERROR_WRONG_PARAM;

  // get info of device requested
  DEVICE_RETURN_CODE_T ret = DeviceManager::getInstance().getInfo(deviceid, pinfo);
  if (DEVICE_OK != ret)
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Failed to get device info\n");
  }

  return ret;
}

DEVICE_RETURN_CODE_T CommandManager::getDeviceList(int *pcamdev, int *pmicdev, int *pcamsupport,
                                                   int *pmicsupport)
{
  PMLOG_INFO(CONST_MODULE_CM, "CommandManager::getDeviceList started! \n");

  // get list of devices connected
  DEVICE_RETURN_CODE_T ret =
      DeviceManager::getInstance().getList(pcamdev, pmicdev, pcamsupport, pmicsupport);
  if (DEVICE_OK != ret)
  {
    PMLOG_INFO(CONST_MODULE_CM, "Failed to get device list\n");
  }

  return ret;
}

DEVICE_RETURN_CODE_T CommandManager::updateList(DEVICE_LIST_T *plist, int ncount,
                                                DEVICE_EVENT_STATE_T *pcamevent,
                                                DEVICE_EVENT_STATE_T *pmicevent)
{
  PMLOG_INFO(CONST_MODULE_CM, "updateList ncount : %d\n", ncount);

  // update list of devices
  DEVICE_RETURN_CODE_T ret =
      DeviceManager::getInstance().updateList(plist, ncount, pcamevent, pmicevent);
  if (DEVICE_OK != ret)
  {
    PMLOG_INFO(CONST_MODULE_CM, "Failed to update device list\n");
  }

  return ret;
}

DEVICE_RETURN_CODE_T CommandManager::getProperty(int devhandle, CAMERA_PROPERTIES_T *devproperty)
{
  PMLOG_INFO(CONST_MODULE_CM, "getProperty devhandle : %d\n", devhandle);

  if ((n_invalid_id == devhandle) || (NULL == devproperty))
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  if (nullptr != ptr)
    // send request to get property of device
    return ptr->getProperty(devhandle, devproperty);
  else
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::setProperty(int devhandle, CAMERA_PROPERTIES_T *oInfo)
{
  PMLOG_INFO(CONST_MODULE_CM, "setProperty devhandle : %d\n", devhandle);

  if (n_invalid_id == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  if (nullptr != ptr)
    // send request to set property of device
    return ptr->setProperty(devhandle, oInfo);
  else
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::setFormat(int devhandle, CAMERA_FORMAT oformat)
{
  PMLOG_INFO(CONST_MODULE_CM, "setFormat ! devhandle : %d \n", devhandle);

  if (n_invalid_id == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  if (nullptr != ptr)
    // send request to set format of device
    return ptr->setFormat(devhandle, oformat);
  else
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::startPreview(int devhandle, int *pkey)
{
  PMLOG_INFO(CONST_MODULE_CM, "startPreview  : devhandle : %d\n", devhandle);

  if (n_invalid_id == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  if (nullptr != ptr)
    // start preview
    return ptr->startPreview(devhandle, pkey);
  else
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::stopPreview(int devhandle)
{
  PMLOG_INFO(CONST_MODULE_CM, "stopPreview  : devhandle : %d\n", devhandle);

  if (n_invalid_id == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  if (nullptr != ptr)
    // stop preview
    return ptr->stopPreview(devhandle);
  else
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::startCapture(int devhandle, CAMERA_FORMAT sformat,
                                                  const std::string& imagepath)
{
  PMLOG_INFO(CONST_MODULE_CM, "startCapture : devhandle : %d\n", devhandle);

  if (n_invalid_id == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  if (nullptr != ptr)
    // start capture
    return ptr->startCapture(devhandle, sformat, imagepath);
  else
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::stopCapture(int devhandle)
{
  PMLOG_INFO(CONST_MODULE_CM, "stopCapture : devhandle : %d\n", devhandle);

  if (n_invalid_id == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  if (nullptr != ptr)
    // stop capture
    return ptr->stopCapture(devhandle);
  else
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::captureImage(int devhandle, int ncount, CAMERA_FORMAT sformat,
                                                  const std::string& imagepath)
{
  PMLOG_INFO(CONST_MODULE_CM, "captureImage : devhandle : %d\n", devhandle);

  if (n_invalid_id == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  if (nullptr != ptr)
    // capture image
    return ptr->captureImage(devhandle, ncount, sformat, imagepath);
  else
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::getFormat(int devhandle, CAMERA_FORMAT *oformat)
{
  PMLOG_INFO(CONST_MODULE_CM, "getFormat : devhandle : %d\n", devhandle);

  if (n_invalid_id == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  VirtualDeviceManager *ptr = getVirtualDeviceMgrObj(devhandle);
  if (nullptr != ptr)
    // get format
    return ptr->getFormat(devhandle, oformat);
  else
    return DEVICE_ERROR_UNKNOWN;
}

int CommandManager::getCameraId(int devhandle)
{
  PMLOG_INFO(CONST_MODULE_CM, "getCameraId : devhandle : %d\n", devhandle);
  if (n_invalid_id == devhandle)
    return n_invalid_id;

  std::multimap<std::string, Device>::iterator it;
  for (it = virtualdevmgrobj_map_.begin(); it != virtualdevmgrobj_map_.end(); ++it)
  {
    Device obj = it->second;
    if (devhandle == obj.devicehandle)
      return obj.deviceid;
  }
  return n_invalid_id;
}
