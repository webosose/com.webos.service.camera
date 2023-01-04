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
#include "addon.h" /* calls platform specific functionality if addon interface has been implemented */
#include "command_manager.h"
#include "device_controller.h"

DeviceManager::DeviceManager() {}

bool DeviceManager::isDeviceIdValid(int deviceid)
{
    if (deviceMap_.find(deviceid) != deviceMap_.end())
        return true;
    return false;
}

bool DeviceManager::setDeviceStatus(int deviceid, bool status)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid %d status : %d \n!!", deviceid, status);

    if (!isDeviceIdValid(deviceid))
        return CONST_PARAM_VALUE_FALSE;

    deviceMap_[deviceid].isDeviceOpen = status;

    return CONST_PARAM_VALUE_TRUE;
}

bool DeviceManager::isDeviceOpen(int deviceid)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d", deviceid);

    if (!isDeviceIdValid(deviceid))
        return CONST_PARAM_VALUE_FALSE;

    PMLOG_DEBUG("deviceMap_[%d].isDeviceOpen : %d", deviceid, deviceMap_[deviceid].isDeviceOpen);
    if (deviceMap_[deviceid].isDeviceOpen)
    {
        PMLOG_INFO(CONST_MODULE_DM, "Device is open!!");
        return CONST_PARAM_VALUE_TRUE;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_DM, "Device is not open!!");
        return CONST_PARAM_VALUE_FALSE;
    }
}

bool DeviceManager::isDeviceValid(int deviceid) { return isDeviceIdValid(deviceid); }

void DeviceManager::getDeviceNode(int deviceid, std::string &devicenode)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d", deviceid);

    devicenode = "";
    if (isDeviceIdValid(deviceid))
    {
        devicenode = deviceMap_[deviceid].stList.strDeviceNode;
    }
}

void DeviceManager::getDeviceHandle(int deviceid, void **devicehandle)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d", deviceid);

    *devicehandle = nullptr;
    if (isDeviceIdValid(deviceid))
    {
        *devicehandle = deviceMap_[deviceid].pcamhandle;
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

int DeviceManager::getDeviceCounts(std::string type)
{
    int count = 0;
    for (auto iter : deviceMap_)
    {
        if (iter.second.stList.strDeviceType == type)
            ++count;
    }

    return count;
}

bool DeviceManager::getDeviceUserData(int deviceid, std::string &userData)
{
    if (isDeviceIdValid(deviceid))
    {
        userData = deviceMap_[deviceid].userData;
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

int DeviceManager::addDevice(DEVICE_LIST_T *pList, std::string userData)
{
    DEVICE_STATUS devStatus;
    devStatus.isDeviceOpen = false;
    devStatus.pcamhandle   = nullptr;
    devStatus.userData     = userData;

    devStatus.stList.strVendorName = pList->strVendorName;
    PMLOG_INFO(CONST_MODULE_DM, "strVendorName : %s", devStatus.stList.strVendorName.c_str());
    devStatus.stList.strProductName = pList->strProductName;
    PMLOG_INFO(CONST_MODULE_DM, "strProductName : %s", devStatus.stList.strProductName.c_str());
    devStatus.stList.strVendorID = pList->strVendorID;
    PMLOG_INFO(CONST_MODULE_DM, "strVendorID : %s", devStatus.stList.strVendorID.c_str());
    devStatus.stList.strProductID = pList->strProductID;
    PMLOG_INFO(CONST_MODULE_DM, "strProductID : %s", devStatus.stList.strProductID.c_str());
    devStatus.stList.strDeviceSubtype = pList->strDeviceSubtype;
    PMLOG_INFO(CONST_MODULE_DM, "strDeviceSubtype : %s", devStatus.stList.strDeviceSubtype.c_str());

    devStatus.stList.strDeviceType = pList->strDeviceType;
    PMLOG_INFO(CONST_MODULE_DM, "strDeviceType : %s", devStatus.stList.strDeviceType.c_str());
    devStatus.stList.nDeviceNum = pList->nDeviceNum;
    PMLOG_INFO(CONST_MODULE_DM, "nDeviceNum : %d", devStatus.stList.nDeviceNum);
    devStatus.stList.nPortNum = pList->nPortNum;
    PMLOG_INFO(CONST_MODULE_DM, "nPortNum : %d", devStatus.stList.nPortNum);
    devStatus.stList.isPowerOnConnect = pList->isPowerOnConnect;
    PMLOG_INFO(CONST_MODULE_DM, "isPowerOnConnect : %d", devStatus.stList.isPowerOnConnect);
    devStatus.stList.strDeviceNode = pList->strDeviceNode;
    PMLOG_INFO(CONST_MODULE_DM, "strDeviceNode : %s", devStatus.stList.strDeviceNode.c_str());
    devStatus.stList.strDeviceKey = pList->strDeviceKey;
    PMLOG_INFO(CONST_MODULE_DM, "strDeviceKey : %s", devStatus.stList.strDeviceKey.c_str());

    int devidx = 0;
    for (int i = 1; i <= MAX_DEVICE_COUNT; i++)
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
        return 0;

    // Push platform-specific device private data associated with this device */
    AddOn::pushDevicePrivateData(devidx, devidx, pList);

    deviceMap_[devidx] = devStatus;
    PMLOG_INFO(CONST_MODULE_DM, "devidx : %d, deviceMap_.size : %d \n", devidx, deviceMap_.size());
    return devidx;
}

bool DeviceManager::removeDevice(int deviceid)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d", deviceid);

    if (isDeviceIdValid(deviceid))
    {
        if (deviceMap_[deviceid].isDeviceOpen)
        {
            PMLOG_INFO(CONST_MODULE_DM, "start cleaning the unplugged device!");
            CommandManager::getInstance().release(deviceid);
            PMLOG_INFO(CONST_MODULE_DM, "end cleaning the unplugged device!");
        }

        // Pop platform-specific private data associated with this device.
        AddOn::popDevicePrivateData(deviceid);

        deviceMap_.erase(deviceid);
        PMLOG_INFO(CONST_MODULE_DM, "erase OK, deviceMap_.size : %d", deviceMap_.size());
        return true;
    }
    PMLOG_INFO(CONST_MODULE_DM, "can not found device for deviceid : %d", deviceid);
    return false;
}

DEVICE_RETURN_CODE_T DeviceManager::getInfo(int deviceid, camera_device_info_t *p_info)
{
    PMLOG_INFO(CONST_MODULE_DM, "started ! deviceid : %d \n", deviceid);

    if (!isDeviceIdValid(deviceid))
        return DEVICE_ERROR_NODEVICE;

    std::string strdevicenode;
    strdevicenode = deviceMap_[deviceid].stList.strDeviceNode;

    std::string deviceType = getDeviceType(deviceid);
    PMLOG_INFO(CONST_MODULE_DM, "deviceType : %s", deviceType.c_str());

    DEVICE_RETURN_CODE_T ret = DeviceControl::getDeviceInfo(strdevicenode, deviceType, p_info);
    if (DEVICE_OK != ret)
    {
        PMLOG_INFO(CONST_MODULE_DM, "Failed to get device info\n");
    }

    memset(p_info->str_devicename, '\0', sizeof(p_info->str_devicename));
    strncpy(p_info->str_devicename, deviceMap_[deviceid].stList.strProductName.c_str(),
            sizeof(p_info->str_devicename) - 1);
    memset(p_info->str_vendorid, '\0', sizeof(p_info->str_vendorid));
    strncpy(p_info->str_vendorid, deviceMap_[deviceid].stList.strVendorID.c_str(),
            sizeof(p_info->str_vendorid) - 1);
    memset(p_info->str_productid, '\0', sizeof(p_info->str_productid));
    strncpy(p_info->str_productid, deviceMap_[deviceid].stList.strProductID.c_str(),
            sizeof(p_info->str_productid) - 1);

    return ret;
}

bool DeviceManager::setDeviceHandle(int deviceid, void *handle)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d", deviceid);

    if (!isDeviceIdValid(deviceid))
        return false;

    deviceMap_[deviceid].pcamhandle = handle;

    return true;
}
