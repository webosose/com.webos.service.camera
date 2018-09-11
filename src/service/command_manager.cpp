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
#include "constants.h"
#include "service_main.h"

#define SESSION 0
/*-----------------------------------------------------------------------------
 Static  Static prototype
 (Static Variables & Function Prototypes Declarations)
 ------------------------------------------------------------------------------*/
extern DEVICE_LIST_T arrDevList[MAX_DEVICE];

DeviceManager *DeviceManager::devInfoinstance = nullptr;
DeviceManager *devInfo = DeviceManager::getInstance();

DeviceControl *DeviceControl::devctlinstance = nullptr;
DeviceControl *devCtl = DeviceControl::getInstance();

DEVICE_RETURN_CODE_T CommandManager::open(int deviceID, DEVICE_TYPE_T devType,int *devhandle)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE stDevHandle;
    if (devInfo->isDeviceValid(devType, &deviceID))
    {
        if(INVALID_ID == deviceID)
            return DEVICE_ERROR_NODEVICE;
        else
            return DEVICE_ERROR_DEVICE_IS_ALREADY_OPENED;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open devID\n");
        ret = devInfo->createHandle(deviceID, devType, devhandle);
        if (DEVICE_OK != ret)
        {
            PMLOG_ERROR(CONST_MODULE_CM, "Failed to create device handle\n");
            return ret;
        }
        devInfo->getHandle(*devhandle, devType, &stDevHandle);
        ret = devCtl->open(stDevHandle, devType);
        if (ret == DEVICE_OK)
        {
            devInfo->deviceStatus(deviceID, devType, TRUE);
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_CM, "Failed to open device\n");
            return ret;
        }
    }

    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T CommandManager::close(int deviceID, DEVICE_TYPE_T devType)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE stDevHandle;

    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;

    if (devInfo->isDeviceOpen(devType, &deviceID))
    {
        devInfo->getHandle(deviceID, devType, &stDevHandle);
        ret = devCtl->close(stDevHandle, devType);
        if (DEVICE_OK == ret)
        {
            devInfo->deviceStatus(deviceID, devType, FALSE);
            PMLOG_INFO(CONST_MODULE_DM, "Device Close successfully\n");
            return ret;
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_DM, "Requested device is not closed %d", deviceID);
            return DEVICE_ERROR_CAN_NOT_CLOSE;
        }
        return DEVICE_OK;
    }
    else
        return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;

    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
}

DEVICE_RETURN_CODE_T CommandManager::getDeviceInfo(int deviceID, DEVICE_TYPE_T devType,
        CAMERA_INFO_T *pInfo)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;

    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    ret = devInfo->getInfo(deviceID,pInfo);
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return ret;
}


DEVICE_RETURN_CODE_T CommandManager::getDeviceList(int *pCamDev, int *pMicDev, int *pCamSupport,
        int *pMicSupport)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);
    DEVICE_RETURN_CODE_T ret;

    ret = devInfo->getList(pCamDev, pMicDev, pCamSupport, pMicSupport);
    if (DEVICE_OK != ret)
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Failed to get device list\n");
        return ret;
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return ret;
}

DEVICE_RETURN_CODE_T CommandManager::createHandle(int deviceID, DEVICE_TYPE_T devType,
        int *devhandle)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);
    DEVICE_RETURN_CODE_T ret;
    ret = devInfo->createHandle(deviceID, devType, devhandle);
    if (DEVICE_OK != ret)
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Failed to get device list\n");
        return ret;
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return ret;
}

DEVICE_RETURN_CODE_T CommandManager::updateList(DEVICE_LIST_T *pList, int nDevCount,
        DEVICE_EVENT_STATE_T *pCamEvent, DEVICE_EVENT_STATE_T *pMicEvent)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    ret = devInfo->updateList(pList, nDevCount, pCamEvent, pMicEvent);
    if (DEVICE_OK != ret)
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Failed to get device list\n");
        return ret;
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return ret;

}

DEVICE_RETURN_CODE_T CommandManager::getProperty(int deviceID, DEVICE_TYPE_T devType,
        CAMERA_PROPERTIES_T *devproperty)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE sdevhandle;
    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED || devproperty == NULL)
        return DEVICE_ERROR_WRONG_PARAM;

    //initilize the property
    devproperty->nZoom = CONST_VARIABLE_INITIALIZE;
    devproperty->nGridZoomX = CONST_VARIABLE_INITIALIZE;
    devproperty->nGridZoomY = CONST_VARIABLE_INITIALIZE;
    devproperty->nPan = CONST_VARIABLE_INITIALIZE;
    devproperty->nTilt = CONST_VARIABLE_INITIALIZE;
    devproperty->nContrast = CONST_VARIABLE_INITIALIZE;
    devproperty->nBrightness = CONST_VARIABLE_INITIALIZE;
    devproperty->nSaturation = CONST_VARIABLE_INITIALIZE;
    devproperty->nSharpness = CONST_VARIABLE_INITIALIZE;
    devproperty->nHue = CONST_VARIABLE_INITIALIZE;
    devproperty->nWhiteBalanceTemperature = CONST_VARIABLE_INITIALIZE;
    devproperty->nGain = CONST_VARIABLE_INITIALIZE;
    devproperty->nGamma = CONST_VARIABLE_INITIALIZE;
    devproperty->nFrequency = CONST_VARIABLE_INITIALIZE;
    devproperty->bMirror = CONST_VARIABLE_INITIALIZE;
    devproperty->nExposure = CONST_VARIABLE_INITIALIZE;
    devproperty->bAutoExposure = CONST_VARIABLE_INITIALIZE;
    devproperty->bAutoWhiteBalance = CONST_VARIABLE_INITIALIZE;
    devproperty->nBitrate = CONST_VARIABLE_INITIALIZE;
    devproperty->nFramerate = CONST_VARIABLE_INITIALIZE;
    devproperty->ngopLength = CONST_VARIABLE_INITIALIZE;
    devproperty->bLed = CONST_VARIABLE_INITIALIZE;
    devproperty->bYuvMode = CONST_VARIABLE_INITIALIZE;
    devproperty->nIllumination = CONST_VARIABLE_INITIALIZE;
    devproperty->bBacklightCompensation = CONST_VARIABLE_INITIALIZE;

    devproperty->nMicMaxGain = CONST_VARIABLE_INITIALIZE;
    devproperty->nMicMinGain = CONST_VARIABLE_INITIALIZE;
    devproperty->nMicGain = CONST_VARIABLE_INITIALIZE;
    devproperty->bMicMute = CONST_VARIABLE_INITIALIZE;


    if (devInfo->isDeviceOpen(devType, &deviceID))
    {

        PMLOG_INFO(CONST_MODULE_CM, "Device is open \n");
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        CAMERA_PRINT_INFO("%s:%d ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }

    devInfo->getHandle(deviceID, devType, &sdevhandle);
    PMLOG_INFO(CONST_MODULE_CM, "%s:%d here!", __FUNCTION__, __LINE__);
    ret = devCtl->getDeviceProperty(sdevhandle, devType, devproperty);
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return ret;
}

DEVICE_RETURN_CODE_T CommandManager::setProperty(int deviceID, DEVICE_TYPE_T devType,
        CAMERA_PROPERTIES_T *oInfo)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE devHandle;
    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (DEVICE_CAMERA == devType)
    {

        if (devInfo->isDeviceOpen(devType, &deviceID))
        {

            PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
            devInfo->getHandle(deviceID, devType, &devHandle);
            ret = devCtl->setDeviceProperty(devHandle, devType, oInfo);
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return ret;
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
        }
    }
    else if (DEVICE_MICROPHONE == devType)
    {
        ret = devCtl->setDeviceProperty(devHandle, devType, oInfo);
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return ret;

    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device is not valid \n!!");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_NODEVICE;
    }
}

DEVICE_RETURN_CODE_T CommandManager::setFormat(int deviceID, DEVICE_TYPE_T devType,
        FORMAT oFormat)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE devHandle;
    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (DEVICE_CAMERA == devType)
    {
        if (devInfo->isDeviceOpen(devType, &deviceID))
        {

            PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
            devInfo->getHandle(deviceID, devType, &devHandle);
            ret = devCtl->setFormat(devHandle, devType, oFormat);
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return ret;
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_CM, "Device not open %d", deviceID);
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
        }
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device is not valid \n!!");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_NODEVICE;
    }
}


DEVICE_RETURN_CODE_T CommandManager::startPreview(int deviceID, DEVICE_TYPE_T devType, int *pKey)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE devHandle;
    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (devInfo->isDeviceOpen(devType, &deviceID))
    {

        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        if(devInfo->getHandle(deviceID, devType, &devHandle)== DEVICE_OK)
        {
            ret = devCtl->startPreview(devHandle, devType,pKey);
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return ret;
         }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }

}


DEVICE_RETURN_CODE_T CommandManager::stopPreview(int deviceID, DEVICE_TYPE_T devType)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE devHandle;
    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;

    if (devInfo->isDeviceOpen(devType, &deviceID))
    {

        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        if(devInfo->getHandle(deviceID, devType, &devHandle)== DEVICE_OK)
        {
            ret = devCtl->stopPreview(devHandle, devType);
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return ret;
        }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }

}

DEVICE_RETURN_CODE_T CommandManager::startCapture(int deviceID, DEVICE_TYPE_T devType,
        FORMAT sFormat)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE devHandle;
    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    PMLOG_INFO(CONST_MODULE_CM, "Device is open check\n");
    if (devInfo->isDeviceOpen(devType, &deviceID))
    {
        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        if(devInfo->getHandle(deviceID, devType, &devHandle)==DEVICE_OK)
        {
            ret = devCtl->startCapture(devHandle, devType, sFormat);
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return ret;
        }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T CommandManager::stopCapture(int deviceID, DEVICE_TYPE_T devType)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE devHandle;
    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;

    if (devInfo->isDeviceOpen(devType, &deviceID))
    {
        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        if(devInfo->getHandle(deviceID, devType, &devHandle)== DEVICE_OK)
        {
            ret = devCtl->stopCapture(devHandle, devType);
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return ret;
        }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T CommandManager::captureImage(int deviceID, DEVICE_TYPE_T devType, int nCount,
        FORMAT sFormat)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE devHandle;
    if (deviceID == INVALID_ID || devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (devInfo->isDeviceOpen(devType, &deviceID))
    {
        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        if(devInfo->getHandle(deviceID, devType, &devHandle)== DEVICE_OK)
        {
            ret = devCtl->captureImage(devHandle, devType, nCount, sFormat);
            CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
            return ret;
        }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}
