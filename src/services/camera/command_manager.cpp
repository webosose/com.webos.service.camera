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
#define LOG_TAG "CommandManager"
#include "command_manager.h"
#include "addon.h"
#include "camera_constants.h"
#include "device_manager.h"
#ifdef DAC_ENABLED
#include "camera_dac_policy.h"
#endif

std::shared_ptr<VirtualDeviceManager> CommandManager::getVirtualDeviceMgrObj(int devhandle)
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
            PLOGI("ptr : %p \n", obj.ptr.get());
            virtualdevmgrobj_map_.erase(it);
            return;
        }
    }
}

DEVICE_RETURN_CODE_T CommandManager::open(int deviceid, int *devicehandle, std::string appId,
                                          std::string apppriority)
{
    PLOGI("deviceid : %d \n", deviceid);

    std::string devicenode;
    DeviceManager::getInstance().getDeviceNode(deviceid, devicenode);
    PLOGI("devicenode : %s \n", devicenode.c_str());

    std::multimap<std::string, Device>::iterator it;
    it = virtualdevmgrobj_map_.find(devicenode);

    Device obj;
    if (it == virtualdevmgrobj_map_.end())
    {
        obj.ptr = std::make_shared<VirtualDeviceManager>();
    }
    else
        obj = it->second;

    PLOGI("ptr : %p \n", obj.ptr.get());

    if (nullptr != obj.ptr)
    {
        obj.ptr->setAddon(pAddon_);

        // open device and return devicehandle
        DEVICE_RETURN_CODE_T ret = obj.ptr->open(deviceid, devicehandle, appId, apppriority);
        if (DEVICE_OK == ret)
        {
            PLOGI("devicehandle : %d \n", *devicehandle);
            obj.devicehandle = *devicehandle;
            obj.deviceid     = deviceid;
            obj.clientName   = "";
            virtualdevmgrobj_map_.insert(std::make_pair(devicenode, obj));

            if (pAddon_ && pAddon_->hasImplementation())
            {
                std::string deviceKey = DeviceManager::getInstance().getDeviceKey(deviceid);
                bool res = pAddon_->notifyDeviceOpened(std::move(deviceKey), std::move(appId),
                                                       std::move(apppriority));
                PLOGI("AddOn::notifyDeviceOpened = %d ", res);
            }
        }
        else
        {
            PLOGE("open fail");
        }
        return ret;
    }
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::close(int devhandle)
{
    PLOGI("devhandle : %d \n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    PLOGI("ptr : %p \n", ptr.get());

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
    PLOGI("deviceid : %d\n", deviceid);

    if (n_invalid_id == deviceid)
        return DEVICE_ERROR_WRONG_PARAM;

    // get info of device requested
    DEVICE_RETURN_CODE_T ret = DeviceManager::getInstance().getInfo(deviceid, pinfo);
    if (DEVICE_OK != ret)
    {
        PLOGE("Failed to get device info\n");
    }

    return ret;
}

DEVICE_RETURN_CODE_T CommandManager::getDeviceList(std::vector<int> &idList)
{
    PLOGI("started!");

    // get list of devices connected
    return DeviceManager::getInstance().getDeviceIdList(idList);
}

DEVICE_RETURN_CODE_T CommandManager::getProperty(int devhandle, CAMERA_PROPERTIES_T *devproperty)
{
    PLOGI("devhandle : %d\n", devhandle);

    if ((n_invalid_id == devhandle) || (NULL == devproperty))
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
        // send request to get property of device
        return ptr->getProperty(devhandle, devproperty);
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::setProperty(int devhandle, CAMERA_PROPERTIES_T *oInfo)
{
    PLOGI("devhandle : %d\n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
        // send request to set property of device
        return ptr->setProperty(devhandle, oInfo);
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::setFormat(int devhandle, CAMERA_FORMAT oformat)
{
    PLOGI("devhandle : %d \n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
        // send request to set format of device
        return ptr->setFormat(devhandle, oformat);
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::startCamera(int devhandle, LSHandle *sh)
{
    PLOGI("devhandle : %d\n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
        // start preview
        return ptr->startCamera(devhandle, sh);
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::stopCamera(int devhandle, bool forceComplete)
{
    PLOGI("devhandle : %d\n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
        // stop preview
        return ptr->stopCamera(devhandle, forceComplete);
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::startPreview(int devhandle, std::string disptype, LSHandle *sh)
{
    PLOGI("devhandle : %d\n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
        // start preview
        return ptr->startPreview(devhandle, std::move(disptype), sh);
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::stopPreview(int devhandle, bool forceComplete)
{
    PLOGI("devhandle : %d\n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
        // stop preview
        return ptr->stopPreview(devhandle, forceComplete);
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::startCapture(int devhandle, CAMERA_FORMAT sformat,
                                                  const std::string &imagepath,
                                                  const std::string &mode, int ncount, int userid)
{
    PLOGI("devhandle : %d\n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::string capture_path = imagepath;
#if DAC_ENABLED
    if (!CameraDacPolicy::getInstance().checkCredential(userid, capture_path))
    {
        return DEVICE_ERROR_DAC_POLICY_VIOLATION;
    }
    PLOGI("capture_path: %s", capture_path.c_str());

    CameraDacPolicy::getInstance().apply(userid);
#endif

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
    {
        // capture image
        return ptr->startCapture(devhandle, sformat, capture_path, mode, ncount);
    }
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::stopCapture(int devhandle, bool request)
{
    PLOGI("devhandle : %d\n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
        // stop capture
        return ptr->stopCapture(devhandle, request);
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::capture(int devhandle, int ncount,
                                             const std::string &imagepath,
                                             std::vector<std::string> &capturedFiles, int userid)
{
    PLOGI("devhandle : %d\n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::string capture_path = imagepath;
#if DAC_ENABLED
    if (capture_path.empty())
        capture_path = cstr_capturedir;

    if (!CameraDacPolicy::getInstance().checkCredential(userid, capture_path))
    {
        return DEVICE_ERROR_DAC_POLICY_VIOLATION;
    }
    PLOGI("capture_path: %s", capture_path.c_str());

    CameraDacPolicy::getInstance().apply(userid);
#endif

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
    {
        // capture image
        return ptr->capture(devhandle, ncount, capture_path, capturedFiles);
    }
    else
        return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T CommandManager::getFormat(int devhandle, CAMERA_FORMAT *oformat)
{
    PLOGI("devhandle : %d\n", devhandle);

    if (n_invalid_id == devhandle)
        return DEVICE_ERROR_WRONG_PARAM;

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
        // get format
        return ptr->getFormat(devhandle, oformat);
    else
        return DEVICE_ERROR_UNKNOWN;
}

int CommandManager::getCameraId(int devhandle)
{
    PLOGI("devhandle : %d\n", devhandle);
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

int CommandManager::getCameraHandle(int devid)
{
    PLOGI("devid : %d\n", devid);

    std::multimap<std::string, Device>::iterator it;
    for (it = virtualdevmgrobj_map_.begin(); it != virtualdevmgrobj_map_.end(); ++it)
    {
        Device obj = it->second;
        if (devid == obj.deviceid)
            return obj.devicehandle;
    }
    return n_invalid_id;
}

DEVICE_RETURN_CODE_T CommandManager::getFd(int devhandle, const std::string &type, int *shmfd)
{
    PLOGI("devhandle : %d\n", devhandle);

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
    {
        return ptr->getFd(devhandle, type, shmfd);
    }
    return DEVICE_ERROR_HANDLE_NOT_EXIST;
}

bool CommandManager::setClientDevice(int devhandle, std::string clientName)
{
    PLOGI("devhandle : %d\n", devhandle);
    if (n_invalid_id == devhandle)
        return false;

    std::multimap<std::string, Device>::iterator it;
    for (it = virtualdevmgrobj_map_.begin(); it != virtualdevmgrobj_map_.end(); ++it)
    {
        if (devhandle == it->second.devicehandle)
        {
            it->second.clientName = clientName;
            PLOGI("devicehandle : %d, deviceid : %d, clientName : %s", it->second.devicehandle,
                  it->second.deviceid, it->second.clientName.c_str());
            return true;
        }
    }
    return false;
}

DEVICE_RETURN_CODE_T CommandManager::checkDeviceClient(int devhandle, std::string clientID)
{
    std::multimap<std::string, Device>::iterator it;
    for (it = virtualdevmgrobj_map_.begin(); it != virtualdevmgrobj_map_.end(); ++it)
    {
        if (devhandle == it->second.devicehandle)
        {
            // if device is opened from luna-send, the client won't be in watcher list
            if (it->second.clientName.length() == 0)
                return DEVICE_OK;

            if (clientID == it->second.clientName)
                return DEVICE_OK;
            else
                return DEVICE_ERROR_HANDLE_NOT_EXIST;
        }
    }
    return DEVICE_ERROR_HANDLE_NOT_EXIST;
}

void CommandManager::closeClientDevice(std::string clientName)
{
    PLOGI("clientName : %s", clientName.c_str());

    auto it = virtualdevmgrobj_map_.begin();
    while (it != virtualdevmgrobj_map_.end())
    {
        Device &device = it->second;

        if (clientName == device.clientName && nullptr != device.ptr)
        {
            PLOGI("stop & close devicehandle : %d", device.devicehandle);

            stopAndCloseDevice(device);

            it = virtualdevmgrobj_map_.erase(it);
        }
        else
        {
            it++;
        }
    }
}

void CommandManager::handleCrash()
{
    PLOGI("start freeing resources for abnormal service termination \n");

    auto it = virtualdevmgrobj_map_.begin();
    while (it != virtualdevmgrobj_map_.end())
    {
        Device &device = it->second;

        stopAndCloseDevice(device);

        it = virtualdevmgrobj_map_.erase(it);
    }

    PLOGI("end freeing resources for abnormal service termination \n");
}

void CommandManager::release(int deviceid)
{
    PLOGI("deviceid : %d", deviceid);

    auto it = virtualdevmgrobj_map_.begin();
    while (it != virtualdevmgrobj_map_.end())
    {
        Device &device = it->second;
        if (deviceid == device.deviceid)
        {
            stopAndCloseDevice(device);
            it = virtualdevmgrobj_map_.erase(it);
        }
        else
        {
            it++;
        }
    }
}

DEVICE_RETURN_CODE_T
CommandManager::getSupportedCameraSolutionInfo(int devhandle,
                                               std::vector<std::string> &solutionsInfo)
{
    PLOGI("getSupportedCameraSolutionInfo : devhandle : %d\n", devhandle);

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
    {
        return ptr->getSupportedCameraSolutionInfo(devhandle, solutionsInfo);
    }
    else
    {
        return DEVICE_ERROR_UNKNOWN;
    }
}

DEVICE_RETURN_CODE_T
CommandManager::getEnabledCameraSolutionInfo(int devhandle, std::vector<std::string> &solutionsInfo)
{
    PLOGI("getSupportedCameraSolutionInfo : devhandle : %d\n", devhandle);

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
    {
        return ptr->getEnabledCameraSolutionInfo(devhandle, solutionsInfo);
    }
    else
    {
        return DEVICE_ERROR_UNKNOWN;
    }
}

DEVICE_RETURN_CODE_T CommandManager::enableCameraSolution(int devhandle,
                                                          const std::vector<std::string> &solutions)
{
    PLOGI("enableCameraSolutionInfo : devhandle : %d\n", devhandle);

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
    {
        return ptr->enableCameraSolution(devhandle, solutions);
    }
    else
    {
        return DEVICE_ERROR_UNKNOWN;
    }
}

DEVICE_RETURN_CODE_T
CommandManager::disableCameraSolution(int devhandle, const std::vector<std::string> &solutions)
{
    PLOGI("enableCameraSolutionInfo : devhandle : %d\n", devhandle);

    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
    {
        return ptr->disableCameraSolution(devhandle, solutions);
    }
    else
    {
        return DEVICE_ERROR_UNKNOWN;
    }
}

CameraDeviceState CommandManager::getDeviceState(int devhandle)
{
    std::shared_ptr<VirtualDeviceManager> ptr = getVirtualDeviceMgrObj(devhandle);
    if (nullptr != ptr)
    {
        return ptr->getDeviceState(devhandle);
    }
    else
    {
        return CameraDeviceState::CAM_DEVICE_STATE_CLOSE;
    }
}

void CommandManager::stopAndCloseDevice(Device &device)
{
    // send request to stop all and close the device
    device.ptr->stopCapture(device.devicehandle);

    CameraDeviceState state = getDeviceState(device.devicehandle);
    if (state == CameraDeviceState::CAM_DEVICE_STATE_STREAMING)
    {
        stopCamera(device.devicehandle, true);
    }
    else if (state == CameraDeviceState::CAM_DEVICE_STATE_PREVIEW)
    {
        stopPreview(device.devicehandle, true);
    }

    device.ptr->close(device.devicehandle);
}
