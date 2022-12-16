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

int DeviceManager::findDevNum(int ndevicehandle)
{
    int nDeviceID = n_invalid_id;
    PMLOG_DEBUG("ndevicehandle : %d", ndevicehandle);
    PMLOG_DEBUG("deviceMap_.count : %d", deviceMap_.size());

    if (ndevicehandle == n_invalid_id)
        return nDeviceID;

    for (auto iter : deviceMap_)
    {
        PMLOG_DEBUG("iter.second.nDevIndex : %d", iter.second.nDevIndex);
        PMLOG_DEBUG("iter.second.nDeviceID : %d", iter.second.nDeviceID);

        if ((iter.second.nDevIndex == ndevicehandle) || (iter.second.nDeviceID == ndevicehandle))
        {
            nDeviceID = iter.first;
            PMLOG_DEBUG("dev_num is :%d", nDeviceID);
            break;
        }
    }
    return nDeviceID;
}

bool DeviceManager::setOpenStatus(int deviceID, bool status)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceID %d status : %d \n!!", deviceID, status);
    int dev_num = findDevNum(deviceID);
    PMLOG_INFO(CONST_MODULE_DM, "dev_num : %d \n!!", dev_num);

    if (n_invalid_id == dev_num)
        return CONST_PARAM_VALUE_FALSE;

    deviceMap_[dev_num].isDeviceOpen = status;

    return CONST_PARAM_VALUE_TRUE;
}

bool DeviceManager::isDeviceOpen(int *deviceID)
{
    PMLOG_INFO(CONST_MODULE_DM, "started!");
    int dev_num = findDevNum(*deviceID);
    if (n_invalid_id == dev_num)
    {
        *deviceID = dev_num;
        return CONST_PARAM_VALUE_FALSE;
    }

    PMLOG_DEBUG("isDeviceOpen :  *deviceID : %d\n", *deviceID);
    PMLOG_DEBUG("isDeviceOpen :  deviceMap_[%d].nDeviceID : %d\n", dev_num,
                deviceMap_[dev_num].nDeviceID);

    if (*deviceID == deviceMap_[dev_num].nDeviceID)
    {
        PMLOG_DEBUG("isDeviceOpen : deviceMap_[%d].isDeviceOpen : %d\n", dev_num,
                    deviceMap_[dev_num].isDeviceOpen);
        if (deviceMap_[dev_num].isDeviceOpen)
        {
            PMLOG_INFO(CONST_MODULE_DM, "Device is open\n!!");
            return CONST_PARAM_VALUE_TRUE;
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_DM, "Device is not open\n!!");
            return CONST_PARAM_VALUE_FALSE;
        }
    }
    else
        return CONST_PARAM_VALUE_FALSE;
}

bool DeviceManager::isDeviceValid(DEVICE_TYPE_T devType, int *deviceID)
{
    int dev_num = findDevNum(*deviceID);
    if (n_invalid_id == dev_num)
    {
        *deviceID = dev_num;
        return CONST_PARAM_VALUE_TRUE;
    }

    if (TRUE == deviceMap_[dev_num].isDeviceOpen)
    {
        return CONST_PARAM_VALUE_TRUE;
    }
    else
    {
        return CONST_PARAM_VALUE_FALSE;
    }
}

void DeviceManager::getDeviceNode(int *device_id, std::string &strdevicenode)
{
    int dev_num = findDevNum(*device_id);
    if (n_invalid_id == dev_num)
    {
        *device_id = dev_num;
        return;
    }
    strdevicenode = deviceMap_[dev_num].stList.strDeviceNode;
    return;
}

void DeviceManager::getDeviceHandle(int *device_id, void **devicehandle)
{
    int dev_num = findDevNum(*device_id);
    if (n_invalid_id == dev_num)
    {
        *device_id = dev_num;
        return;
    }
    *devicehandle = deviceMap_[dev_num].pcamhandle;
    return;
}

int DeviceManager::getDeviceId(int *device_id)
{
    int dev_num = findDevNum(*device_id);
    if (n_invalid_id == dev_num)
    {
        *device_id = dev_num;
        return 0;
    }

    return deviceMap_[dev_num].nDeviceID;
}

DEVICE_TYPE_T DeviceManager::getDeviceType(int *device_id)
{
    int dev_num = findDevNum(*device_id);
    if (n_invalid_id == dev_num)
    {
        *device_id = dev_num;
        return DEVICE_DEVICE_UNDEFINED;
    }

    return deviceMap_[dev_num].devType;
}

int DeviceManager::getDeviceCounts(DEVICE_TYPE_T type)
{
    int count = 0;
    for (auto iter : deviceMap_)
    {
        if (iter.second.devType == type)
            ++count;
    }

    return count;
}

bool DeviceManager::addVirtualHandle(int devid, int virtualHandle)
{
    PMLOG_INFO(CONST_MODULE_DM, "devid: %d, virualHandle: %d", devid, virtualHandle);
    if (deviceMap_.find(devid) != deviceMap_.end())
    {
        deviceMap_[devid].handleList.push_back(virtualHandle);
        PMLOG_INFO(CONST_MODULE_DM, "deviceMap_[%d].handleList.size : %d", devid,
                   deviceMap_[devid].handleList.size());
        return true;
    }

    return false;
}

bool DeviceManager::eraseVirtualHandle(int deviceId, int virtualHandle)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceId: %d, virtualHandle: %d", deviceId, virtualHandle);

    // find devid
    int devid = 0;
    for (auto iter : deviceMap_)
    {
        PMLOG_INFO(CONST_MODULE_DM, "first: %d, nDeviceID: %d", iter.first, iter.second.nDeviceID);
        if (iter.second.nDeviceID == deviceId)
        {
            devid = iter.first;
            break;
        }
    }
    if (devid == 0)
    {
        PMLOG_INFO(CONST_MODULE_DM, "Cannot found devid");
        return false;
    }

    PMLOG_INFO(CONST_MODULE_DM, "devid: %d", devid);
    if (deviceMap_.find(devid) != deviceMap_.end())
    {
        for (auto it = deviceMap_[devid].handleList.begin();
             it != deviceMap_[devid].handleList.end(); it++)
        {
            PMLOG_INFO(CONST_MODULE_DM, "cur.handleList : %d", *it);
            if (*it == virtualHandle)
            {
                deviceMap_[devid].handleList.erase(it);
                PMLOG_INFO(CONST_MODULE_DM, "deviceMap_[%d].handleList.size : %d", devid,
                           deviceMap_[devid].handleList.size());
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
    devStatus.nDeviceID    = n_invalid_id;

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

    devStatus.stList.strDeviceLabel = pList->strDeviceLabel;
    PMLOG_INFO(CONST_MODULE_DM, "strDeviceLabel : %s", devStatus.stList.strDeviceLabel.c_str());

    devStatus.nDevCount = deviceMap_.size() + 1;
    PMLOG_INFO(CONST_MODULE_DM, "nDevCount : %d", devStatus.nDevCount);
    devStatus.devType = DEVICE_V4L2_CAMERA;
    if (devStatus.stList.strDeviceLabel == "remote")
        devStatus.devType = DEVICE_REMOTE_CAMERA;
    else if (devStatus.stList.strDeviceLabel == "dummy")
        devStatus.devType = DEVICE_V4L2_CAMERA_DUMMY;
    else if (devStatus.stList.strDeviceLabel == "fake")
        devStatus.devType = DEVICE_REMOTE_CAMERA_FAKE;

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

    devStatus.nDevIndex = devidx;
    PMLOG_INFO(CONST_MODULE_DM, "devStatus.nDevIndex : %d \n", devStatus.nDevIndex);

    // Push platform-specific device private data associated with this device */
    AddOn::pushDevicePrivateData(devStatus.nDeviceID, devStatus.nDevIndex, devStatus.devType, pList);

    deviceMap_[devidx] = devStatus;
    PMLOG_INFO(CONST_MODULE_DM, "devidx : %d, deviceMap_.size : %d \n", devidx, deviceMap_.size());
    return devidx;
}

bool DeviceManager::removeDevice(int devid)
{
    PMLOG_INFO(CONST_MODULE_DM, "devid : %d", devid);
    auto dev = deviceMap_.find(devid);
    if (dev != deviceMap_.end())
    {
        // Pop platform-specific private data associated with this device.
        AddOn::popDevicePrivateData(devid);

        deviceMap_.erase(dev);
        PMLOG_INFO(CONST_MODULE_DM, "erase OK, deviceMap_.size : %d", deviceMap_.size());
        return true;
    }
    PMLOG_INFO(CONST_MODULE_DM, "can not found device for devid : %d", devid);
    return false;
}

DEVICE_RETURN_CODE_T DeviceManager::updateList(DEVICE_LIST_T *pList, int nDevCount,
                                               DEVICE_EVENT_STATE_T *pCamEvent,
                                               DEVICE_EVENT_STATE_T *pMicEvent)
{
    PMLOG_INFO(CONST_MODULE_DM, "started! nDevCount : %d \n", nDevCount);

    // Find the number of real V4L2 cameras
    int numV4L2Cameras = getDeviceCounts(DEVICE_V4L2_CAMERA);

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
                    iter.second.stList.strDeviceLabel == pList[i].strDeviceLabel)
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
            if (iter->second.devType != DEVICE_V4L2_CAMERA)
            {
                unplugged = false;
            }

            // Find out which camera is unplugged
            for (int i = 0; i < nDevCount; i++)
            {
                if (iter->second.stList.strDeviceNode == pList[i].strDeviceNode &&
                    iter->second.stList.strDeviceLabel == pList[i].strDeviceLabel)
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

DEVICE_RETURN_CODE_T DeviceManager::getInfo(int ndev_id, camera_device_info_t *p_info)
{
    PMLOG_INFO(CONST_MODULE_DM, "started ! ndev_id : %d \n", ndev_id);

    int ncam_id = findDevNum(ndev_id);
    if (n_invalid_id == ncam_id)
        return DEVICE_ERROR_NODEVICE;

    PMLOG_INFO(CONST_MODULE_DM, "deviceMap_[%d].nDevIndex : %d \n", ncam_id,
               deviceMap_[ncam_id].nDevIndex);

    std::string strdevicenode;
    if (deviceMap_[ncam_id].nDevIndex == ndev_id)
    {
        strdevicenode = deviceMap_[ncam_id].stList.strDeviceNode;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_DM, "Failed to get device number\n");
        return DEVICE_ERROR_NODEVICE;
    }

    DEVICE_RETURN_CODE_T ret = DEVICE_RETURN_UNDEFINED;
    DEVICE_TYPE_T type = getDeviceType(&ndev_id);
    // debug
    PMLOG_INFO(CONST_MODULE_DM, "type : %d", (int)type);

    p_info->subsystem = nullptr;
    switch (type)
    {
    case DEVICE_V4L2_CAMERA:
        p_info->subsystem = cstr_libv4l2.c_str();
        break;
    case DEVICE_REMOTE_CAMERA:
        p_info->subsystem = cstr_libremote.c_str();
        break;
    case DEVICE_REMOTE_CAMERA_FAKE:
        p_info->subsystem = cstr_libfake.c_str();
        break;
    case DEVICE_V4L2_CAMERA_DUMMY:
        p_info->subsystem = cstr_libdummy.c_str();
        break;
    default:
        break;
    }

    if (p_info->subsystem == nullptr)
    {
        PMLOG_INFO(CONST_MODULE_DM, "Invalid subsystem");
        return DEVICE_RETURN_UNDEFINED;
    }

    ret = DeviceControl::getDeviceInfo(strdevicenode, p_info);
    if (DEVICE_OK != ret)
    {
        PMLOG_INFO(CONST_MODULE_DM, "Failed to get device info\n");
    }
    memset(p_info->str_devicename, '\0', sizeof(p_info->str_devicename));
    strncpy(p_info->str_devicename, deviceMap_[ncam_id].stList.strProductName.c_str(),
            sizeof(p_info->str_devicename) - 1);
    memset(p_info->str_vendorid, '\0', sizeof(p_info->str_vendorid));
    strncpy(p_info->str_vendorid, deviceMap_[ncam_id].stList.strVendorID.c_str(),
            sizeof(p_info->str_vendorid) - 1);
    memset(p_info->str_productid, '\0', sizeof(p_info->str_productid));
    strncpy(p_info->str_productid, deviceMap_[ncam_id].stList.strProductID.c_str(),
            sizeof(p_info->str_productid) - 1);

    return ret;
}

DEVICE_RETURN_CODE_T DeviceManager::updateHandle(int deviceid, void *handle)
{
    PMLOG_INFO(CONST_MODULE_DM, "deviceid : %d \n", deviceid);
    int devicehandle = getRandomNumber();
    int dev_num      = findDevNum(deviceid);
    if (n_invalid_id == dev_num)
        return DEVICE_ERROR_NODEVICE;

    if (handle)
        deviceMap_[dev_num].nDeviceID = devicehandle;
    deviceMap_[dev_num].pcamhandle = handle;

    // Pass updated handle value to platform-specific device private handler
    AddOn::updateDevicePrivateHandle(deviceid, devicehandle);

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
    devInfo.strDeviceType        = "CAM";
    devInfo.strDeviceSubtype     = "IP-CAM JPEG";
    std::string remoteDeviceNode = "udpsrc=" + deviceInfo->clientKey;
    devInfo.strDeviceNode = remoteDeviceNode;
    devInfo.strDeviceKey  = deviceInfo->clientKey;
    devInfo.strDeviceLabel = deviceInfo->deviceLabel;

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

    if (pStatus->devType != DEVICE_REMOTE_CAMERA && pStatus->devType != DEVICE_REMOTE_CAMERA_FAKE)
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
        if (std::string("CAM") != it.second.stList.strDeviceType)
            continue;
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
