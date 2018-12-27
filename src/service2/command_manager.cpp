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
#include "command_manager.h"
#include "constants.h"
#include "device_controller.h"
#include "device_manager.h"
#include "virtual_device_manager.h"

DEVICE_RETURN_CODE_T CommandManager::open(int deviceid, int *devicehandle, std::string appid,
                                          std::string apppriority)
{
  PMLOG_INFO(CONST_MODULE_CM, "open started! deviceid : %d \n", deviceid);

  // open device and return devicehandle
  return VirtualDeviceManager::getInstance().open(deviceid, devicehandle, appid, apppriority);
}

DEVICE_RETURN_CODE_T CommandManager::close(int devhandle, std::string appid)
{
  PMLOG_INFO(CONST_MODULE_CM, "close started! devhandle : %d appid : %s\n", devhandle,
             appid.c_str());

  if (INVALID_ID == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  // send request to close the device
  return VirtualDeviceManager::getInstance().close(devhandle, appid);
}

DEVICE_RETURN_CODE_T CommandManager::getDeviceInfo(int deviceid, CAMERA_INFO_T *pinfo)
{
  PMLOG_INFO(CONST_MODULE_CM, "getDeviceInfo started! deviceid : %d\n", deviceid);

  if (INVALID_ID == deviceid)
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
  DEVICE_RETURN_CODE_T ret = DEVICE_OK;

  // get list of devices connected
  ret = DeviceManager::getInstance().getList(pcamdev, pmicdev, pcamsupport, pmicsupport);
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

  DEVICE_RETURN_CODE_T ret = DEVICE_OK;
  // update list of devices
  ret = DeviceManager::getInstance().updateList(plist, ncount, pcamevent, pmicevent);
  if (DEVICE_OK != ret)
  {
    PMLOG_INFO(CONST_MODULE_CM, "Failed to update device list\n");
  }

  return ret;
}

DEVICE_RETURN_CODE_T CommandManager::getProperty(int devhandle, CAMERA_PROPERTIES_T *devproperty)
{
  PMLOG_INFO(CONST_MODULE_CM, "getProperty devhandle : %d\n", devhandle);

  if ((INVALID_ID == devhandle) || (NULL == devproperty))
    return DEVICE_ERROR_WRONG_PARAM;

  // initilize the property
  devproperty->nZoom = CONST_VARIABLE_INITIALIZE;
  devproperty->nGridZoomX = CONST_VARIABLE_INITIALIZE;
  devproperty->nGridZoomY = CONST_VARIABLE_INITIALIZE;
  devproperty->nPan = CONST_VARIABLE_INITIALIZE;
  devproperty->nTilt = CONST_VARIABLE_INITIALIZE;
  devproperty->nContrast = CONST_VARIABLE_INITIALIZE;
  devproperty->nBrightness = CONST_VARIABLE_INITIALIZE;
  devproperty->nSaturation = CONST_VARIABLE_INITIALIZE;
  devproperty->nSharpness = CONST_VARIABLE_INITIALIZE;
  devproperty->nHue = CONST_VARIABLE_INITIALIZE;
  devproperty->nWhiteBalanceTemperature = CONST_VARIABLE_INITIALIZE;
  devproperty->nGain = CONST_VARIABLE_INITIALIZE;
  devproperty->nGamma = CONST_VARIABLE_INITIALIZE;
  devproperty->nFrequency = CONST_VARIABLE_INITIALIZE;
  devproperty->bMirror = CONST_VARIABLE_INITIALIZE;
  devproperty->nExposure = CONST_VARIABLE_INITIALIZE;
  devproperty->bAutoExposure = CONST_VARIABLE_INITIALIZE;
  devproperty->bAutoWhiteBalance = CONST_VARIABLE_INITIALIZE;
  devproperty->nBitrate = CONST_VARIABLE_INITIALIZE;
  devproperty->nFramerate = CONST_VARIABLE_INITIALIZE;
  devproperty->ngopLength = CONST_VARIABLE_INITIALIZE;
  devproperty->bLed = CONST_VARIABLE_INITIALIZE;
  devproperty->bYuvMode = CONST_VARIABLE_INITIALIZE;
  devproperty->nIllumination = CONST_VARIABLE_INITIALIZE;
  devproperty->bBacklightCompensation = CONST_VARIABLE_INITIALIZE;
  devproperty->nMicMaxGain = CONST_VARIABLE_INITIALIZE;
  devproperty->nMicMinGain = CONST_VARIABLE_INITIALIZE;
  devproperty->nMicGain = CONST_VARIABLE_INITIALIZE;
  devproperty->bMicMute = CONST_VARIABLE_INITIALIZE;

  // send request to get property of device
  return VirtualDeviceManager::getInstance().getProperty(devhandle, devproperty);
}

DEVICE_RETURN_CODE_T CommandManager::setProperty(int devhandle, CAMERA_PROPERTIES_T *oInfo)
{
  PMLOG_INFO(CONST_MODULE_CM, "setProperty devhandle : %d\n", devhandle);

  if (INVALID_ID == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  // send request to set property of device
  return VirtualDeviceManager::getInstance().setProperty(devhandle, oInfo);
}

DEVICE_RETURN_CODE_T CommandManager::setFormat(int devhandle, FORMAT oformat)
{
  PMLOG_INFO(CONST_MODULE_CM, "setFormat started! devhandle : %d \n", devhandle);

  if (INVALID_ID == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  // send request to set format of device
  return VirtualDeviceManager::getInstance().setFormat(devhandle, oformat);
}

DEVICE_RETURN_CODE_T CommandManager::startPreview(int devhandle, int *pkey)
{
  PMLOG_INFO(CONST_MODULE_CM, "startPreview started : devhandle : %d\n", devhandle);

  if (INVALID_ID == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  // start preview
  return VirtualDeviceManager::getInstance().startPreview(devhandle, pkey);
}

DEVICE_RETURN_CODE_T CommandManager::stopPreview(int devhandle)
{
  PMLOG_INFO(CONST_MODULE_CM, "stopPreview started : devhandle : %d\n", devhandle);

  if (INVALID_ID == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  // stop preview
  return VirtualDeviceManager::getInstance().stopPreview(devhandle);
}

DEVICE_RETURN_CODE_T CommandManager::startCapture(int devhandle, FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_CM, "startCapture started : devhandle : %d\n", devhandle);

  if (INVALID_ID == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  // start capture
  return VirtualDeviceManager::getInstance().startCapture(devhandle, sformat);
}

DEVICE_RETURN_CODE_T CommandManager::stopCapture(int devhandle)
{
  PMLOG_INFO(CONST_MODULE_CM, "stopCapture started : devhandle : %d\n", devhandle);

  if (INVALID_ID == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  // stop capture
  return VirtualDeviceManager::getInstance().stopCapture(devhandle);
}

DEVICE_RETURN_CODE_T CommandManager::captureImage(int devhandle, int ncount, FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_CM, "captureImage started : devhandle : %d\n", devhandle);

  if (INVALID_ID == devhandle)
    return DEVICE_ERROR_WRONG_PARAM;

  // capture image
  return VirtualDeviceManager::getInstance().captureImage(devhandle, ncount, sformat);
}
