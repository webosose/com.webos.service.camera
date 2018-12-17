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
#include "device_manager.h"
#include "device_controller.h"
#include <string.h>

/*-----------------------------------------------------------------------------

 (Type Definitions)
 ------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 Static  Static prototype
 (Static Variables & Function Prototypes Declarations)
 ------------------------------------------------------------------------------*/
static int gdevCount = 0;

typedef struct _DEVICE_STATUS
{
    //Device
    char strDeviceName[256];
    int nDeviceID;
    int nDevIndex;
    int nDevCount;
    DEVICE_HANDLE sDevHandle;
    bool isDeviceOpen;
    int nSessionID;
    bool isSessionOwner;
    DEVICE_TYPE_T devType;
    DEVICE_LIST_T stList;
} DEVICE_STATUS;

static DEVICE_STATUS gdev_status[MAX_DEVICE];

int find_devnum(int ndeviceID)
{
    int nDeviceID = INVALID_ID;
    PMLOG_INFO(CONST_MODULE_DM, "find_devnum : gdevcount: %d \n",gdevCount);

    for (int i = 0; i < gdevCount; i++)
    {
        PMLOG_INFO(CONST_MODULE_DM, "find_devnum : gdev_status[%d].nDevIndex : %d \n",i,gdev_status[i].nDevIndex);
        PMLOG_INFO(CONST_MODULE_DM, "find_devnum : gdev_status[%d].nDeviceID : %d \n",i,gdev_status[i].nDeviceID);
        PMLOG_INFO(CONST_MODULE_DM, "find_devnum : ndeviceID : %d \n",ndeviceID);
        if((gdev_status[i].nDevIndex == ndeviceID)||(gdev_status[i].nDeviceID == ndeviceID))
        {
            nDeviceID = i;
            PMLOG_INFO(CONST_MODULE_DM, "dev_num is :%d\n", i);
        }
    }

    return nDeviceID;
}

bool DeviceManager::deviceStatus(int deviceID, DEVICE_TYPE_T devType, bool status)
{
    PMLOG_INFO(CONST_MODULE_LUNA, "deviceStatus : deviceID %d status : %d \n!!",deviceID,status);
    int dev_num = find_devnum(deviceID);
    PMLOG_INFO(CONST_MODULE_LUNA, "deviceStatus : dev_num : %d \n!!",dev_num);

    if(INVALID_ID == dev_num)
        return CONST_PARAM_VALUE_FALSE;
    if (status)
    {
        gdev_status[dev_num].devType = devType;
        gdev_status[dev_num].isDeviceOpen = true;
    }
    else
    {
        gdev_status[dev_num].devType = DEVICE_DEVICE_UNDEFINED;
        gdev_status[dev_num].isDeviceOpen = false;
    }

    return CONST_PARAM_VALUE_TRUE;
}

bool DeviceManager::isDeviceOpen(DEVICE_TYPE_T devType, int *deviceID)
{
    PMLOG_INFO(CONST_MODULE_LUNA, "DeviceManager::isDeviceOpen !!\n");
    int dev_num = find_devnum(*deviceID);
    if (INVALID_ID == dev_num)
    {
        *deviceID = dev_num;
        return CONST_PARAM_VALUE_FALSE;
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "isDeviceOpen :  *deviceID : %d\n",*deviceID);
    PMLOG_INFO(CONST_MODULE_LUNA, "isDeviceOpen :  *gdev_status[%d].nDeviceID : %d\n",dev_num,gdev_status[dev_num].nDeviceID);
    if (*deviceID == gdev_status[dev_num].nDeviceID)
    {
        PMLOG_INFO(CONST_MODULE_LUNA, "isDeviceOpen : gdev_status[%d].isDeviceOpen : %d\n",dev_num,gdev_status[dev_num].isDeviceOpen);
        if (gdev_status[dev_num].isDeviceOpen)
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
    int dev_num = find_devnum(*deviceID);
    if (INVALID_ID == dev_num)
    {
        *deviceID = dev_num;
        return CONST_PARAM_VALUE_TRUE;
    }

    if(TRUE == gdev_status[dev_num].isDeviceOpen)
    {
         return CONST_PARAM_VALUE_TRUE;
    }
     else
    {
         return CONST_PARAM_VALUE_FALSE;
    }
}

DEVICE_RETURN_CODE_T DeviceManager::getList(int *pCamDev, int *pMicDev, int *pCamSupport,
        int *pMicSupport)
{
    PMLOG_INFO(CONST_MODULE_DM, "DeviceManager::getList!!\n");

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int devCount = gdevCount;
    DEVICE_LIST_T pList[devCount];

    if (gdevCount)
    {
        for (int i = 0; i < gdevCount; i++)
        {
            pList[i] = (gdev_status[i].stList);
        }
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_DM, "No device detected by PDM!!!\n");
        return DEVICE_OK;
    }
    ret = DeviceControl::getInstance().getDeviceList(pList, pCamDev, pMicDev, pCamSupport, pMicSupport, devCount);
    if (DEVICE_OK != ret)
    {
        PMLOG_INFO(CONST_MODULE_DM, "Failed at control function\n");
        return ret;
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceManager::updateList(DEVICE_LIST_T *pList, int nDevCount,
        DEVICE_EVENT_STATE_T *pCamEvent, DEVICE_EVENT_STATE_T *pMicEvent)
{
    PMLOG_INFO(CONST_MODULE_DM,"DeviceManager::updateList started! nDevCount : %d \n",nDevCount);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int nCamDev = 0;
    int nMicDev = 0;
    int nCamSupport = 0;
    int nMicSupport = 0;

    if(gdevCount < nDevCount)
        *pCamEvent = DEVICE_EVENT_STATE_PLUGGED;
    else if(gdevCount > nDevCount)
        *pCamEvent = DEVICE_EVENT_STATE_UNPLUGGED;
    else
        PMLOG_INFO(CONST_MODULE_DM,"No event changed!!\n");

    gdevCount = nDevCount;
    for (int i = 0; i < gdevCount; i++)
    {
        strncpy(gdev_status[i].stList.strVendorName,pList[i].strVendorName,(CONST_MAX_STRING_LENGTH - 1));
        PMLOG_INFO(CONST_MODULE_DM,"gdev_status[%d].stList.strVendorName : %s \n",i,gdev_status[i].stList.strVendorName);
        strncpy(gdev_status[i].stList.strProductName,pList[i].strProductName,(CONST_MAX_STRING_LENGTH - 1));
        PMLOG_INFO(CONST_MODULE_DM,"gdev_status[%d].stList.strProductName : %s \n",i,gdev_status[i].stList.strProductName);
        strncpy(gdev_status[i].stList.strSerialNumber,pList[i].strSerialNumber,(CONST_MAX_STRING_LENGTH - 1));
        PMLOG_INFO(CONST_MODULE_DM,"gdev_status[%d].stList.strSerialNumber : %s \n",i,gdev_status[i].stList.strSerialNumber);
        strncpy(gdev_status[i].stList.strDeviceSubtype,pList[i].strDeviceSubtype,CONST_MAX_STRING_LENGTH - 1);
        PMLOG_INFO(CONST_MODULE_DM,"gdev_status[%d].stList.strDeviceSubtype : %s \n",i,gdev_status[i].stList.strDeviceSubtype);
        strncpy(gdev_status[i].stList.strDeviceType,pList[i].strDeviceType,(CONST_MAX_STRING_LENGTH - 1));
        PMLOG_INFO(CONST_MODULE_DM,"gdev_status[%d].stList.strDeviceType : %s \n",i,gdev_status[i].stList.strDeviceType);
        gdev_status[i].stList.nDeviceNum = pList[i].nDeviceNum;
        PMLOG_INFO(CONST_MODULE_DM,"gdev_status[%d].stList.nDeviceNum : %d \n",i,gdev_status[i].stList.nDeviceNum);
        gdev_status[i].nDevCount = nDevCount;
        PMLOG_INFO(CONST_MODULE_DM,"gdev_status[%d].nDevCount : %d \n",i,gdev_status[i].nDevCount);
        gdev_status[i].nDevIndex = i;
        PMLOG_INFO(CONST_MODULE_DM,"gdev_status[%d].nDevIndex : %d \n",i,gdev_status[i].nDevIndex);
        if(strcmp(pList[i].strDeviceType,"CAM")== 0)
            gdev_status[i].devType = DEVICE_CAMERA;
    }

    ret = DeviceControl::getInstance().getDeviceList(pList, &nCamDev, &nMicDev, &nCamSupport, &nMicSupport, nDevCount);
    if (DEVICE_OK == ret)
        PMLOG_INFO(CONST_MODULE_LUNA,"%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}


DEVICE_RETURN_CODE_T DeviceManager::getInfo(int ndevID, CAMERA_INFO_T *pInfo)
{
    PMLOG_INFO(CONST_MODULE_DM, "DeviceManager::getInfo started ndevID : %d \n",ndevID);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    DEVICE_LIST_T stList;
    int nCamID = 0;
    nCamID = find_devnum(ndevID);
    if(INVALID_ID == nCamID)
        return DEVICE_ERROR_NODEVICE;

    PMLOG_INFO(CONST_MODULE_DM,"gdev_status[%d].nDevIndex : %d \n",nCamID,gdev_status[nCamID].nDevIndex);

    if (gdev_status[nCamID].nDevIndex == nCamID)
    {
        stList = gdev_status[nCamID].stList;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_DM, "Failed to get device number\n");
        return DEVICE_ERROR_NODEVICE;
    }

    ret = DeviceControl::getInstance().getDeviceInfo(stList,pInfo);
    if (DEVICE_OK != ret)
    {
        PMLOG_INFO(CONST_MODULE_DM, "Failed to get device info\n");
        return ret;
    }

    PMLOG_INFO(CONST_MODULE_DM, "DeviceManager::getInfo ended \n");
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceManager::createHandle(int nDeviceID, DEVICE_TYPE_T devType, int *ndevID)
{
    PMLOG_INFO(CONST_MODULE_DM, "createHandle started nDeviceID : %d \n",nDeviceID);

    int dev_num = find_devnum(nDeviceID);
    if(INVALID_ID == dev_num)
        return DEVICE_ERROR_NODEVICE;

    DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().createHandle(gdev_status[dev_num].stList, &(gdev_status[dev_num].sDevHandle));
    if (DEVICE_OK == ret)
    {
        *ndevID = rand() % 10000;
        gdev_status[dev_num].nDeviceID = *ndevID;
    }
    PMLOG_INFO(CONST_MODULE_DM, "createHandle ended \n");
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceManager::getHandle(int nDeviceID, DEVICE_TYPE_T devType,
        DEVICE_HANDLE *devHandle)
{
    PMLOG_INFO(CONST_MODULE_DM, "getHandle started nDeviceID : %d \n",nDeviceID);

    int dev_num = find_devnum(nDeviceID);
    if(INVALID_ID == dev_num)
        return DEVICE_ERROR_NODEVICE;
    if (nDeviceID == gdev_status[dev_num].nDeviceID)
    {
        *devHandle = gdev_status[dev_num].sDevHandle;
    }
    else
        return DEVICE_ERROR_NODEVICE;

    PMLOG_INFO(CONST_MODULE_DM, "getHandle ended \n");
    return DEVICE_OK;
}

bool DeviceManager::isUpdatedList(void)
{
    return DeviceControl::getInstance().isUpdatedCameraList();
}
