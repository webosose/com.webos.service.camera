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

DEVICE_RETURN_CODE_T CommandManager::createHandle(int deviceid, int *devicehandle,
                                                  std::string subsystem)
{
  PMLOG_INFO(CONST_MODULE_CM, "createHandle started!deviceid : %d \n", deviceid);

  DEVICE_RETURN_CODE_T ret = DEVICE_OK;
  // create device handle
  ret = DeviceManager::getInstance().createHandle(deviceid, devicehandle, subsystem);
  if (DEVICE_OK != ret)
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Failed to createHandle\n");
  }

  return ret;
}

DEVICE_RETURN_CODE_T CommandManager::open(int deviceid, int *devicehandle)
{
  PMLOG_INFO(CONST_MODULE_CM, "open started! deviceid : %d \n", deviceid);

  // check if camera device requested to open is valid and not already opened
  if (DeviceManager::getInstance().isDeviceValid(DEVICE_CAMERA, &deviceid))
  {
    if (INVALID_ID == deviceid)
      return DEVICE_ERROR_NODEVICE;
    else
      return DEVICE_ERROR_DEVICE_IS_ALREADY_OPENED;
  }
  else
  {
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    // create v4l2 handle
    ret = createHandle(deviceid, devicehandle, "libv4l2-camera-plugin.so");
    if (DEVICE_OK != ret)
    {
      PMLOG_ERROR(CONST_MODULE_CM, "Failed to create handle\n");
      return DEVICE_ERROR_CAN_NOT_OPEN;
    }

    std::string device_node;
    // get the device node of requested camera to be opened
    DeviceManager::getInstance().getDeviceNode(&deviceid, device_node);
    PMLOG_INFO(CONST_MODULE_CM, "open : device_node : %s \n", device_node.c_str());

    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    PMLOG_INFO(CONST_MODULE_CM, "open : handle : %p \n", handle);

    // open the camera here
    ret = DeviceControl::getInstance().open(handle, device_node);
    if (DEVICE_OK != ret)
      PMLOG_ERROR(CONST_MODULE_CM, "Failed to open device\n");
    else
      DeviceManager::getInstance().deviceStatus(deviceid, DEVICE_CAMERA, TRUE);

    return ret;
  }
}

DEVICE_RETURN_CODE_T CommandManager::close(int deviceid)
{
  PMLOG_INFO(CONST_MODULE_CM, "close started! \n");

  DEVICE_RETURN_CODE_T ret = DEVICE_OK;

  if (INVALID_ID == deviceid)
    return DEVICE_ERROR_WRONG_PARAM;

  // check if device is opened
  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    PMLOG_INFO(CONST_MODULE_CM, "open : handle : %p \n", handle);

    ret = DeviceControl::getInstance().close(handle);
    if (DEVICE_OK == ret)
    {
      DeviceManager::getInstance().deviceStatus(deviceid, DEVICE_CAMERA, FALSE);
      ret = DeviceControl::getInstance().destroyHandle(handle);
    }
    else
      PMLOG_ERROR(CONST_MODULE_CM, "Failed to close device\n");

    return ret;
  }
  else
    return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
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
    PMLOG_ERROR(CONST_MODULE_CM, "Failed to get device list\n");
  }

  return ret;
}

DEVICE_RETURN_CODE_T CommandManager::updateList(DEVICE_LIST_T *plist, int ncount,
                                                DEVICE_EVENT_STATE_T *pcamevent,
                                                DEVICE_EVENT_STATE_T *pmicevent)
{
  PMLOG_INFO(CONST_MODULE_CM, "updateList nDevCount : %d\n", ncount);

  DEVICE_RETURN_CODE_T ret = DEVICE_OK;
  // update list of devices
  ret = DeviceManager::getInstance().updateList(plist, ncount, pcamevent, pmicevent);
  if (DEVICE_OK != ret)
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Failed to get device list\n");
  }

  return ret;
}

DEVICE_RETURN_CODE_T CommandManager::getProperty(int deviceid, CAMERA_PROPERTIES_T *devproperty)
{
  PMLOG_INFO(CONST_MODULE_CM, "getProperty deviceID : %d\n", deviceid);

  if ((INVALID_ID == deviceid) || (NULL == devproperty))
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

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    PMLOG_INFO(CONST_MODULE_CM, "Device is open \n");
  }
  else
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }

  void *handle;
  DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
  // get property of device opened
  DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().getDeviceProperty(handle, devproperty);
  return ret;
}

DEVICE_RETURN_CODE_T CommandManager::setProperty(int deviceid, CAMERA_PROPERTIES_T *oInfo)
{
  PMLOG_INFO(CONST_MODULE_CM, "setProperty deviceID : %d\n", deviceid);

  if (INVALID_ID == deviceid)
    return DEVICE_ERROR_WRONG_PARAM;

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // set device properties
    DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().setDeviceProperty(handle, oInfo);
    return ret;
  }
  else
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T CommandManager::setFormat(int deviceid, FORMAT oformat)
{
  PMLOG_INFO(CONST_MODULE_CM, "setFormat started! deviceid : %d \n", deviceid);

  if (INVALID_ID == deviceid)
    return DEVICE_ERROR_WRONG_PARAM;

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // set format
    DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().setFormat(handle, oformat);
    return ret;
  }
  else
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
}

DEVICE_RETURN_CODE_T CommandManager::startPreview(int deviceid, int *pkey)
{
  PMLOG_INFO(CONST_MODULE_CM, "startPreview started : deviceid : %d\n", deviceid);

  if (INVALID_ID == deviceid)
    return DEVICE_ERROR_WRONG_PARAM;

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // start preview
    DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().startPreview(handle, pkey);
    return ret;
  }
  else
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T CommandManager::stopPreview(int deviceid)
{
  PMLOG_INFO(CONST_MODULE_CM, "stopPreview started : deviceid : %d\n", deviceid);

  if (INVALID_ID == deviceid)
    return DEVICE_ERROR_WRONG_PARAM;

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // stop preview
    DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().stopPreview(handle);
    return ret;
  }
  else
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T CommandManager::startCapture(int deviceid, FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_CM, "startCapture started : deviceid : %d\n", deviceid);

  if (INVALID_ID == deviceid)
    return DEVICE_ERROR_WRONG_PARAM;

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // start capture
    DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().startCapture(handle, sformat);
    return ret;
  }
  else
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T CommandManager::stopCapture(int deviceid)
{
  PMLOG_INFO(CONST_MODULE_CM, "stopCapture started : deviceid : %d\n", deviceid);

  if (INVALID_ID == deviceid)
    return DEVICE_ERROR_WRONG_PARAM;

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // stop capture
    DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().stopCapture(handle);
    return ret;
  }
  else
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T CommandManager::captureImage(int deviceid, int ncount, FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_CM, "captureImage started : deviceid : %d\n", deviceid);

  if (INVALID_ID == deviceid)
    return DEVICE_ERROR_WRONG_PARAM;

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // capture number of images specified by ncount
    DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().captureImage(handle, ncount, sformat);
    return ret;
  }
  else
  {
    PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}
