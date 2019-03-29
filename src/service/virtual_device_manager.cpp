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
#include "virtual_device_manager.h"
#include "constants.h"
#include "device_manager.h"

#include <algorithm>

VirtualDeviceManager::VirtualDeviceManager()
    : virtualhandle_map_(), handlepriority_map_(), bpreviewinprogress_(false),
      bcaptureinprogress_(false), shmkey_(0), sformat_()
{
}

bool VirtualDeviceManager::checkDeviceOpen(int devhandle)
{
  std::map<int, std::string>::iterator it = handlepriority_map_.find(devhandle);

  if (it == handlepriority_map_.end())
    return false;
  else
    return true;
}

bool VirtualDeviceManager::checkAppPriorityMap()
{
  std::map<int, std::string>::iterator it;

  for (it = handlepriority_map_.begin(); it != handlepriority_map_.end(); ++it)
  {
    if (cstr_primary == it->second)
      return true;
  }
  return false;
}

int VirtualDeviceManager::getVirtualDeviceHandle(int devid)
{
  int virtual_devhandle = rand() % 10000;
  DeviceStateMap obj_devstate;
  obj_devstate.ndeviceid_ = DeviceManager::getInstance().getDeviceId(&devid);
  obj_devstate.ecamstate_ = CameraDeviceState::CAM_DEVICE_STATE_OPEN;
  virtualhandle_map_[virtual_devhandle] = obj_devstate;
  return virtual_devhandle;
}

void VirtualDeviceManager::removeVirtualDeviceHandle(int devhandle)
{
  // remove virtual device handle key value from map
  virtualhandle_map_.erase(devhandle);
}

std::string VirtualDeviceManager::getAppPriority(int devhandle)
{
  std::map<int, std::string>::iterator it = handlepriority_map_.find(devhandle);
  if (it != handlepriority_map_.end())
  {
    return it->second;
  }
  return cstr_empty;
}

void VirtualDeviceManager::removeHandlePriorityObj(int devhandle)
{
  // remove virtual device handle key value from map
  handlepriority_map_.erase(devhandle);
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::openDevice(int devid, int *devhandle)
{
  // create v4l2 handle
  void *p_cam_handle;
  DEVICE_RETURN_CODE_T ret =
      objdevicecontrol_.createHandle(&p_cam_handle, "libv4l2-camera-plugin.so");
  if (DEVICE_OK != ret)
  {
    PMLOG_INFO(CONST_MODULE_VDM, "openDevice : Failed to create handle\n");
    return DEVICE_ERROR_CAN_NOT_OPEN;
  }
  DeviceManager::getInstance().updateHandle(devid, p_cam_handle);
  std::string devnode;
  // get the device node of requested camera to be opened
  DeviceManager::getInstance().getDeviceNode(&devid, devnode);
  PMLOG_INFO(CONST_MODULE_VDM, "openDevice : devnode : %s \n", devnode.c_str());

  void *handle;
  DeviceManager::getInstance().getDeviceHandle(&devid, &handle);
  PMLOG_INFO(CONST_MODULE_VDM, "openDevice : handle : %p \n", handle);

  // open the camera here
  ret = objdevicecontrol_.open(handle, devnode);
  if (DEVICE_OK != ret)
    PMLOG_INFO(CONST_MODULE_VDM, "openDevice : Failed to open device\n");
  else
    DeviceManager::getInstance().deviceStatus(devid, DEVICE_CAMERA, TRUE);

  // get virtual device handle for device opened
  *devhandle = getVirtualDeviceHandle(devid);

  return ret;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::open(int devid, int *devhandle, std::string apppriority)
{
  PMLOG_INFO(CONST_MODULE_VDM, "open ! deviceid : %d \n", devid);

  // check if priortiy is not set by user
  if (cstr_empty == apppriority)
  {
    PMLOG_INFO(CONST_MODULE_VDM, "open ! empty app priority\n");
    if (true == checkAppPriorityMap())
    {
      PMLOG_INFO(CONST_MODULE_VDM, "open : Already an app is registered as primary\n");
      apppriority = cstr_secondary;
    }
    else
      apppriority = cstr_primary;
  }
  // check if already a primary app is opened and new app should not be opened
  // as a primary device. Connection should be rejected.
  else if (cstr_primary == apppriority)
  {
    if (true == checkAppPriorityMap())
    {
      PMLOG_INFO(CONST_MODULE_VDM, "open : Already an app has registered as primary device\n");
      return DEVICE_ERROR_ALREADY_OEPENED_PRIMARY_DEVICE;
    }
  }

  // check if camera device requested to open is valid and not already opened
  if (DeviceManager::getInstance().isDeviceValid(DEVICE_CAMERA, &devid))
  {
    // check if deviceid requested is there in connected device list
    if (n_invalid_id == devid)
    {
      PMLOG_INFO(CONST_MODULE_VDM, "open : Device is invalid\n");
      return DEVICE_ERROR_NODEVICE;
    }
    else
    {
      // device is already opened, hence return virtual handle for device
      *devhandle = getVirtualDeviceHandle(devid);
      PMLOG_INFO(CONST_MODULE_VDM, "open : Device is already opened! Handle : %d \n", *devhandle);
      // add handle with priority to map
      handlepriority_map_.insert(std::make_pair(*devhandle, apppriority));

      return DEVICE_OK;
    }
  }
  else
  {
    // open the actual device
    DEVICE_RETURN_CODE_T ret = openDevice(devid, devhandle);
    if (DEVICE_OK == ret)
    {
      // add handle with priority to map
      handlepriority_map_.insert(std::make_pair(*devhandle, apppriority));
    }
    return ret;
  }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::close(int devhandle)
{
  PMLOG_INFO(CONST_MODULE_VDM, "close : devhandle : %d \n", devhandle);

  // check if app has already closed device
  if (false == checkDeviceOpen(devhandle))
  {
    // app already closed device
    PMLOG_INFO(CONST_MODULE_VDM, "close : App has already closed the device\n");
    return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
  }

  // get number of elements in map
  int nelements = handlepriority_map_.size();
  PMLOG_INFO(CONST_MODULE_VDM, "close : nelements : %d \n", nelements);

  if (1 <= nelements)
  {
    // if there are elements in the map, get device id for device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "close : deviceid : %d \n", deviceid);

    // check if device state is open then only allow to close
    if (CameraDeviceState::CAM_DEVICE_STATE_OPEN != obj_devstate.ecamstate_)
    {
      PMLOG_INFO(CONST_MODULE_VDM, "close : Camera State : %d \n", obj_devstate.ecamstate_);
      return DEVICE_ERROR_CAN_NOT_CLOSE;
    }

    // check if device is opened
    if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
    {
      DEVICE_RETURN_CODE_T ret = DEVICE_OK;

      void *handle;
      DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
      PMLOG_INFO(CONST_MODULE_VDM, "close : handle : %p \n", handle);

      // close the device only if its the last app to request for close
      if (1 == nelements)
      {
        // close the actual device
        ret = objdevicecontrol_.close(handle);
        if (DEVICE_OK == ret)
        {
          DeviceManager::getInstance().deviceStatus(deviceid, DEVICE_CAMERA, FALSE);
          ret = objdevicecontrol_.destroyHandle(handle);
          // remove the virtual device
          removeVirtualDeviceHandle(deviceid);
          // since the device is closed, remove the element from map
          removeHandlePriorityObj(devhandle);
        }
        else
          PMLOG_ERROR(CONST_MODULE_VDM, "Failed to close device\n");
      }
      else
      {
        // remove the virtual device
        removeVirtualDeviceHandle(deviceid);
        // remove the app from map
        removeHandlePriorityObj(devhandle);
      }

      return ret;
    }
    else
    {
      // remove the virtual device
      removeVirtualDeviceHandle(deviceid);
      // remove the app from map
      removeHandlePriorityObj(devhandle);
      return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
    }
  }
  else
    return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::startPreview(int devhandle, int *pkey)
{
  PMLOG_INFO(CONST_MODULE_VDM, "startPreview : devhandle : %d \n", devhandle);

  // get device id for virtual device handle
  DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
  int deviceid = obj_devstate.ndeviceid_;
  PMLOG_INFO(CONST_MODULE_VDM, "startPreview : deviceid : %d \n", deviceid);

  // check if device is opened
  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    // check if device state is open then only allow to start preview
    if (CameraDeviceState::CAM_DEVICE_STATE_OPEN != obj_devstate.ecamstate_)
    {
      PMLOG_INFO(CONST_MODULE_VDM, "startPreview : Camera State : %d \n", obj_devstate.ecamstate_);
      return DEVICE_ERROR_CAN_NOT_START;
    }

    if (!bpreviewinprogress_)
    {
      void *handle;
      DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
      // start preview
      DEVICE_RETURN_CODE_T ret = objdevicecontrol_.startPreview(handle, pkey);
      if (DEVICE_OK == ret)
      {
        bpreviewinprogress_ = true;
        shmkey_ = *pkey;
      }
      // add to vector the app calling startPreview
      npreviewhandle_.push_back(devhandle);
      // update state of device to preview
      obj_devstate.ecamstate_ = CameraDeviceState::CAM_DEVICE_STATE_PREVIEW;
      virtualhandle_map_[devhandle] = obj_devstate;
      return ret;
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_VDM, "startPreview : preview already started by other app \n");
      *pkey = shmkey_;
      // add to vector the app calling startPreview
      npreviewhandle_.push_back(devhandle);
      // update state of device to preview
      obj_devstate.ecamstate_ = CameraDeviceState::CAM_DEVICE_STATE_PREVIEW;
      virtualhandle_map_[devhandle] = obj_devstate;
      return DEVICE_OK;
    }
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_VDM, "startPreview : Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::stopPreview(int devhandle)
{
  PMLOG_INFO(CONST_MODULE_VDM, "stopPreview : devhandle : %d \n", devhandle);

  // get device id for virtual device handle
  DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
  int deviceid = obj_devstate.ndeviceid_;
  PMLOG_INFO(CONST_MODULE_VDM, "stopPreview : deviceid : %d \n", deviceid);

  // check if device is opened
  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    // check if device state is preview then only allow to stop preview
    if (CameraDeviceState::CAM_DEVICE_STATE_PREVIEW != obj_devstate.ecamstate_)
    {
      PMLOG_INFO(CONST_MODULE_VDM, "stopPreview : Camera State : %d \n", obj_devstate.ecamstate_);
      return DEVICE_ERROR_CAN_NOT_STOP;
    }

    int size = npreviewhandle_.size();
    PMLOG_INFO(CONST_MODULE_VDM, "stopPreview : size : %d \n", size);
    if (1 < size)
    {
      // remove the handle from vector since stopPreview is called
      std::vector<int>::iterator position =
          std::find(npreviewhandle_.begin(), npreviewhandle_.end(), devhandle);
      if (position != npreviewhandle_.end())
      {
        npreviewhandle_.erase(position);
        // update state of device to open
        obj_devstate.ecamstate_ = CameraDeviceState::CAM_DEVICE_STATE_OPEN;
        virtualhandle_map_[devhandle] = obj_devstate;
        return DEVICE_OK;
      }
      else
      {
        PMLOG_INFO(CONST_MODULE_VDM, "stopPreview : device has already called stopPreview\n");
        return DEVICE_ERROR_NODEVICE;
      }
    }
    else if (1 == size)
    {
      std::vector<int>::iterator position =
          std::find(npreviewhandle_.begin(), npreviewhandle_.end(), devhandle);
      if (position != npreviewhandle_.end())
      {
        // last handle to call stopPreview
        void *handle;
        DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
        // stop preview
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.stopPreview(handle);
        // reset preview parameters for camera device
        if (DEVICE_OK == ret)
        {
          bpreviewinprogress_ = false;
          shmkey_ = 0;
          // remove the handle from vector since stopPreview is called
          npreviewhandle_.erase(position);
          // update state of device to open
          obj_devstate.ecamstate_ = CameraDeviceState::CAM_DEVICE_STATE_OPEN;
          virtualhandle_map_[devhandle] = obj_devstate;
        }
        return ret;
      }
      else
      {
        PMLOG_INFO(CONST_MODULE_VDM, "stopPreview : device has already called stopPreview\n");
        return DEVICE_ERROR_NODEVICE;
      }
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_VDM, "stopPreview : device has already stopped preview\n");
      return DEVICE_ERROR_NODEVICE;
    }
  }
  else
  {
    PMLOG_ERROR(CONST_MODULE_CM, "stopPreview : Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::captureImage(int devhandle, int ncount,
                                                        CAMERA_FORMAT sformat,
                                                        const std::string& imagepath,
                                                        const std::string& mode)
{
  PMLOG_INFO(CONST_MODULE_VDM, "captureImage : devhandle : %d ncount : %d \n", devhandle, ncount);

  // get device id for virtual device handle
  DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
  int deviceid = obj_devstate.ndeviceid_;
  PMLOG_INFO(CONST_MODULE_VDM, "captureImage : deviceid : %d \n", deviceid);

  // check if there is any change in format
  if ((sformat.eFormat != sformat_.eFormat) || (sformat.nHeight != sformat_.nHeight) ||
      (sformat.nWidth != sformat_.nWidth))
  {
    // check if the app requesting startCapture is secondary then do not change settings
    std::string priority = getAppPriority(devhandle);
    PMLOG_INFO(CONST_MODULE_VDM, "captureImage : priority : %s\n", priority.c_str());
    if (cstr_primary != priority)
    {
      // check if there exists any app with primary priority
      if (true == checkAppPriorityMap())
      {
        PMLOG_INFO(CONST_MODULE_VDM,
                   "captureImage : Already an app exists with primary priority\n");
        sformat.eFormat = sformat_.eFormat;
        sformat.nHeight = sformat_.nHeight;
        sformat.nWidth = sformat_.nWidth;
      }
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_VDM, "captureImage : Save the format\n");
      sformat_.eFormat = sformat.eFormat;
      sformat_.nHeight = sformat.nHeight;
      sformat_.nWidth = sformat.nWidth;
    }
  }

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // capture number of images specified by ncount
    DEVICE_RETURN_CODE_T ret =
        objdevicecontrol_.captureImage(handle, ncount, sformat, imagepath, mode);
    return ret;
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_VDM, "captureImage : Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::startCapture(int devhandle, CAMERA_FORMAT sformat,
                                                        const std::string& imagepath)
{
  PMLOG_INFO(CONST_MODULE_VDM, "startCapture : devhandle : %d\n", devhandle);

  // get device id for virtual device handle
  DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
  int deviceid = obj_devstate.ndeviceid_;
  PMLOG_INFO(CONST_MODULE_VDM, "startCapture : deviceid : %d \n", deviceid);

  // check if there is any change in format
  if ((sformat.eFormat != sformat_.eFormat) || (sformat.nHeight != sformat_.nHeight) ||
      (sformat.nWidth != sformat_.nWidth))
  {
    // check if the app requesting startCapture is secondary then do not change settings
    std::string priority = getAppPriority(devhandle);
    PMLOG_INFO(CONST_MODULE_VDM, "startCapture : priority : %s\n", priority.c_str());
    if (cstr_primary != priority)
    {
      // check if there exists any app with primary priority
      if (true == checkAppPriorityMap())
      {
        PMLOG_INFO(CONST_MODULE_VDM,
                   "startCapture : Already an app exists with primary priority\n");
        sformat.eFormat = sformat_.eFormat;
        sformat.nHeight = sformat_.nHeight;
        sformat.nWidth = sformat_.nWidth;
      }
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_VDM, "captureImage : Save the format\n");
      sformat_.eFormat = sformat.eFormat;
      sformat_.nHeight = sformat.nHeight;
      sformat_.nWidth = sformat.nWidth;
    }
  }

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    if (!bcaptureinprogress_)
    {
      void *handle;
      DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
      // start capture
      DEVICE_RETURN_CODE_T ret = objdevicecontrol_.startCapture(handle, sformat, imagepath);
      if (DEVICE_OK == ret)
      {
        bcaptureinprogress_ = true;
        // add to vector the app
        ncapturehandle_.push_back(devhandle);
      }
      return ret;
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_VDM, "startCapture : capture already in progress\n");
      // add to vector the app
      ncapturehandle_.push_back(devhandle);
      return DEVICE_OK;
    }
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_VDM, "startCapture : Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::stopCapture(int devhandle)
{
  PMLOG_INFO(CONST_MODULE_VDM, "stopCapture : devhandle : %d\n", devhandle);

  // get device id for virtual device handle
  DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
  int deviceid = obj_devstate.ndeviceid_;
  PMLOG_INFO(CONST_MODULE_VDM, "stopCapture : deviceid : %d \n", deviceid);

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    int size = ncapturehandle_.size();

    PMLOG_INFO(CONST_MODULE_VDM, "stopCapture : size : %d \n", size);
    if (1 < size)
    {
      // remove the handle from vector since stopCapture is called
      std::vector<int>::iterator position =
          std::find(ncapturehandle_.begin(), ncapturehandle_.end(), devhandle);
      if (position != ncapturehandle_.end())
      {
        ncapturehandle_.erase(position);
        return DEVICE_OK;
      }
      else
      {
        PMLOG_INFO(CONST_MODULE_VDM, "stopCapture : device has already stopped capture\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
      }
    }
    else if (1 == size)
    {
      std::vector<int>::iterator position =
          std::find(ncapturehandle_.begin(), ncapturehandle_.end(), devhandle);
      if (position != ncapturehandle_.end())
      {
        // last handle to call stopCapture
        void *handle;
        DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
        // stop capture
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.stopCapture(handle);
        // reset capture parameters for camera device
        if (DEVICE_OK == ret)
        {
          bcaptureinprogress_ = false;
          // remove the handle from vector since stopPreview is called
          ncapturehandle_.erase(position);
        }
        return ret;
      }
      else
      {
        PMLOG_INFO(CONST_MODULE_VDM, "stopCapture : device has already stopped capture\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
      }
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_VDM, "stopCapture : device has already stopped capture\n");
      return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
    }
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_VDM, "stopCapture : Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::getProperty(int devhandle,
                                                       CAMERA_PROPERTIES_T *devproperty)
{
  PMLOG_INFO(CONST_MODULE_VDM, "getProperty : devhandle : %d\n", devhandle);

  // get device id for virtual device handle
  DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
  int deviceid = obj_devstate.ndeviceid_;
  PMLOG_INFO(CONST_MODULE_VDM, "getProperty : deviceid : %d \n", deviceid);

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // get property of device opened
    DEVICE_RETURN_CODE_T ret = objdevicecontrol_.getDeviceProperty(handle, devproperty);
    return ret;
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_VDM, "getProperty : Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::setProperty(int devhandle, CAMERA_PROPERTIES_T *oInfo)
{
  PMLOG_INFO(CONST_MODULE_VDM, "setProperty : devhandle : %d\n", devhandle);

  // get device id for virtual device handle
  DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
  int deviceid = obj_devstate.ndeviceid_;
  PMLOG_INFO(CONST_MODULE_VDM, "setProperty : deviceid : %d \n", deviceid);

  // check if the app requesting setProperty is secondary then do not change settings
  std::string priority = getAppPriority(devhandle);
  PMLOG_INFO(CONST_MODULE_VDM, "setProperty : priority : %s\n", priority.c_str());
  if (cstr_primary != priority)
  {
    PMLOG_INFO(CONST_MODULE_VDM, "setProperty : Check if any primary app exists\n");
    // check if there exists any app with primary priority
    if (true == checkAppPriorityMap())
    {
      PMLOG_INFO(CONST_MODULE_VDM, "setProperty : Cannot change property as not a primary app\n");
      return DEVICE_ERROR_CAN_NOT_SET;
    }
  }

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // set device properties
    DEVICE_RETURN_CODE_T ret = objdevicecontrol_.setDeviceProperty(handle, oInfo);
    return ret;
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_VDM, "setProperty : Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::setFormat(int devhandle, CAMERA_FORMAT oformat)
{
  PMLOG_INFO(CONST_MODULE_VDM, "setFormat : devhandle : %d\n", devhandle);

  // get device id for virtual device handle
  DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
  int deviceid = obj_devstate.ndeviceid_;
  PMLOG_INFO(CONST_MODULE_VDM, "setFormat : deviceid : %d \n", deviceid);

  // check if the app requesting setFormat is secondary then do not change settings
  std::string priority = getAppPriority(devhandle);
  PMLOG_INFO(CONST_MODULE_VDM, "setFormat : priority : %s\n", priority.c_str());
  if (cstr_primary != priority)
  {
    // check if there exists any app with primary priority
    if (true == checkAppPriorityMap())
    {
      PMLOG_INFO(CONST_MODULE_VDM, "setFormat : Cannot change format as not a primary app\n");
      return DEVICE_ERROR_CAN_NOT_SET;
    }
  }

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // set format
    DEVICE_RETURN_CODE_T ret = objdevicecontrol_.setFormat(handle, oformat);
    if (DEVICE_OK == ret)
    {
      sformat_.eFormat = oformat.eFormat;
      sformat_.nHeight = oformat.nHeight;
      sformat_.nWidth = oformat.nWidth;
    }
    return ret;
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_VDM, "setFormat : Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::getFormat(int devhandle, CAMERA_FORMAT *oformat)
{
  PMLOG_INFO(CONST_MODULE_VDM, "getFormat : devhandle : %d\n", devhandle);

  // get device id for virtual device handle
  DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
  int deviceid = obj_devstate.ndeviceid_;
  PMLOG_INFO(CONST_MODULE_VDM, "getFormat : deviceid : %d \n", deviceid);

  if (DeviceManager::getInstance().isDeviceOpen(&deviceid))
  {
    void *handle;
    DeviceManager::getInstance().getDeviceHandle(&deviceid, &handle);
    // get format of device
    DEVICE_RETURN_CODE_T ret = objdevicecontrol_.getFormat(handle, oformat);
    return ret;
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_VDM, "getFormat : Device not open\n");
    return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
  }
}
