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
#define LOG_TAG "DeviceManager"
#include "device_manager.h"
#include "addon.h" /* calls platform specific functionality if addon interface has been implemented */
#include "camera_hal_proxy.h"
#include "command_manager.h"
#include "event_notification.h"
#include "whitelist_checker.h"

DeviceManager::DeviceManager() {}

bool DeviceManager::isDeviceIdValid(int deviceid)
{
    if (deviceMap_.find(deviceid) != deviceMap_.end())
        return true;
    return false;
}

bool DeviceManager::setDeviceStatus(int deviceid, bool status)
{
    PLOGI("deviceid %d status : %d \n!!", deviceid, status);

    if (!isDeviceIdValid(deviceid))
        return CONST_PARAM_VALUE_FALSE;

    deviceMap_[deviceid].isDeviceOpen = status;

    return CONST_PARAM_VALUE_TRUE;
}

bool DeviceManager::isDeviceOpen(int deviceid)
{
    if (!isDeviceIdValid(deviceid))
    {
        PLOGE("deviceid %d is an invalid ID", deviceid);
        return CONST_PARAM_VALUE_FALSE;
    }

    PLOGD("deviceMap_[%d].isDeviceOpen : %d", deviceid, deviceMap_[deviceid].isDeviceOpen);
    if (deviceMap_[deviceid].isDeviceOpen)
    {
        PLOGI("deviceid %d is open!!", deviceid);
        return CONST_PARAM_VALUE_TRUE;
    }
    else
    {
        PLOGI("deviceid %d is not open!!", deviceid);
        return CONST_PARAM_VALUE_FALSE;
    }
}

bool DeviceManager::isDeviceValid(int deviceid) { return isDeviceIdValid(deviceid); }

void DeviceManager::getDeviceNode(int deviceid, std::string &devicenode)
{
    PLOGI("deviceid : %d", deviceid);

    devicenode = "";
    if (isDeviceIdValid(deviceid))
    {
        devicenode = deviceMap_[deviceid].stList.strDeviceNode;
    }
}

std::string DeviceManager::getDeviceType(int deviceid)
{
    std::string deviceType = "unknown";
    if (isDeviceIdValid(deviceid))
    {
        deviceType = deviceMap_[deviceid].stList.strDeviceType;
    }
    return deviceType;
}

std::string DeviceManager::getDeviceKey(int deviceid)
{
    std::string deviceKey;
    if (isDeviceIdValid(deviceid))
    {
        deviceKey = deviceMap_[deviceid].stList.strDeviceKey;
    }
    return deviceKey;
}

int DeviceManager::getDeviceCounts(std::string type)
{
    int count = 0;
    for (auto iter : deviceMap_)
    {
        if (iter.second.stList.strDeviceType == type && count < INT_MAX)
        {
            ++count;
        }
    }

    return count;
}

bool DeviceManager::getDeviceUserData(int deviceid, std::string &userData)
{
    if (isDeviceIdValid(deviceid))
    {
        userData = deviceMap_[deviceid].stList.strUserData;
        return true;
    }
    return false;
}

DEVICE_RETURN_CODE_T DeviceManager::getDeviceIdList(std::vector<int> &idList)
{
    for (auto list : deviceMap_)
        idList.push_back(list.first);
    return DEVICE_OK;
}

int DeviceManager::addDevice(const DEVICE_LIST_T &deviceInfo)
{
    DEVICE_STATUS devStatus;
    devStatus.isDeviceOpen      = false;
    devStatus.isDeviceInfoSaved = false;
    devStatus.stList            = deviceInfo;
    PLOGI("strVendorName    : %s", deviceInfo.strVendorName.c_str());
    PLOGI("strProductName   : %s", deviceInfo.strProductName.c_str());
    PLOGI("strVendorID      : %s", deviceInfo.strVendorID.c_str());
    PLOGI("strProductID     : %s", deviceInfo.strProductID.c_str());
    PLOGI("strDeviceSubtype : %s", deviceInfo.strDeviceSubtype.c_str());
    PLOGI("strDeviceType    : %s", deviceInfo.strDeviceType.c_str());
    PLOGI("nDeviceNum       : %d", deviceInfo.nDeviceNum);
    PLOGI("nPortNum         : %d", deviceInfo.nPortNum);
    PLOGI("isPowerOnConnect : %d", deviceInfo.isPowerOnConnect);
    PLOGI("strDeviceNode    : %s", deviceInfo.strDeviceNode.c_str());
    PLOGI("strDeviceKey     : %s", deviceInfo.strDeviceKey.c_str());

    // Assign a new deviceid
    int deviceid = 0;
    for (int i = 1; i <= MAX_DEVICE_COUNT; i++)
    {
        if (!isDeviceIdValid(i))
        {
            deviceid = i;
            break;
        }
    }
    if (deviceid == 0)
        return 0;

    deviceMap_[deviceid] = devStatus;
    PLOGI("deviceid : %d, deviceMap_.size : %zd \n", deviceid, deviceMap_.size());

    // Push platform-specific device private data associated with this device */
    if (pAddon_ && pAddon_->hasImplementation())
        pAddon_->notifyDeviceAdded(&deviceInfo);

    if (!pAddon_ || false == pAddon_->hasImplementation())
    {
        WhitelistChecker::check(deviceInfo.strProductName, deviceInfo.strVendorName);
    }

    if (lshandle_)
    {
        EventNotification obj;
        obj.eventReply(lshandle_, CONST_EVENT_KEY_CAMERA_LIST, EventType::EVENT_TYPE_CONNECT);
    }

    return deviceid;
}

bool DeviceManager::removeDevice(int deviceid)
{
    PLOGI("deviceid : %d", deviceid);

    if (!isDeviceIdValid(deviceid))
    {
        PLOGI("can not found device for deviceid : %d", deviceid);
        return false;
    }

    if (deviceMap_[deviceid].isDeviceOpen)
    {
        PLOGI("start cleaning the unplugged device!");
        CommandManager::getInstance().release(deviceid);
        PLOGI("end cleaning the unplugged device!");
    }

    // Pop platform-specific private data associated with this device.
    DEVICE_LIST_T devInfo = deviceMap_[deviceid].stList;
    deviceMap_.erase(deviceid);

    if (pAddon_ && pAddon_->hasImplementation())
        pAddon_->notifyDeviceRemoved(&devInfo);
    PLOGI("erase OK, deviceMap_.size : %zd", deviceMap_.size());

    if (lshandle_)
    {
        EventNotification obj;
        obj.eventReply(lshandle_, CONST_EVENT_KEY_CAMERA_LIST, EventType::EVENT_TYPE_DISCONNECT);

        // unsubscribe getFomat, getProperties for disconnected camera
        obj.removeSubscription(lshandle_, deviceid);
    }

    return true;
}

bool DeviceManager::updateDeviceList(std::string deviceType,
                                     const std::vector<DEVICE_LIST_T> &deviceList)
{
    // find unplugged device & remove device
    std::vector<int> unpluggedDeviceIdList;
    for (auto &curDev : deviceMap_)
    {
        if (curDev.second.stList.strDeviceType == deviceType)
        {
            if (std::find_if(deviceList.begin(), deviceList.end(),
                             [&](const DEVICE_LIST_T &dev) {
                                 return dev.strDeviceNode == curDev.second.stList.strDeviceNode;
                             }) == deviceList.end())
            {
                unpluggedDeviceIdList.push_back(curDev.first);
            }
        }
    }
    for (auto unpluggedId : unpluggedDeviceIdList)
    {
        removeDevice(unpluggedId);
    }

    // find plugged device & add device
    for (auto &newDev : deviceList)
    {
        if (std::find_if(deviceMap_.begin(), deviceMap_.end(),
                         [&](const std::pair<int, DEVICE_STATUS> &s)
                         {
                             return (s.second.stList.strDeviceType == deviceType) &&
                                    (s.second.stList.strDeviceNode == newDev.strDeviceNode);
                         }) == deviceMap_.end())
        {
            addDevice(newDev);
        }
    }

    if (pAddon_ && pAddon_->hasImplementation())
        pAddon_->notifyDeviceListUpdated(deviceType, static_cast<const void *>(&deviceList));
    return true;
}

DEVICE_RETURN_CODE_T DeviceManager::getInfo(int deviceid, camera_device_info_t *p_info)
{
    PLOGI("started ! deviceid : %d \n", deviceid);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    if (!isDeviceIdValid(deviceid))
        return DEVICE_ERROR_NODEVICE;

    if (!deviceMap_[deviceid].isDeviceInfoSaved)
    {
        std::string strdevicenode;
        strdevicenode = deviceMap_[deviceid].stList.strDeviceNode;

        std::string deviceType = getDeviceType(deviceid);
        PLOGI("deviceType : %s", deviceType.c_str());

        ret = CameraHalProxy::getDeviceInfo(strdevicenode, deviceType, p_info);

        if (DEVICE_OK != ret)
        {
            PLOGI("Failed to get device info\n");
            return ret;
        }
        // save DB data S
        deviceMap_[deviceid].deviceInfoDB.stResolution.clear();

        for (auto const &v : p_info->stResolution)
        {
            std::vector<std::string> c_res;
            c_res.clear();
            c_res.assign(v.c_res.begin(), v.c_res.end());
            deviceMap_[deviceid].deviceInfoDB.stResolution.emplace_back(c_res, v.e_format);
        }

        deviceMap_[deviceid].deviceInfoDB.n_devicetype = p_info->n_devicetype;
        deviceMap_[deviceid].deviceInfoDB.b_builtin    = p_info->b_builtin;
        PLOGI("save DB, deviceid:%d\n", deviceid);
        // save DB data E
        p_info->str_devicename = deviceMap_[deviceid].stList.strProductName;
        p_info->str_vendorid   = deviceMap_[deviceid].stList.strVendorID;
        p_info->str_productid  = deviceMap_[deviceid].stList.strProductID;

        deviceMap_[deviceid].isDeviceInfoSaved = true;
    }
    else
    {
        PLOGI("load DB, deviceid:%d\n", deviceid);
        // Load DB data S
        for (auto const &v : deviceMap_[deviceid].deviceInfoDB.stResolution)
        {
            std::vector<std::string> c_res;
            c_res.clear();
            c_res.assign(v.c_res.begin(), v.c_res.end());
            p_info->stResolution.emplace_back(c_res, v.e_format);
        }

        p_info->n_devicetype = deviceMap_[deviceid].deviceInfoDB.n_devicetype;
        p_info->b_builtin    = deviceMap_[deviceid].deviceInfoDB.b_builtin;
        // Load DB data E

        p_info->str_devicename = deviceMap_[deviceid].stList.strProductName;
        p_info->str_vendorid   = deviceMap_[deviceid].stList.strVendorID;
        p_info->str_productid  = deviceMap_[deviceid].stList.strProductID;
    }

    return ret;
}
