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

bool DeviceManager::addVirtualHandle(int deviceid, int virtualHandle)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d, virualHandle : %d", deviceid, virtualHandle);
    if (isDeviceIdValid(deviceid))
    {
        deviceMap_[deviceid].handleList.push_back(virtualHandle);
        PMLOG_INFO(CONST_MODULE_DM, "deviceMap_[%d].handleList.size : %d", deviceid,
                   deviceMap_[deviceid].handleList.size());
        return true;
    }

    return false;
}

bool DeviceManager::eraseVirtualHandle(int deviceid, int virtualHandle)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d, virtualHandle : %d", deviceid, virtualHandle);

    if (isDeviceIdValid(deviceid))
    {
        for (auto it = deviceMap_[deviceid].handleList.begin();
             it != deviceMap_[deviceid].handleList.end(); it++)
        {
            PMLOG_INFO(CONST_MODULE_DM, "cur.handleList : %d", *it);
            if (*it == virtualHandle)
            {
                deviceMap_[deviceid].handleList.erase(it);
                PMLOG_INFO(CONST_MODULE_DM, "deviceMap_[%d].handleList.size : %d", deviceid,
                           deviceMap_[deviceid].handleList.size());
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
            pCamDev[i]     = iter.first;
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

int DeviceManager::addDevice(DEVICE_LIST_T *pList)
{
    DEVICE_STATUS devStatus;
    devStatus.isDeviceOpen = false;
    devStatus.pcamhandle   = nullptr;

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
        // Pop platform-specific private data associated with this device.
        AddOn::popDevicePrivateData(deviceid);

        deviceMap_.erase(deviceid);
        PMLOG_INFO(CONST_MODULE_DM, "erase OK, deviceMap_.size : %d", deviceMap_.size());
        return true;
    }
    PMLOG_INFO(CONST_MODULE_DM, "can not found device for deviceid : %d", deviceid);
    return false;
}

DEVICE_RETURN_CODE_T DeviceManager::updateList(DEVICE_LIST_T *pList, int nDevCount,
                                               DEVICE_EVENT_STATE_T *pCamEvent,
                                               DEVICE_EVENT_STATE_T *pMicEvent)
{
    PMLOG_INFO(CONST_MODULE_DM, "started! nDevCount : %d \n", nDevCount);

    // Find the number of real V4L2 cameras
    int numV4L2Cameras = getDeviceCounts("v4l2");

    if (numV4L2Cameras < nDevCount) // Plugged
    {
        *pCamEvent = DEVICE_EVENT_STATE_PLUGGED;
        for (int i = 0; i < nDevCount; i++)
        {
            int id = 0;
            // find exist device
            for (auto iter : deviceMap_)
            {
                if (iter.second.stList.strDeviceNode == pList[i].strDeviceNode &&
                    iter.second.stList.strDeviceType == pList[i].strDeviceType)
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
    else if (numV4L2Cameras > nDevCount) // Unpluged
    {
        *pCamEvent = DEVICE_EVENT_STATE_UNPLUGGED;
        for (auto iter = deviceMap_.begin(); iter != deviceMap_.end();)
        {
            bool unplugged = true;

            // Skip cameras except real V4L2 cameras
            if (iter->second.stList.strDeviceType != "v4l2")
            {
                unplugged = false;
            }

            // Find out which camera is unplugged
            for (int i = 0; i < nDevCount; i++)
            {
                if (iter->second.stList.strDeviceNode == pList[i].strDeviceNode &&
                    iter->second.stList.strDeviceType == pList[i].strDeviceType)
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
                    for (int i = iter->second.handleList.size() - 1; i >= 0; i--)
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

DEVICE_RETURN_CODE_T DeviceManager::getInfo(int deviceid, camera_device_info_t *p_info)
{
    PMLOG_INFO(CONST_MODULE_DM, "started ! deviceid : %d \n", deviceid);

    if (!isDeviceIdValid(deviceid))
        return DEVICE_ERROR_NODEVICE;

    std::string strdevicenode;
    strdevicenode = deviceMap_[deviceid].stList.strDeviceNode;

    DEVICE_RETURN_CODE_T ret = DEVICE_RETURN_UNDEFINED;
    std::string type         = getDeviceType(deviceid);
    PMLOG_INFO(CONST_MODULE_DM, "type : %s", type.c_str());
    std::string libname = "lib" + type + "-camera-plugin.so";
    p_info->subsystem   = libname.c_str();
    ret                 = DeviceControl::getDeviceInfo(strdevicenode, p_info);
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

DEVICE_RETURN_CODE_T DeviceManager::updateHandle(int deviceid, void *handle)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d", deviceid);

    if (!isDeviceIdValid(deviceid))
        return DEVICE_ERROR_NODEVICE;

    deviceMap_[deviceid].pcamhandle = handle;

    return DEVICE_OK;
}

int DeviceManager::addRemoteCamera(deviceInfo_t *deviceInfo)
{
    DEVICE_LIST_T devInfo;

    if (!deviceInfo)
        return 0;

    PMLOG_INFO(CONST_MODULE_DM, "start");
    devInfo.nDeviceNum       = 0;
    devInfo.nPortNum         = 0;
    devInfo.isPowerOnConnect = true;
    devInfo.strVendorID      = "RemoteCamera";
    devInfo.strProductID     = "RemoteCamera";
    devInfo.strVendorName =
        (!deviceInfo->manufacturer.empty()) ? deviceInfo->manufacturer : "LG Electronics";
    devInfo.strProductName =
        (!deviceInfo->modelName.empty()) ? deviceInfo->modelName : "ThinQ WebCam";
    devInfo.strDeviceSubtype     = "IP-CAM JPEG";
    std::string remoteDeviceNode = "udpsrc=" + deviceInfo->clientKey;
    devInfo.strDeviceNode        = remoteDeviceNode;
    devInfo.strDeviceKey         = deviceInfo->clientKey;
    devInfo.strDeviceType        = deviceInfo->deviceLabel;

    int dev_idx = addDevice(&devInfo);

    printCameraStatus();
    return dev_idx;
}

int DeviceManager::removeRemoteCamera(int dev_idx)
{
    PMLOG_INFO(CONST_MODULE_DM, "start");

    if (dev_idx == 0)
    {
        PMLOG_INFO(CONST_MODULE_DM, "dev_idx : %d, No such device", dev_idx);
        return 0;
    }

    DEVICE_STATUS *pStatus = &deviceMap_[dev_idx];

    if (pStatus->stList.strDeviceType != "remote" && pStatus->stList.strDeviceType != "fake")
    {
        PMLOG_INFO(CONST_MODULE_DM, "dev_idx : %d, is not remote camera", dev_idx);
        return 0;
    }

    if (pStatus->isDeviceOpen && pStatus->handleList.size() > 0)
    {
        PMLOG_INFO(CONST_MODULE_DM, "start cleaning the unplugged device!");
        CommandManager::getInstance().requestPreviewCancel(dev_idx);
        for (int i = pStatus->handleList.size() - 1; i >= 0; i--)
        {
            CommandManager::getInstance().stopPreview(pStatus->handleList[i]);
            CommandManager::getInstance().close(pStatus->handleList[i]);
        }
        PMLOG_INFO(CONST_MODULE_DM, "end cleaning the unplugged device!");
    }
    removeDevice(dev_idx);

    printCameraStatus();
    return 1;
}

int DeviceManager::set_appcastclient(AppCastClient *pData)
{
    appCastClient_ = pData;
    return 1;
}

AppCastClient *DeviceManager::get_appcastclient() { return appCastClient_; }

bool DeviceManager::isRemoteCamera(DEVICE_LIST_T &deviceList)
{
    return (deviceList.strDeviceSubtype == "IP-CAM JPEG");
}

bool DeviceManager::isRemoteCamera(void *camhandle)
{
    for (auto iter : deviceMap_)
    {
        if (iter.second.pcamhandle == camhandle)
        {
            return isRemoteCamera(iter.second.stList);
        }
    }

    return false;
}

void DeviceManager::printCameraStatus()
{
    // Print the number of cameras
    int numV4L2Cameras = deviceMap_.size();
    for (auto iter : deviceMap_)
        if (isRemoteCamera(iter.second.stList))
            numV4L2Cameras--;
    PMLOG_INFO(CONST_MODULE_DM, "total_cameras:%d, usb:%d, remote:%d \n", deviceMap_.size(),
               numV4L2Cameras, deviceMap_.size() - numV4L2Cameras);
}

bool DeviceManager::getCurrentDeviceInfo(std::string &productId, std::string &vendorId,
                                         std::string &productName)
{
    for (auto &it : deviceMap_)
    {
        if (it.second.isDeviceOpen)
        {
            productId   = it.second.stList.strProductID;
            vendorId    = it.second.stList.strVendorID;
            productName = it.second.stList.strProductName;

            PMLOG_INFO(CONST_MODULE_DM, "strProductID = %s, strVendorID = %s \n", productId.c_str(),
                       vendorId.c_str());
            return true;
        }
    }
    return false;
}
