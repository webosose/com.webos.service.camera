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
#define LOG_TAG "VirtualDeviceManager"
#include "virtual_device_manager.h"
#include "addon.h" // platform-specific functionality extension support
#include "camera_constants.h"
#include "command_manager.h"
#include "device_manager.h"
#include "preview_display_control.h"

VirtualDeviceManager::VirtualDeviceManager()
    : virtualhandle_map_(), handlepriority_map_(), bcaptureinprogress_(false), shmkey_(0),
      sformat_()
{
    PLOGI("");
}

VirtualDeviceManager::~VirtualDeviceManager() { PLOGI(""); }

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
        {
            PLOGI("devhandle = %d, mode = %s", it->first, it->second.c_str());
            return true;
        }
    }
    return false;
}

int VirtualDeviceManager::getVirtualDeviceHandle(int devid)
{
    int virtual_devhandle = getRandomNumber();
    DeviceStateMap obj_devstate;
    obj_devstate.ndeviceid_               = devid;
    obj_devstate.ecamstate_               = CameraDeviceState::CAM_DEVICE_STATE_OPEN;
    virtualhandle_map_[virtual_devhandle] = obj_devstate;
    PLOGI("devid: %d, virtual_devhandle:%d, ndeviceid_:%d", devid, virtual_devhandle,
          obj_devstate.ndeviceid_);
    PLOGI("virtualhandle_map_.size = %zd", virtualhandle_map_.size());
    return virtual_devhandle;
}

void VirtualDeviceManager::removeVirtualDeviceHandle(int devhandle)
{
    // remove virtual device handle key value from map
    virtualhandle_map_.erase(devhandle);
    PLOGI("virtualhandle_map_.size = %zd", virtualhandle_map_.size());
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
    std::string deviceType   = DeviceManager::getInstance().getDeviceType(devid);
    DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.createHal(std::move(deviceType));
    if (DEVICE_OK != ret)
    {
        PLOGI("Failed to create handle\n");
        return DEVICE_ERROR_CAN_NOT_OPEN;
    }

    std::string devnode;
    // get the device node of requested camera to be opened
    DeviceManager::getInstance().getDeviceNode(devid, devnode);
    PLOGI("devnode : %s \n", devnode.c_str());

    std::string payload = "";
    DeviceManager::getInstance().getDeviceUserData(devid, payload);

    // open the camera here
    ret = objcamerahalproxy_.open(std::move(devnode), devid, std::move(payload));
    if (DEVICE_OK != ret)
        PLOGI("Failed to open device\n");
    else
        DeviceManager::getInstance().setDeviceStatus(devid, TRUE);

    // get virtual device handle for device opened
    *devhandle = getVirtualDeviceHandle(devid);

    return ret;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::open(int devid, int *devhandle, std::string appId,
                                                std::string apppriority)
{
    PLOGI("deviceid : %d \n", devid);

    // check if priortiy is not set by user
    if (cstr_empty == apppriority)
    {
        PLOGI("empty app priority\n");
        if (true == checkAppPriorityMap())
        {
            PLOGI("Already an app is registered as primary\n");
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
            PLOGI("Already an app has registered as primary device\n");
            return DEVICE_ERROR_ALREADY_OEPENED_PRIMARY_DEVICE;
        }
    }

    // check if camera device requested to open is valid
    if (!DeviceManager::getInstance().isDeviceValid(devid))
    {
        PLOGI("Device is invalid\n");
        return DEVICE_ERROR_NODEVICE;
    }

    // check if camera device requested to open is not already opened
    if (DeviceManager::getInstance().isDeviceOpen(devid))
    {
        // device is already opened, hence return virtual handle for device
        *devhandle = getVirtualDeviceHandle(devid);
        PLOGI("Device is already opened! Handle : %d \n", *devhandle);
        // add handle with priority to map
        handlepriority_map_.insert(std::make_pair(*devhandle, apppriority));

        return DEVICE_OK;
    }

    // open the actual device
    DEVICE_RETURN_CODE_T ret = openDevice(devid, devhandle);
    if (DEVICE_OK == ret)
    {
        // add handle with priority to map
        handlepriority_map_.insert(std::make_pair(*devhandle, apppriority));
    }
    return ret;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::close(int devhandle)
{
    PLOGI("devhandle : %d \n", devhandle);

    // check if app has already closed device
    if (false == checkDeviceOpen(devhandle))
    {
        // app already closed device
        PLOGE("App has already closed the device\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
    }

    // get number of elements in map
    unsigned long nelements = handlepriority_map_.size();
    PLOGI("nelements : %lu \n", nelements);

    if (nelements == 0)
    {
        PLOGE("Device is already closed.");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
    }

    // if there are elements in the map, get device id for device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    // check if device state is open then only allow to close
    if (CameraDeviceState::CAM_DEVICE_STATE_OPEN != obj_devstate.ecamstate_)
    {
        PLOGE("Camera State : %d \n", (int)obj_devstate.ecamstate_);
        return DEVICE_ERROR_INVALID_STATE;
    }

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    // check if device is opened
    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // close the device Only if its the last app to request for close
        if (1 == nelements)
        {
            // close the actual device
            ret = objcamerahalproxy_.close();
            if (DEVICE_OK == ret)
            {
                DeviceManager::getInstance().setDeviceStatus(deviceid, FALSE);
                ret = objcamerahalproxy_.destroyHal();
                if (DEVICE_OK != ret)
                {
                    PLOGE("Failed to destroy HAL");
                }
            }
            else
            {
                PLOGE("Failed to close device");
                return ret;
            }
        }
    }
    else
    {
        PLOGE("Device is already cloesed");
        ret = DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
    }

    // remove the virtual device
    removeVirtualDeviceHandle(devhandle);
    // remove the element from map
    removeHandlePriorityObj(devhandle);

    return ret;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::startCamera(int devhandle, std::string memtype,
                                                       int *pkey, LSHandle *sh, const char *subskey)
{
    if (isValidMemtype(memtype) == false)
    {
        PLOGE("Invalid memtype : %s", memtype.c_str());
        return DEVICE_ERROR_UNSUPPORTED_MEMORYTYPE;
    }

    PLOGI("devhandle : %d \n", devhandle);

    // Get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    // Check if device is opened
    if (!DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        PLOGE("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }

    // Check if device state is open before starting the preview
    if (CameraDeviceState::CAM_DEVICE_STATE_OPEN != obj_devstate.ecamstate_)
    {
        PLOGE("Camera State : %d \n", (int)obj_devstate.ecamstate_);
        return DEVICE_ERROR_INVALID_STATE;
    }

    unsigned long streaming_handle_size = nstreaminghandle_.size();
    PLOGI("Streaming handle size : %lu", streaming_handle_size);

    if (streaming_handle_size == 0) // Primary
    {
        // start preview
        DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.startPreview(memtype, &shmkey_, sh, subskey);
        if (DEVICE_OK != ret)
        {
            PLOGE("startPreview error : %d", ret);
            return ret;
        }

        objcamerahalproxy_.subscribe();

        // Apply platform-specific policy to solutions if exists.
        if (pAddon_ && pAddon_->hasImplementation())
        {
            std::string deviceKey       = DeviceManager::getInstance().getDeviceKey(deviceid);
            DEVICE_RETURN_CODE_T result = objcamerahalproxy_.enableCameraSolution(
                pAddon_->getEnabledSolutionList(std::move(deviceKey)));
            PLOGI("Enable camera solution : %d", result);
        }
    }
    else
    {
        PLOGI("streaming or preview already started by other app \n");
    }

    // Add to the device handle vector that accesses shared memory
    nstreaminghandle_.push_back(devhandle);

    // update state of device to preview
    obj_devstate.ecamstate_       = CameraDeviceState::CAM_DEVICE_STATE_STREAMING;
    obj_devstate.shmemtype        = memtype;
    virtualhandle_map_[devhandle] = obj_devstate;

    *pkey = shmkey_;
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::stopCamera(int devhandle)
{
    PLOGI("devhandle : %d \n", devhandle);

    // Get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    // Check if device is opened
    if (!DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        PLOGE("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }

    // Check if device state is streaming before stopping the preview
    if (obj_devstate.ecamstate_ < CameraDeviceState::CAM_DEVICE_STATE_STREAMING)
    {
        PLOGE("Camera State : %d \n", (int)obj_devstate.ecamstate_);
        return DEVICE_ERROR_INVALID_STATE;
    }

    std::vector<int>::iterator position =
        std::find(nstreaminghandle_.begin(), nstreaminghandle_.end(), devhandle);
    if (position == nstreaminghandle_.end())
    {
        PLOGE("not a streaming handle or device has already called stopCamera\n");
        return DEVICE_ERROR_NODEVICE;
    }

    unsigned long streaming_handle_size = nstreaminghandle_.size();
    PLOGI("Streaming handle size : %lu", streaming_handle_size);

    if (0 == streaming_handle_size)
    {
        PLOGE("device has already stopped streaming or preview\n");
        return DEVICE_ERROR_NODEVICE;
    }

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    if (1 == streaming_handle_size) // Primary
    {
        // stop preview
        ret = objcamerahalproxy_.stopPreview();
        objcamerahalproxy_.unsubscribe();

        if (DEVICE_OK != ret)
        {
            PLOGE("stopPreview error : %d", ret);
            return ret;
        }
    }

    // remove the handle from vector since stopCamera is called
    nstreaminghandle_.erase(position);

    // update state of device to open
    obj_devstate.ecamstate_       = CameraDeviceState::CAM_DEVICE_STATE_OPEN;
    obj_devstate.shmemtype        = "";
    virtualhandle_map_[devhandle] = obj_devstate;

    shmkey_ = 0;

    PLOGI("ok");
    return ret;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::startPreview(int devhandle, std::string memtype,
                                                        int *pkey, std::string windowid,
                                                        std::string *pmedia, LSHandle *sh,
                                                        const char *subskey)
{
    if (windowid.empty())
    {
        PLOGE("windowId is empty!");
        return DEVICE_ERROR_INVALID_WINDOW_ID;
    }

    PLOGI("devhandle : %d \n", devhandle);

    DEVICE_RETURN_CODE_T ret = startCamera(devhandle, memtype, pkey, sh, subskey);
    if (DEVICE_OK != ret)
    {
        PLOGE("startCamera error : %d", ret);
        return ret;
    }

    bool result = startPreviewDisplay(devhandle, std::move(windowid), std::move(memtype), shmkey_);
    if (!result)
    {
        stopCamera(devhandle);
        PLOGE("Fail to preview due to invalid windowId\n");
        return DEVICE_ERROR_INVALID_WINDOW_ID;
    }
    *pmedia = "undefined";

    // update state of device to preview
    DeviceStateMap obj_devstate   = virtualhandle_map_[devhandle];
    obj_devstate.ecamstate_       = CameraDeviceState::CAM_DEVICE_STATE_PREVIEW;
    virtualhandle_map_[devhandle] = obj_devstate;

    PLOGI("ok");
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::stopPreview(int devhandle)
{
    PLOGI("devhandle : %d \n", devhandle);

    if (!stopPreviewDisplay(devhandle))
    {
        PLOGE("not a previewing handle or already called stopPreview\n");
        return DEVICE_ERROR_INVALID_STATE;
    }

    DEVICE_RETURN_CODE_T ret = stopCamera(devhandle);
    if (DEVICE_OK != ret)
    {
        PLOGE("stopCamera error : %d", ret);
        return ret;
    }

    PLOGI("ok");
    return DEVICE_OK;
}

void VirtualDeviceManager::updateFormat(CAMERA_FORMAT &sformat, int devhandle)
{
    PLOGI("start!");
    // check if there is any change in format
    if ((sformat.eFormat != sformat_.eFormat) || (sformat.nHeight != sformat_.nHeight) ||
        (sformat.nWidth != sformat_.nWidth))
    {
        // check if the app requesting startCapture is secondary then do not change settings
        std::string priority = getAppPriority(devhandle);
        PLOGI("priority : %s\n", priority.c_str());
        if (cstr_primary != priority)
        {
            // check if there exists any app with primary priority
            if (true == checkAppPriorityMap())
            {
                PLOGI("Already an app exists with primary priority\n");
                sformat.eFormat = sformat_.eFormat;
                sformat.nHeight = sformat_.nHeight;
                sformat.nWidth  = sformat_.nWidth;
            }
        }
        else
        {
            PLOGI("Save the format\n");
            sformat_.eFormat = sformat.eFormat;
            sformat_.nHeight = sformat.nHeight;
            sformat_.nWidth  = sformat.nWidth;
        }
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::singleCapture(int devhandle, CAMERA_FORMAT sformat,
                                                         const std::string &imagepath,
                                                         const std::string &mode, int ncount)
{
    PLOGI("devhandle : %d ncount : %d \n", devhandle, ncount);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    // check if there is any change in format
    updateFormat(sformat, devhandle);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // capture number of images specified by ncount
        DEVICE_RETURN_CODE_T ret =
            objcamerahalproxy_.startCapture(sformat, imagepath, mode, ncount);
        return ret;
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::continuousCapture(int devhandle, CAMERA_FORMAT sformat,
                                                             const std::string &imagepath)
{
    PLOGI("devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    // check if there is any change in format
    updateFormat(sformat, devhandle);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        if (!bcaptureinprogress_)
        {
            // start capture
            DEVICE_RETURN_CODE_T ret =
                objcamerahalproxy_.startCapture(sformat, imagepath, cstr_continuous, 0, devhandle);
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
            PLOGI("capture already in progress\n");
            // add to vector the app
            ncapturehandle_.push_back(devhandle);
            return DEVICE_OK;
        }
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::startCapture(int devhandle, CAMERA_FORMAT sformat,
                                                        const std::string &imagepath,
                                                        const std::string &mode, int ncount)
{
    PLOGI("mode : %s", mode.c_str());

    if (mode == cstr_continuous)
        return continuousCapture(devhandle, sformat, imagepath);
    else
        return singleCapture(devhandle, sformat, imagepath, mode, ncount);
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::stopCapture(int devhandle, bool request)
{
    PLOGI("devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    if (!DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        PLOGE("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }

    unsigned long size = ncapturehandle_.size();
    PLOGI("size : %lu \n", size);

    if (0 == size)
    {
        PLOGE("device has already stopped capture\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
    }

    std::vector<int>::iterator position =
        std::find(ncapturehandle_.begin(), ncapturehandle_.end(), devhandle);
    if (position == ncapturehandle_.end())
    {
        PLOGE("device has already stopped capture\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
    }

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    if (1 == size)
    {
        // stop capture
        if (request)
            ret = objcamerahalproxy_.stopCapture(devhandle);

        // reset capture parameters for camera device
        if (DEVICE_OK == ret)
        {
            bcaptureinprogress_ = false;
        }
    }

    // remove the handle from vector since stopPreview is called
    ncapturehandle_.erase(position);

    return ret;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::capture(int devhandle, int ncount,
                                                   const std::string &imagepath,
                                                   std::vector<std::string> &capturedFiles)
{
    PLOGI("devhandle : %d ncount : %d \n", devhandle, ncount);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // check if device state is preview then only allow to capture
        if (CameraDeviceState::CAM_DEVICE_STATE_PREVIEW != obj_devstate.ecamstate_)
        {
            PLOGE("Invalid camera state : %d \n", (int)obj_devstate.ecamstate_);
            return DEVICE_ERROR_INVALID_STATE;
        }

        // capture number of images specified by ncount
        return objcamerahalproxy_.capture(ncount, imagepath, capturedFiles);
    }
    else
    {
        PLOGE("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::getProperty(int devhandle,
                                                       CAMERA_PROPERTIES_T *devproperty)
{
    PLOGI("devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get property of device opened
        DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.getDeviceProperty(devproperty);
        return ret;
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::setProperty(int devhandle, CAMERA_PROPERTIES_T *oInfo)
{
    PLOGI("devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    // check if the app requesting setProperty is secondary then do not change settings
    std::string priority = getAppPriority(devhandle);
    PLOGI("priority : %s\n", priority.c_str());
    if (cstr_primary != priority)
    {
        PLOGI("Check if any primary app exists\n");
        // check if there exists any app with primary priority
        if (true == checkAppPriorityMap())
        {
            PLOGI("Cannot change property as not a primary app\n");
            return DEVICE_ERROR_CAN_NOT_SET;
        }
    }

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // set device properties
        DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.setDeviceProperty(oInfo);
        return ret;
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::setFormat(int devhandle, CAMERA_FORMAT oformat)
{
    PLOGI("devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    // check if the app requesting setFormat is secondary then do not change settings
    std::string priority = getAppPriority(devhandle);
    PLOGI("priority : %s\n", priority.c_str());
    if (cstr_primary != priority)
    {
        // check if there exists any app with primary priority
        if (true == checkAppPriorityMap())
        {
            PLOGI("Cannot change format as not a primary app\n");
            return DEVICE_ERROR_CAN_NOT_SET;
        }
    }

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // set format
        DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.setFormat(oformat);
        if (DEVICE_OK == ret)
        {
            sformat_.eFormat = oformat.eFormat;
            sformat_.nHeight = oformat.nHeight;
            sformat_.nWidth  = oformat.nWidth;
        }
        return ret;
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::getFormat(int devhandle, CAMERA_FORMAT *oformat)
{
    PLOGI("devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get format of device
        DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.getFormat(oformat);
        return ret;
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::getFd(int devhandle, int *shmfd)
{
    PLOGI("devhandle : %d\n", devhandle);

    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];

    if (obj_devstate.ecamstate_ == CameraDeviceState::CAM_DEVICE_STATE_STREAMING ||
        obj_devstate.ecamstate_ == CameraDeviceState::CAM_DEVICE_STATE_PREVIEW)
    {
        if (obj_devstate.shmemtype == kMemtypePosixshm)
        {
            DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.getFd(shmfd);
            if (ret == DEVICE_OK)
            {
                PLOGI("posix shared memory fd is : %d\n", *shmfd);
            }
            return ret;
        }
        else
        {
            PLOGI("handle is not posix shared memory \n");
            return DEVICE_ERROR_NOT_POSIXSHM;
        }
    }
    else
    {
        PLOGI("Camera State : %d \n", (int)obj_devstate.ecamstate_);
        return DEVICE_ERROR_INVALID_STATE;
    }
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::registerClient(int n_client_pid, int n_client_sig,
                                                          int devhandle, std::string &outmsg)
{
    return objcamerahalproxy_.registerClient((pid_t)n_client_pid, n_client_sig, devhandle, outmsg);
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::unregisterClient(int n_client_pid, std::string &outmsg)
{
    return objcamerahalproxy_.unregisterClient((pid_t)n_client_pid, outmsg);
}

bool VirtualDeviceManager::isRegisteredClient(int devhandle)
{
    return objcamerahalproxy_.isRegisteredClient(devhandle);
}

void VirtualDeviceManager::requestPreviewCancel() { objcamerahalproxy_.requestPreviewCancel(); }

DEVICE_RETURN_CODE_T
VirtualDeviceManager::getSupportedCameraSolutionInfo(int devhandle,
                                                     std::vector<std::string> &solutionsInfo)
{
    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get supported solutions of device opened
        DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.getSupportedCameraSolutionInfo(solutionsInfo);
        return ret;
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T
VirtualDeviceManager::getEnabledCameraSolutionInfo(int devhandle,
                                                   std::vector<std::string> &solutionsInfo)
{
    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get enabled solutions of device opened
        DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.getEnabledCameraSolutionInfo(solutionsInfo);
        return ret;
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T
VirtualDeviceManager::enableCameraSolution(int devhandle, const std::vector<std::string> &solutions)
{
    PLOGI("VirtualDeviceManager enableCameraSolutionInfo E\n");

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get enabled solutions of device opened
        DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.enableCameraSolution(solutions);
        if (ret == DEVICE_OK)
        {
            // Attach platform-specific private component to device in order to enforce
            // platform-specific policy
            if (pAddon_ && pAddon_->hasImplementation())
            {
                std::string deviceKey = DeviceManager::getInstance().getDeviceKey(deviceid);
                pAddon_->notifySolutionEnabled(std::move(deviceKey), solutions);
            }
        }

        return ret;
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T
VirtualDeviceManager::disableCameraSolution(int devhandle,
                                            const std::vector<std::string> &solutions)
{
    PLOGI("VirtualDeviceManager disableCameraSolutionInfo E\n");

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PLOGI("deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get disabled solutions of device opened
        DEVICE_RETURN_CODE_T ret = objcamerahalproxy_.disableCameraSolution(solutions);

        if (ret == DEVICE_OK)
        {
            // Detach platform-specific private component from device used for platform-specific
            // policy application
            if (pAddon_ && pAddon_->hasImplementation())
            {
                std::string deviceKey = DeviceManager::getInstance().getDeviceKey(deviceid);
                pAddon_->notifySolutionDisabled(std::move(deviceKey), solutions);
            }
        }

        return ret;
    }
    else
    {
        PLOGI("Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

bool VirtualDeviceManager::startPreviewDisplay(int handle, std::string window_id,
                                               std::string mem_type, int key)
{
    std::string priority = getAppPriority(handle);
    PLOGI("priority : %s", priority.c_str());

    auto pdc = std::make_unique<PreviewDisplayControl>(window_id);

    std::string camera_id =
        "camera" + std::to_string(CommandManager::getInstance().getCameraId(handle));
    CAMERA_FORMAT camera_format;
    getFormat(handle, &camera_format);
    bool ret = pdc->start(std::move(camera_id), std::move(window_id), camera_format,
                          std::move(mem_type), key, handle, (cstr_primary == priority));
    if (ret)
    {
        std::string outmsg;
        registerClient(pdc->getPid(), 10, handle, outmsg);
        PLOGI("outmsg : %s", outmsg.c_str());

        previewDisplayControls.push_back(std::move(pdc));
    }
    return ret;
}

bool VirtualDeviceManager::stopPreviewDisplay(int handle)
{
    for (auto it = previewDisplayControls.begin(); it != previewDisplayControls.end(); ++it)
    {
        if ((*it)->getHandle() == handle)
        {
            bool ret = (*it)->stop();

            std::string outmsg;
            unregisterClient((*it)->getPid(), outmsg);
            PLOGI("outmsg : %s", outmsg.c_str());

            previewDisplayControls.erase(it);
            return ret;
        }
    }
    return false;
}

CameraDeviceState VirtualDeviceManager::getDeviceState(int devhandle)
{
    for (auto it = virtualhandle_map_.begin(); it != virtualhandle_map_.end(); ++it)
    {
        if (devhandle == it->first)
        {
            DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
            return obj_devstate.ecamstate_;
        }
    }
    return CameraDeviceState::CAM_DEVICE_STATE_CLOSE;
}