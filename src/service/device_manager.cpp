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
#include "service_main.h"

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

DeviceControl *dCtl = DeviceControl::getInstance();

static DEVICE_STATUS gdev_status[MAX_DEVICE];

int find_devnum(int ndeviceID)
{
    int i = 0;
    int nDeviceID = 0;
    for (i = 0; i < gdevCount; i++)
    {
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
    int dev_num = find_devnum(deviceID);
    if (status)
    {
        gdev_status[dev_num].devType = devType;
        gdev_status[dev_num].isDeviceOpen = TRUE;

    }
    else
    {
        gdev_status[dev_num].devType = DEVICE_DEVICE_UNDEFINED;
        gdev_status[dev_num].isDeviceOpen = FALSE;
    }
    return CONST_PARAM_VALUE_TRUE;

}

bool DeviceManager::isDeviceOpen(DEVICE_TYPE_T devType, int deviceID)
{
    CAMERA_PRINT_INFO("%s:%d] Started!", __FUNCTION__, __LINE__);
    int dev_num = find_devnum(deviceID);
    if (deviceID == gdev_status[dev_num].nDeviceID)
    {
        if (gdev_status[dev_num].isDeviceOpen)
        {
            PMLOG_INFO(CONST_MODULE_DM, "Device is open\n!!");
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return CONST_PARAM_VALUE_TRUE;
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_DM, "Device is not open\n!!");
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return CONST_PARAM_VALUE_FALSE;
        }
    }
    else
        return CONST_PARAM_VALUE_FALSE;
}

bool DeviceManager::isDeviceValid(DEVICE_TYPE_T devType, int deviceID)
{
    int dev_num = find_devnum(deviceID);
    if(gdev_status[dev_num].isDeviceOpen == TRUE)
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
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);
    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int devCount = gdevCount;
    DEVICE_LIST_T *pList;
    int nCamDev, nMicDev, nCamSupport, nMicSupport;
    if (gdevCount)
    {
        for (int i = 0; i < gdevCount; i++)
        {
            pList = &(gdev_status[i].stList);
        }
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_DM, "No device found!!!\n");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_DEVICE_IS_NOT_STARTED;
    }
    ret = dCtl->getDeviceList(pList, &nCamDev, &nMicDev, &nCamSupport, &nMicSupport, devCount);
    if (DEVICE_OK != ret)
    {
        PMLOG_INFO(CONST_MODULE_DM, "Failed at control function\n");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return ret;
    }
    *pCamDev = nCamDev;
    *pMicDev = nMicDev;
    *pCamSupport = nCamSupport;
    *pMicSupport = nMicSupport;
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;

}

DEVICE_RETURN_CODE_T DeviceManager::updateList(DEVICE_LIST_T *pList, int nDevCount,
        DEVICE_EVENT_STATE_T *pCamEvent, DEVICE_EVENT_STATE_T *pMicEvent)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);
    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int nCamDev = 0;
    int nMicDev = 0;
    int nCamSupport = 0;
    int nMicSupport = 0;
    static int nid = 0;
    gdevCount = nDevCount;
    for (int i = 0; i < gdevCount; i++)
    {
        gdev_status[i].stList = pList[i];
        gdev_status[i].nDevCount = nDevCount;
        gdev_status[i].nDevIndex = i+1;
    }
    ret = dCtl->getDeviceList(pList, &nCamDev, &nMicDev, &nCamSupport, &nMicSupport, nDevCount);
    if (DEVICE_OK == ret)
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}


DEVICE_RETURN_CODE_T DeviceManager::getInfo(int ndevID, CAMERA_INFO_T *pInfo)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);
    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    DEVICE_LIST_T stList;
    int nCamID = 0;
    nCamID = find_devnum(ndevID);
    if (gdev_status[nCamID].nDevIndex == ndevID)
    {
        stList = gdev_status[nCamID].stList;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_DM, "Failed to get device number\n");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_NODEVICE;
    }
    ret = dCtl->getDeviceInfo(stList,pInfo);
    if (DEVICE_OK != ret)
    {
        PMLOG_INFO(CONST_MODULE_DM, "Failed to get device info\n");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return ret;
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;

}


DEVICE_RETURN_CODE_T DeviceManager::createHandle(int nDeviceID, DEVICE_TYPE_T devType, int *ndevID)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int dev_num = find_devnum(nDeviceID);
    ret = dCtl->createHandle(gdev_status[dev_num].stList, &(gdev_status[dev_num].sDevHandle));
    if (DEVICE_OK == ret)
    {
        *ndevID = rand() % 10000;
        gdev_status[dev_num].nDeviceID = *ndevID;

    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}
DEVICE_RETURN_CODE_T DeviceManager::getHandle(int nDeviceID, DEVICE_TYPE_T devType,
        DEVICE_HANDLE *devHandle)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    int dev_num = find_devnum(nDeviceID);

    if (nDeviceID == gdev_status[dev_num].nDeviceID)
    {
        *devHandle = gdev_status[dev_num].sDevHandle;
    }
    else
        return DEVICE_ERROR_NODEVICE;
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

bool DeviceManager::isUpdatedList(void)
{
    return dCtl->isUpdatedCameraList();
}

