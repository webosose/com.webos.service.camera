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

/*-----------------------------------------------------------------------------
 Static  Static prototype
 (Static Variables & Function Prototypes Declarations)
 ------------------------------------------------------------------------------*/
extern DEVICE_LIST_T arrDevList[MAX_DEVICE];

DEVICE_RETURN_CODE_T CommandManager::open(int deviceID, DEVICE_TYPE_T devType,int *devhandle)
{
    PMLOG_INFO(CONST_MODULE_CM,"open started! deviceID : %d\n",deviceID);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    if (DeviceManager::getInstance().isDeviceValid(devType, &deviceID))
    {
        if(INVALID_ID == deviceID)
            return DEVICE_ERROR_NODEVICE;
        else
            return DEVICE_ERROR_DEVICE_IS_ALREADY_OPENED;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        ret = DeviceManager::getInstance().createHandle(deviceID, devType, devhandle);
        if (DEVICE_OK != ret)
        {
            PMLOG_ERROR(CONST_MODULE_CM, "Failed to create device handle\n");
            return ret;
        }
        DEVICE_HANDLE st_devhandle;
        DeviceManager::getInstance().getHandle(*devhandle, devType, &st_devhandle);
        ret = DeviceControl::getInstance().open(st_devhandle, devType);
        if (DEVICE_OK == ret)
        {
            DeviceManager::getInstance().deviceStatus(*devhandle, devType, TRUE);
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_CM, "Failed to open device\n");
        }
    }

    return ret;
}

DEVICE_RETURN_CODE_T CommandManager::close(int deviceID, DEVICE_TYPE_T devType)
{
    PMLOG_INFO(CONST_MODULE_CM,"close started! deviceID : %d\n",deviceID);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    if ( (INVALID_ID == deviceID) || (DEVICE_DEVICE_UNDEFINED == devType) )
        return DEVICE_ERROR_WRONG_PARAM;

    if (DeviceManager::getInstance().isDeviceOpen(devType, &deviceID))
    {
        DEVICE_HANDLE st_devhandle;
        DeviceManager::getInstance().getHandle(deviceID, devType, &st_devhandle);
        ret = DeviceControl::getInstance().close(st_devhandle, devType);
        if (DEVICE_OK == ret)
        {
            DeviceManager::getInstance().deviceStatus(deviceID, devType, FALSE);
            PMLOG_INFO(CONST_MODULE_DM, "Device Closed successfully\n");
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_DM, "Requested device is not closed %d", deviceID);
            ret = DEVICE_ERROR_CAN_NOT_CLOSE;
        }
    }
    else
        ret =  DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;

    return ret;

    PMLOG_INFO(CONST_MODULE_CM,"CommandManager::close ended! \n");
}

DEVICE_RETURN_CODE_T CommandManager::getDeviceInfo(int deviceID, DEVICE_TYPE_T devType,
        CAMERA_INFO_T *pInfo)
{
    PMLOG_INFO(CONST_MODULE_CM,"getDeviceInfo started! deviceID : %d\n",deviceID);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    if ( (INVALID_ID == deviceID) || (DEVICE_DEVICE_UNDEFINED == devType) )
        return DEVICE_ERROR_WRONG_PARAM;

    ret = DeviceManager::getInstance().getInfo(deviceID,pInfo);

    PMLOG_INFO(CONST_MODULE_CM,"CommandManager::getDeviceInfo ended! \n");
    return ret;
}

DEVICE_RETURN_CODE_T CommandManager::getDeviceList(int *pCamDev, int *pMicDev, int *pCamSupport,
        int *pMicSupport)
{
    PMLOG_INFO(CONST_MODULE_CM,"CommandManager::getDeviceList started! \n");
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    ret = DeviceManager::getInstance().getList(pCamDev, pMicDev, pCamSupport, pMicSupport);
    if (DEVICE_OK != ret)
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Failed to get device list\n");
    }

    return ret;
}

DEVICE_RETURN_CODE_T CommandManager::createHandle(int deviceID, DEVICE_TYPE_T devType,
        int *devhandle)
{
    PMLOG_INFO(CONST_MODULE_CM,"createHandle started! deviceID : %d\n",deviceID);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    ret = DeviceManager::getInstance().createHandle(deviceID, devType, devhandle);
    if (DEVICE_OK != ret)
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Failed to get device list\n");
    }

    return ret;
}

DEVICE_RETURN_CODE_T CommandManager::updateList(DEVICE_LIST_T *pList, int nDevCount,
        DEVICE_EVENT_STATE_T *pCamEvent, DEVICE_EVENT_STATE_T *pMicEvent)
{
    PMLOG_INFO(CONST_MODULE_CM,"updateList nDevCount : %d\n",nDevCount);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    ret = DeviceManager::getInstance().updateList(pList, nDevCount, pCamEvent, pMicEvent);
    if (DEVICE_OK != ret)
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Failed to get device list\n");
    }

    return ret;
}

DEVICE_RETURN_CODE_T CommandManager::getProperty(int deviceID, DEVICE_TYPE_T devType,
        CAMERA_PROPERTIES_T *devproperty)
{
    PMLOG_INFO(CONST_MODULE_CM,"getProperty deviceID : %d\n",deviceID);

    if ( (INVALID_ID == deviceID) ||
         (DEVICE_DEVICE_UNDEFINED == devType) ||
         (NULL == devproperty) )
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

    if (DeviceManager::getInstance().isDeviceOpen(devType, &deviceID))
    {
        PMLOG_INFO(CONST_MODULE_CM, "Device is open \n");
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE sdevhandle;
    if(DEVICE_OK == DeviceManager::getInstance().getHandle(deviceID, devType, &sdevhandle))
    {
        ret = DeviceControl::getInstance().getDeviceProperty(sdevhandle, devType, devproperty);
        return ret;
    }
    else
        return DEVICE_ERROR_NODEVICE;
}

DEVICE_RETURN_CODE_T CommandManager::setProperty(int deviceID, DEVICE_TYPE_T devType,
        CAMERA_PROPERTIES_T *oInfo)
{
    PMLOG_INFO(CONST_MODULE_CM,"setProperty deviceID : %d\n",deviceID);

    if ( (INVALID_ID == deviceID) || (DEVICE_DEVICE_UNDEFINED == devType) )
        return DEVICE_ERROR_WRONG_PARAM;

    DEVICE_RETURN_CODE_T ret;
    DEVICE_HANDLE devhandle;
    if (DEVICE_CAMERA == devType)
    {
        if (DeviceManager::getInstance().isDeviceOpen(devType, &deviceID))
        {
            PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
            if(DEVICE_OK == DeviceManager::getInstance().getHandle(deviceID, devType, &devhandle))
            {
                ret = DeviceControl::getInstance().setDeviceProperty(devhandle, devType, oInfo);
                return ret;
            }
            else
                return DEVICE_ERROR_NODEVICE;
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
            return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
        }
    }
    else if (DEVICE_MICROPHONE == devType)
    {
        ret = DeviceControl::getInstance().setDeviceProperty(devhandle, devType, oInfo);
        return ret;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device is not valid \n!!");
        return DEVICE_ERROR_NODEVICE;
    }
}

DEVICE_RETURN_CODE_T CommandManager::setFormat(int deviceID, DEVICE_TYPE_T devType,
        FORMAT oFormat)
{
    PMLOG_INFO(CONST_MODULE_CM, "setFormat started : deviceID : %d\n",deviceID);

    if ( (INVALID_ID == deviceID) || (DEVICE_DEVICE_UNDEFINED == devType) )
        return DEVICE_ERROR_WRONG_PARAM;

    if (DEVICE_CAMERA == devType)
    {
        if (DeviceManager::getInstance().isDeviceOpen(devType, &deviceID))
        {
            PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
            DEVICE_HANDLE devhandle;
            if(DEVICE_OK == DeviceManager::getInstance().getHandle(deviceID, devType, &devhandle))
            {
                DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().setFormat(devhandle, devType, oFormat);
                return ret;
            }
            else
                return DEVICE_ERROR_NODEVICE;
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_CM, "Device not open %d", deviceID);
            return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
        }
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device is not valid \n!!");
        return DEVICE_ERROR_NODEVICE;
    }
}

DEVICE_RETURN_CODE_T CommandManager::startPreview(int deviceID, DEVICE_TYPE_T devType, int *pKey)
{
    PMLOG_INFO(CONST_MODULE_CM, "startPreview started : deviceID : %d\n",deviceID);

    if ( (INVALID_ID == deviceID) || (DEVICE_DEVICE_UNDEFINED == devType) )
        return DEVICE_ERROR_WRONG_PARAM;

    if (DeviceManager::getInstance().isDeviceOpen(devType, &deviceID))
    {
        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        DEVICE_HANDLE devhandle;
        if(DEVICE_OK == DeviceManager::getInstance().getHandle(deviceID, devType, &devhandle))
        {
            DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().startPreview(devhandle, devType,pKey);
            return ret;
         }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T CommandManager::stopPreview(int deviceID, DEVICE_TYPE_T devType)
{
    PMLOG_INFO(CONST_MODULE_CM, "stopPreview started : deviceID : %d\n",deviceID);

    if ( (INVALID_ID == deviceID) || (DEVICE_DEVICE_UNDEFINED == devType) )
        return DEVICE_ERROR_WRONG_PARAM;

    if (DeviceManager::getInstance().isDeviceOpen(devType, &deviceID))
    {
        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        DEVICE_HANDLE devhandle;
        if(DEVICE_OK == DeviceManager::getInstance().getHandle(deviceID, devType, &devhandle))
        {
            DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().stopPreview(devhandle, devType);
            return ret;
        }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T CommandManager::startCapture(int deviceID, DEVICE_TYPE_T devType,
        FORMAT sFormat)
{
    PMLOG_INFO(CONST_MODULE_CM, "startCapture started : deviceID : %d\n",deviceID);

    if ( (INVALID_ID == deviceID) || (DEVICE_DEVICE_UNDEFINED == devType) )
        return DEVICE_ERROR_WRONG_PARAM;

    if (DeviceManager::getInstance().isDeviceOpen(devType, &deviceID))
    {
        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        DEVICE_HANDLE devHandle;
        if(DEVICE_OK == DeviceManager::getInstance().getHandle(deviceID, devType, &devHandle))
        {
            DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().startCapture(devHandle, devType, sFormat);
            return ret;
        }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T CommandManager::stopCapture(int deviceID, DEVICE_TYPE_T devType)
{
    PMLOG_INFO(CONST_MODULE_CM, "stopCapture started : deviceID : %d\n",deviceID);

    if ( (INVALID_ID == deviceID) || (DEVICE_DEVICE_UNDEFINED == devType) )
        return DEVICE_ERROR_WRONG_PARAM;

    if (DeviceManager::getInstance().isDeviceOpen(devType, &deviceID))
    {
        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        DEVICE_HANDLE devhandle;
        if(DEVICE_OK == DeviceManager::getInstance().getHandle(deviceID, devType, &devhandle))
        {
            DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().stopCapture(devhandle, devType);
            return ret;
        }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}

DEVICE_RETURN_CODE_T CommandManager::captureImage(int deviceID, DEVICE_TYPE_T devType, int nCount,
        FORMAT sFormat)
{
    PMLOG_INFO(CONST_MODULE_CM, "captureImage started : deviceID : %d\n",deviceID);

    if ( (INVALID_ID == deviceID) || (DEVICE_DEVICE_UNDEFINED == devType) )
        return DEVICE_ERROR_WRONG_PARAM;

    if (DeviceManager::getInstance().isDeviceOpen(devType, &deviceID))
    {
        PMLOG_INFO(CONST_MODULE_CM, "Device is open\n");
        DEVICE_HANDLE devhandle;
        if(DEVICE_OK == DeviceManager::getInstance().getHandle(deviceID, devType, &devhandle))
        {
            DEVICE_RETURN_CODE_T ret = DeviceControl::getInstance().captureImage(devhandle, devType, nCount, sFormat);
            return ret;
        }
        else
            return DEVICE_ERROR_NODEVICE;
    }
    else
    {
        PMLOG_ERROR(CONST_MODULE_CM, "Device not open\n");
        return DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
    }
}
