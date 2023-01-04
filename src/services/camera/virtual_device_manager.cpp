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
#include "virtual_device_manager.h"
#include "addon.h" // platform-specific functionality extension support
#include "camera_constants.h"
#include "device_manager.h"

VirtualDeviceManager::VirtualDeviceManager()
    : virtualhandle_map_(), handlepriority_map_(), shmempreview_count_{},
      bcaptureinprogress_(false), shmkey_(0), poshmkey_(0), sformat_()
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
        {
            PMLOG_INFO(CONST_MODULE_VDM, "devhandle = %d, mode = %s", it->first,
                       it->second.c_str());
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
    PMLOG_INFO(CONST_MODULE_VDM, "devid: %d, virtual_devhandle:%d, ndeviceid_:%d", devid,
               virtual_devhandle, obj_devstate.ndeviceid_);
    DeviceManager::getInstance().addVirtualHandle(devid, virtual_devhandle);
    PMLOG_INFO(CONST_MODULE_VDM, "virtualhandle_map_.size = %d", virtualhandle_map_.size());
    return virtual_devhandle;
}

void VirtualDeviceManager::removeVirtualDeviceHandle(int devhandle)
{
    // remove virtual device handle key value from map
    int devid = virtualhandle_map_[devhandle].ndeviceid_;
    DeviceManager::getInstance().eraseVirtualHandle(devid, devhandle);
    virtualhandle_map_.erase(devhandle);
    PMLOG_INFO(CONST_MODULE_VDM, "virtualhandle_map_.size = %d", virtualhandle_map_.size());
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

    std::string deviceType   = DeviceManager::getInstance().getDeviceType(devid);
    DEVICE_RETURN_CODE_T ret = objdevicecontrol_.createHandle(&p_cam_handle, deviceType);
    if (DEVICE_OK != ret)
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Failed to create handle\n");
        return DEVICE_ERROR_CAN_NOT_OPEN;
    }

    DeviceManager::getInstance().updateHandle(devid, p_cam_handle);
    std::string devnode;
    // get the device node of requested camera to be opened
    DeviceManager::getInstance().getDeviceNode(devid, devnode);
    PMLOG_INFO(CONST_MODULE_VDM, "devnode : %s \n", devnode.c_str());

    void *handle;
    DeviceManager::getInstance().getDeviceHandle(devid, &handle);
    PMLOG_INFO(CONST_MODULE_VDM, "handle : %p \n", handle);

    // open the camera here
    ret = objdevicecontrol_.open(handle, devnode, devid);
    if (DEVICE_OK != ret)
        PMLOG_INFO(CONST_MODULE_VDM, "Failed to open device\n");
    else
        DeviceManager::getInstance().setDeviceStatus(devid, TRUE);

    // get virtual device handle for device opened
    *devhandle = getVirtualDeviceHandle(devid);

    return ret;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::open(int devid, int *devhandle, std::string appId,
                                                std::string apppriority)
{
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", devid);

    // check if priortiy is not set by user
    if (cstr_empty == apppriority)
    {
        PMLOG_INFO(CONST_MODULE_VDM, "empty app priority\n");
        if (true == checkAppPriorityMap())
        {
            PMLOG_INFO(CONST_MODULE_VDM, "Already an app is registered as primary\n");
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
            PMLOG_INFO(CONST_MODULE_VDM, "Already an app has registered as primary device\n");
            return DEVICE_ERROR_ALREADY_OEPENED_PRIMARY_DEVICE;
        }
    }

    // check if camera device requested to open is valid
    if (!DeviceManager::getInstance().isDeviceValid(devid))
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device is invalid\n");
        return DEVICE_ERROR_NODEVICE;
    }

    // check if camera device requested to open is not already opened
    if (DeviceManager::getInstance().isDeviceOpen(devid))
    {
        // device is already opened, hence return virtual handle for device
        *devhandle = getVirtualDeviceHandle(devid);
        PMLOG_INFO(CONST_MODULE_VDM, "Device is already opened! Handle : %d \n", *devhandle);
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
        if (AddOn::hasImplementation())
        {
            if (apppriority == cstr_primary)
            {
                AddOn::logMessagePrivate(appId);
            }
        }
    }
    return ret;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::close(int devhandle)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d \n", devhandle);

    // check if app has already closed device
    if (false == checkDeviceOpen(devhandle))
    {
        // app already closed device
        PMLOG_INFO(CONST_MODULE_VDM, "App has already closed the device\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
    }

    // get number of elements in map
    int nelements = handlepriority_map_.size();
    PMLOG_INFO(CONST_MODULE_VDM, "nelements : %d \n", nelements);

    if (1 <= nelements)
    {
        // if there are elements in the map, get device id for device handle
        DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
        int deviceid                = obj_devstate.ndeviceid_;
        PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

        // check if device state is open then only allow to close
        if (CameraDeviceState::CAM_DEVICE_STATE_OPEN != obj_devstate.ecamstate_)
        {
            PMLOG_INFO(CONST_MODULE_VDM, "Camera State : %d \n", (int)obj_devstate.ecamstate_);
            return DEVICE_ERROR_CAN_NOT_CLOSE;
        }

        // check if device is opened
        if (DeviceManager::getInstance().isDeviceOpen(deviceid))
        {
            DEVICE_RETURN_CODE_T ret = DEVICE_OK;

            void *handle;
            DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
            PMLOG_INFO(CONST_MODULE_VDM, "handle : %p \n", handle);

            // close the device only if its the last app to request for close
            if (1 == nelements)
            {
                // close the actual device
                ret = objdevicecontrol_.close(handle);
                if (DEVICE_OK == ret)
                {
                    DeviceManager::getInstance().setDeviceStatus(deviceid, FALSE);
                    ret = objdevicecontrol_.destroyHandle(handle);
                    DeviceManager::getInstance().updateHandle(deviceid, nullptr);
                    // remove the virtual device
                    removeVirtualDeviceHandle(devhandle);
                    // since the device is closed, remove the element from map
                    removeHandlePriorityObj(devhandle);
                }
                else
                    PMLOG_ERROR(CONST_MODULE_VDM, "Failed to close device\n");
            }
            else
            {
                // remove the virtual device
                removeVirtualDeviceHandle(devhandle);
                // remove the app from map
                removeHandlePriorityObj(devhandle);
            }

            return ret;
        }
        else
        {
            // remove the virtual device
            removeVirtualDeviceHandle(devhandle);
            // remove the app from map
            removeHandlePriorityObj(devhandle);
            return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
        }
    }
    else
        return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::startPreview(int devhandle, std::string memtype,
                                                        int *pkey, LSHandle *sh,
                                                        const char *subskey)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d \n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    // check if device is opened
    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // check if device state is open then only allow to start preview
        if (CameraDeviceState::CAM_DEVICE_STATE_OPEN != obj_devstate.ecamstate_)
        {
            PMLOG_INFO(CONST_MODULE_VDM, "Camera State : %d \n", (int)obj_devstate.ecamstate_);
            return DEVICE_ERROR_CAN_NOT_START;
        }

        if (((memtype == kMemtypeShmem || memtype == kMemtypeShmemMmap) &&
             shmempreview_count_[SHMEM_SYSTEMV] == 0) ||
            (memtype == kMemtypePosixshm && shmempreview_count_[SHMEM_POSIX] == 0))
        {
            void *handle;
            DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
            // start preview
            DEVICE_RETURN_CODE_T ret =
                objdevicecontrol_.startPreview(handle, memtype, pkey, sh, subskey);
            if (DEVICE_OK == ret)
            {
                // Increament preview count by 1
                if (memtype == kMemtypeShmem || memtype == kMemtypeShmemMmap)
                {
                    obj_devstate.shmemtype = SHMEM_SYSTEMV;
                    shmempreview_count_[SHMEM_SYSTEMV]++;
                    shmkey_ = *pkey;
                }
                else
                {
                    obj_devstate.shmemtype = SHMEM_POSIX;
                    shmempreview_count_[SHMEM_POSIX]++;
                    poshmkey_ = *pkey;
                }
                // add to vector the app calling startPreview
                npreviewhandle_.push_back(devhandle);
                // update state of device to preview
                obj_devstate.ecamstate_       = CameraDeviceState::CAM_DEVICE_STATE_PREVIEW;
                virtualhandle_map_[devhandle] = obj_devstate;

                // Apply platform-specific policy to solutions if exists.
                if (AddOn::hasImplementation())
                {
                    ret = objdevicecontrol_.enableCameraSolution(
                        AddOn::getDevicePrivateData(deviceid));
                    if (DEVICE_OK != ret)
                        PMLOG_INFO(CONST_MODULE_VDM, "Failed to enable camera solution\n");
                }
            }
            return ret;
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_VDM, "preview already started by other app \n");
            if (memtype == kMemtypeShmem || memtype == kMemtypeShmemMmap)
                *pkey = shmkey_;
            else
                *pkey = poshmkey_;
            // add to vector the app calling startPreview
            npreviewhandle_.push_back(devhandle);
            // update state of device to preview
            obj_devstate.ecamstate_ = CameraDeviceState::CAM_DEVICE_STATE_PREVIEW;
            // Increament preview count by 1
            if (memtype == kMemtypeShmem || memtype == kMemtypeShmemMmap)
            {
                obj_devstate.shmemtype = SHMEM_SYSTEMV;
                shmempreview_count_[SHMEM_SYSTEMV]++;
            }
            else
            {
                obj_devstate.shmemtype = SHMEM_POSIX;
                shmempreview_count_[SHMEM_POSIX]++;
            }
            virtualhandle_map_[devhandle] = obj_devstate;
            return DEVICE_OK;
        }
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::stopPreview(int devhandle)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d \n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    int memtype                 = obj_devstate.shmemtype;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    // check if device is opened
    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // check if device state is preview then only allow to stop preview
        if (CameraDeviceState::CAM_DEVICE_STATE_PREVIEW != obj_devstate.ecamstate_)
        {
            PMLOG_INFO(CONST_MODULE_VDM, "Camera State : %d \n", (int)obj_devstate.ecamstate_);
            return DEVICE_ERROR_CAN_NOT_STOP;
        }

        int size = npreviewhandle_.size();
        PMLOG_INFO(CONST_MODULE_VDM, "size : %d \n", size);
        if (1 < shmempreview_count_[memtype])
        {
            // remove the handle from vector since stopPreview is called
            std::vector<int>::iterator position =
                std::find(npreviewhandle_.begin(), npreviewhandle_.end(), devhandle);
            if (position != npreviewhandle_.end())
            {
                npreviewhandle_.erase(position);
                // update state of device to open
                obj_devstate.ecamstate_       = CameraDeviceState::CAM_DEVICE_STATE_OPEN;
                obj_devstate.shmemtype        = SHMEME_UNKNOWN;
                virtualhandle_map_[devhandle] = obj_devstate;
                // Decreament preview count
                shmempreview_count_[memtype]--;
                return DEVICE_OK;
            }
            else
            {
                PMLOG_INFO(CONST_MODULE_VDM, "device has already called stopPreview\n");
                return DEVICE_ERROR_NODEVICE;
            }
        }
        else if (1 == shmempreview_count_[memtype])
        {
            std::vector<int>::iterator position =
                std::find(npreviewhandle_.begin(), npreviewhandle_.end(), devhandle);
            if (position != npreviewhandle_.end())
            {
                // last handle to call stopPreview
                void *handle;
                DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
                // stop preview
                DEVICE_RETURN_CODE_T ret = objdevicecontrol_.stopPreview(handle, memtype);
                // reset preview parameters for camera device
                if (DEVICE_OK == ret)
                {
                    // remove the handle from vector since stopPreview is called
                    npreviewhandle_.erase(position);
                    // update state of device to open
                    obj_devstate.ecamstate_       = CameraDeviceState::CAM_DEVICE_STATE_OPEN;
                    obj_devstate.shmemtype        = SHMEME_UNKNOWN;
                    virtualhandle_map_[devhandle] = obj_devstate;
                    shmempreview_count_[memtype]  = 0;
                    if (memtype == SHMEM_SYSTEMV)
                    {
                        shmkey_ = 0;
                    }
                    else
                    {
                        poshmkey_ = 0;
                    }
                }
                return ret;
            }
            else
            {
                PMLOG_INFO(CONST_MODULE_VDM, "device has already called stopPreview\n");
                return DEVICE_ERROR_NODEVICE;
            }
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_VDM, "device has already stopped preview\n");
            return DEVICE_ERROR_NODEVICE;
        }
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::captureImage(int devhandle, int ncount,
                                                        CAMERA_FORMAT sformat,
                                                        const std::string &imagepath,
                                                        const std::string &mode)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d ncount : %d \n", devhandle, ncount);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    // check if there is any change in format
    updateFormat(sformat, devhandle);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        void *handle;
        DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
        // capture number of images specified by ncount
        DEVICE_RETURN_CODE_T ret =
            objdevicecontrol_.captureImage(handle, ncount, sformat, imagepath, mode);
        return ret;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

void VirtualDeviceManager::updateFormat(CAMERA_FORMAT &sformat, int devhandle)
{
    PMLOG_INFO(CONST_MODULE_VDM, "start!");
    // check if there is any change in format
    if ((sformat.eFormat != sformat_.eFormat) || (sformat.nHeight != sformat_.nHeight) ||
        (sformat.nWidth != sformat_.nWidth))
    {
        // check if the app requesting startCapture is secondary then do not change settings
        std::string priority = getAppPriority(devhandle);
        PMLOG_INFO(CONST_MODULE_VDM, "priority : %s\n", priority.c_str());
        if (cstr_primary != priority)
        {
            // check if there exists any app with primary priority
            if (true == checkAppPriorityMap())
            {
                PMLOG_INFO(CONST_MODULE_VDM, "Already an app exists with primary priority\n");
                sformat.eFormat = sformat_.eFormat;
                sformat.nHeight = sformat_.nHeight;
                sformat.nWidth  = sformat_.nWidth;
            }
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_VDM, "Save the format\n");
            sformat_.eFormat = sformat.eFormat;
            sformat_.nHeight = sformat.nHeight;
            sformat_.nWidth  = sformat.nWidth;
        }
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::startCapture(int devhandle, CAMERA_FORMAT sformat,
                                                        const std::string &imagepath)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    // check if there is any change in format
    updateFormat(sformat, devhandle);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        if (!bcaptureinprogress_)
        {
            void *handle;
            DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
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
            PMLOG_INFO(CONST_MODULE_VDM, "capture already in progress\n");
            // add to vector the app
            ncapturehandle_.push_back(devhandle);
            return DEVICE_OK;
        }
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::stopCapture(int devhandle)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        int size = ncapturehandle_.size();

        PMLOG_INFO(CONST_MODULE_VDM, "size : %d \n", size);
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
                PMLOG_INFO(CONST_MODULE_VDM, "device has already stopped capture\n");
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
                DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
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
                PMLOG_INFO(CONST_MODULE_VDM, "device has already stopped capture\n");
                return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
            }
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_VDM, "device has already stopped capture\n");
            return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
        }
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::getProperty(int devhandle,
                                                       CAMERA_PROPERTIES_T *devproperty)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        void *handle;
        DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
        // get property of device opened
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.getDeviceProperty(handle, devproperty);
        return ret;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::setProperty(int devhandle, CAMERA_PROPERTIES_T *oInfo)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    // check if the app requesting setProperty is secondary then do not change settings
    std::string priority = getAppPriority(devhandle);
    PMLOG_INFO(CONST_MODULE_VDM, "priority : %s\n", priority.c_str());
    if (cstr_primary != priority)
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Check if any primary app exists\n");
        // check if there exists any app with primary priority
        if (true == checkAppPriorityMap())
        {
            PMLOG_INFO(CONST_MODULE_VDM, "Cannot change property as not a primary app\n");
            return DEVICE_ERROR_CAN_NOT_SET;
        }
    }

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        void *handle;
        DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
        // set device properties
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.setDeviceProperty(handle, oInfo);
        return ret;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::setFormat(int devhandle, CAMERA_FORMAT oformat)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    // check if the app requesting setFormat is secondary then do not change settings
    std::string priority = getAppPriority(devhandle);
    PMLOG_INFO(CONST_MODULE_VDM, "priority : %s\n", priority.c_str());
    if (cstr_primary != priority)
    {
        // check if there exists any app with primary priority
        if (true == checkAppPriorityMap())
        {
            PMLOG_INFO(CONST_MODULE_VDM, "Cannot change format as not a primary app\n");
            return DEVICE_ERROR_CAN_NOT_SET;
        }
    }

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        void *handle;
        DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
        // set format
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.setFormat(handle, oformat);
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
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::getFormat(int devhandle, CAMERA_FORMAT *oformat)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d\n", devhandle);

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        void *handle;
        DeviceManager::getInstance().getDeviceHandle(deviceid, &handle);
        // get format of device
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.getFormat(handle, oformat);
        return ret;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T VirtualDeviceManager::getFd(int devhandle, int *shmfd)
{
    PMLOG_INFO(CONST_MODULE_VDM, "devhandle : %d\n", devhandle);

    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];

    if (obj_devstate.ecamstate_ == CameraDeviceState::CAM_DEVICE_STATE_PREVIEW)
    {
        if (obj_devstate.shmemtype == SHMEM_POSIX)
        {
            *shmfd = poshmkey_;
            PMLOG_INFO(CONST_MODULE_VDM, "posix shared memory fd is : %d\n", *shmfd);
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_VDM, "handle is not posix shared memory \n");
            return DEVICE_ERROR_NOT_POSIXSHM;
        }
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device preview not started\n");
        return DEVICE_ERROR_PREVIEW_NOT_STARTED;
    }
    return DEVICE_OK;
}

bool VirtualDeviceManager::registerClient(int n_client_pid, int n_client_sig, int devhandle,
                                          std::string &outmsg)
{
    return objdevicecontrol_.registerClient((pid_t)n_client_pid, n_client_sig, devhandle, outmsg);
}

bool VirtualDeviceManager::unregisterClient(int n_client_pid, std::string &outmsg)
{
    return objdevicecontrol_.unregisterClient((pid_t)n_client_pid, outmsg);
}

bool VirtualDeviceManager::isRegisteredClient(int devhandle)
{
    return objdevicecontrol_.isRegisteredClient(devhandle);
}

void VirtualDeviceManager::requestPreviewCancel() { objdevicecontrol_.requestPreviewCancel(); }

DEVICE_RETURN_CODE_T
VirtualDeviceManager::getSupportedCameraSolutionInfo(int devhandle,
                                                     std::vector<std::string> &solutionsInfo)
{
    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get supported solutions of device opened
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.getSupportedCameraSolutionInfo(solutionsInfo);
        return ret;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
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
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get enabled solutions of device opened
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.getEnabledCameraSolutionInfo(solutionsInfo);
        return ret;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T
VirtualDeviceManager::enableCameraSolution(int devhandle, const std::vector<std::string> solutions)
{
    PMLOG_INFO(CONST_MODULE_VDM, "VirtualDeviceManager enableCameraSolutionInfo E\n");

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get enabled solutions of device opened
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.enableCameraSolution(solutions);
        if (ret == DEVICE_OK)
        {
            // Attach platform-specific private component to device in order to enforce
            // platform-specific policy
            AddOn::attachPrivateComponentToDevice(deviceid, solutions);
        }

        return ret;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T
VirtualDeviceManager::disableCameraSolution(int devhandle, const std::vector<std::string> solutions)
{
    PMLOG_INFO(CONST_MODULE_VDM, "VirtualDeviceManager disableCameraSolutionInfo E\n");

    // get device id for virtual device handle
    DeviceStateMap obj_devstate = virtualhandle_map_[devhandle];
    int deviceid                = obj_devstate.ndeviceid_;
    PMLOG_INFO(CONST_MODULE_VDM, "deviceid : %d \n", deviceid);

    if (DeviceManager::getInstance().isDeviceOpen(deviceid))
    {
        // get disabled solutions of device opened
        DEVICE_RETURN_CODE_T ret = objdevicecontrol_.disableCameraSolution(solutions);

        if (ret == DEVICE_OK)
        {
            // Detach platform-specific private component from device used for platform-specific
            // policy application
            AddOn::detachPrivateComponentFromDevice(deviceid, solutions);
        }

        return ret;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_VDM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}
