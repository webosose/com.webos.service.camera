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
#include "device_controller.h"
#include "hal.h"
#include <string.h>

#include "service_main.h"

//DEVICE_HANDLE sDevHandle;
//DEVICE_LIST_T sDeviceInfo;
/*Functions for Device Controller*/
DEVICE_RETURN_CODE_T DeviceControl::open(DEVICE_HANDLE devHandle, DEVICE_TYPE devType)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (DEVICE_CAMERA == devType)
    {
        CAMERA_PRINT_INFO("%s : %d calling cam open!", __FUNCTION__, __LINE__);
        ret = hal_cam_open(devHandle);
    }
    else
    {
        //ret = hal_mic_open(deviceID,samplingRate,codec);
    }
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "hal_cam_open failed\n!!");
        return ret;
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}
//Close the device
DEVICE_RETURN_CODE_T DeviceControl::close(DEVICE_HANDLE devHandle, DEVICE_TYPE devType)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int deviceID = 1;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (DEVICE_CAMERA == devType)
    {
        ret = hal_cam_close(devHandle);
    }
    else
    {
        ret = hal_mic_close(deviceID);
    }
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "hal_cam_close failed\n!!");
        return ret;
    }

    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startPreview(DEVICE_HANDLE devHandle, DEVICE_TYPE devType,
        int *pKey)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int deviceID = 1;

    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;

    if (DEVICE_CAMERA == devType)
    {
        FORMAT sFormat;
        //setting the default format
        sFormat.eFormat = CONST_DEFAULT_FORMAT;
        sFormat.nWidth = CONST_DEFAULT_WIDTH;
        sFormat.nHeight = CONST_DEFAULT_HEIGHT;
        ret = hal_cam_set_format(devHandle, sFormat);
        ret = hal_cam_start(devHandle, pKey);
    }
    else
    {
        ret = hal_mic_start(deviceID, pKey);
    }
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "hal_cam_start failed\n!!");
        return ret;
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}
//Stop Capture
DEVICE_RETURN_CODE_T DeviceControl::stopPreview(DEVICE_HANDLE devHandle, DEVICE_TYPE devType)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int deviceID = 1;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;

    if (DEVICE_CAMERA == devType)
    {
        ret = hal_cam_stop(devHandle);
    }
    else
    {
        ret = hal_mic_stop(deviceID);
    }
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "hal_cam_stop failed\n!!");
        return ret;
    }

    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startCapture(DEVICE_HANDLE devHandle, DEVICE_TYPE devType,
        FORMAT sFormat)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (DEVICE_CAMERA == devType)
    {
        ret = hal_cam_start_capture(devHandle, sFormat);
    }
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "hal_cam_start_capture failed \n!!");
        return ret;
    }

    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return ret;
}

DEVICE_RETURN_CODE_T DeviceControl::stopCapture(DEVICE_HANDLE devHandle, DEVICE_TYPE devType)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (DEVICE_CAMERA == devType)
    {
        ret = hal_cam_stop_capture(devHandle);
    }
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "hal_cam_stop_capture failed \n!!");
        return ret;
    }

    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

//Stop Capture
DEVICE_RETURN_CODE_T DeviceControl::captureImage(DEVICE_HANDLE devHandle, DEVICE_TYPE devType,
        int nCount, FORMAT sFormat)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    ret = hal_cam_capture_image(devHandle, nCount, sFormat);
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "hal_cam_capture_image failed \n!!");
        return ret;
    }

    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::createHandle(DEVICE_LIST_T sDeviceInfo,
        DEVICE_HANDLE *sDevHandle)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    ret = hal_cam_create_handle(sDeviceInfo, sDevHandle);
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "Failed at controller_createhandle\n!!");
        return ret;
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getDeviceList(DEVICE_LIST_T *pList, int *pCamDev, int *pMicDev,
        int *pCamSupport, int *pMicSupport, int devCount)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);
    *pCamDev = 0;
    *pMicDev = 0;
    *pCamSupport = 0;
    *pMicSupport = 0;
    for (int i = 0; i < devCount; i++)
    {
        if (strcmp(pList[i].strDeviceType, "CAM") == 0)
        {
            pCamDev[i] = i+1;
            pCamSupport[i] = 1;
        }
        else if (strcmp(pList[i].strDeviceType, "MIC") == 0)
        {
            *pMicDev += 1;
            pMicSupport[i] = 1;
        }
        else
            PMLOG_INFO(CONST_MODULE_DC, "Unknown device\n");
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

bool DeviceControl::isUpdatedCameraList()
{
    //HAL open call()
    return TRUE;

}

DEVICE_RETURN_CODE_T DeviceControl::getdeviceinfo(DEVICE_HANDLE devHandle, DEVICE_TYPE devType,
        CAMERA_INFO_T *pInfo)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int deviceID = 1;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;

    if (DEVICE_CAMERA == devType)
    {
        ret = hal_cam_get_info(devHandle, pInfo);
    }
    else
    {
        MIC_INFO_T sMicInfo;
        ret = hal_mic_get_info(deviceID, &sMicInfo);
    }
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "Failed to get the info\n!!");
        return ret;
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getDeviceProperty(DEVICE_HANDLE sDevHandle,
        DEVICE_TYPE_T devType, CAMERA_PROPERTIES_T *oParams)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int deviceID = 1;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (DEVICE_CAMERA == devType)
    {
        CAMERA_PROPERTIES_INDEX_T nCamProperty;
        //ret = hal_cam_get_property(sDevHandle,nCamProperty,oParams);
    }
    else
    {
        MIC_PROPERTIES_INDEX_T nMicProperty;
        long value1;
        ret = hal_mic_get_property(deviceID, nMicProperty, &value1);
    }
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "Failed to close the seesion\n!!");
        return ret;
    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setDeviceProperty(DEVICE_HANDLE devHandle,
        DEVICE_TYPE_T devType, CAMERA_PROPERTIES_T *oParams)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    int deviceID = 1;
    DEVICE_HANDLE sDevHandle;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;

    //HAL open call()
    if (ret != DEVICE_SESSION_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "Failed to close the seesion\n!!");
        return ret;
    }

    if (DEVICE_CAMERA == devType)
    {

        if (oParams->nZoom != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_ZOOM, oParams->nZoom);

        if (oParams->nGridZoomX != CONST_VARIABLE_INITIALIZE)
        {
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_GRIDZOOMX,
                    oParams->nGridZoomX);
        }
        if (oParams->nGridZoomY != CONST_VARIABLE_INITIALIZE)
        {
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_GRIDZOOMY,
                    oParams->nGridZoomY);
        }

        if (oParams->nPan != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_PAN, oParams->nPan);

        if (oParams->nTilt != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_TILT, oParams->nTilt);

        if (oParams->nContrast != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_CONTRAST, oParams->nContrast);

        if (oParams->nBrightness != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_BRIGHTNESS,
                    oParams->nBrightness);

        if (oParams->nSaturation != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_SATURATION,
                    oParams->nSaturation);

        if (oParams->nSharpness != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_SHARPNESS,
                    oParams->nSharpness);

        if (oParams->nHue != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_HUE, oParams->nHue);

        if (oParams->bAutoExposure != CONST_VARIABLE_INITIALIZE) //autoExposure exposure   setting
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_AUTOEXPOSURE,
                    oParams->bAutoExposure);

        if (oParams->bAutoWhiteBalance != CONST_VARIABLE_INITIALIZE) //autoWhiteBalance whiteBalanceTemperature   setting
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_AUTOWHITEBALANCE,
                    oParams->bAutoWhiteBalance);

        if (oParams->nExposure != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_EXPOSURE, oParams->nExposure);

        if (oParams->nWhiteBalanceTemperature != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_WHITEBALANCETEMPERATURE,
                    oParams->nWhiteBalanceTemperature);

        if (oParams->nGain != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_GAIN, oParams->nGain);

        if (oParams->nGamma != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_GAMMA, oParams->nGamma);

        if (oParams->nFrequency != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_FREQUENCY,
                    oParams->nFrequency);

        if (oParams->bMirror != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_MIRROR, oParams->bMirror);

        if (oParams->nBitrate != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_BITRATE, oParams->nBitrate);

        if (oParams->nFramerate != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_FRAMERATE,
                    oParams->nFramerate);

        if (oParams->ngopLength != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_GOPLENGTH,
                    oParams->ngopLength);

        if (oParams->bLed != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_LED, oParams->bLed);

        if (oParams->bYuvMode != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_YUVMODE, oParams->bYuvMode);

        if (oParams->bBacklightCompensation != CONST_VARIABLE_INITIALIZE)
            ret = hal_cam_set_property(sDevHandle, CAMERA_PROPERTIES_BACKLIGHT_COMPENSATION,
                    oParams->bBacklightCompensation);

    }
    else if (DEVICE_MICROPHONE == devType)
    {
        if (oParams->nMicMaxGain != CONST_VARIABLE_INITIALIZE)
            hal_mic_set_property(deviceID, MIC_PROPERTIES_MAXGAIN, oParams->nMicMaxGain);

        if (oParams->nMicMinGain != CONST_VARIABLE_INITIALIZE)
            hal_mic_set_property(deviceID, MIC_PROPERTIES_MINGAIN, oParams->nMicMinGain);

        if (oParams->nMicGain != CONST_VARIABLE_INITIALIZE)
            hal_mic_set_property(deviceID, MIC_PROPERTIES_GAIN, oParams->nMicGain);

        if (oParams->bMicMute != CONST_VARIABLE_INITIALIZE)
            hal_mic_set_property(deviceID, MIC_PROPERTIES_MUTE, oParams->bMicMute);

    }
    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return DEVICE_OK;
}


DEVICE_RETURN_CODE_T DeviceControl::setformat(DEVICE_HANDLE devHandle, DEVICE_TYPE devType,
        FORMAT sFormat)
{
    CAMERA_PRINT_INFO("%s : %d started!", __FUNCTION__, __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_ERROR_UNKNOWN;
    if (devType == DEVICE_DEVICE_UNDEFINED)
        return DEVICE_ERROR_WRONG_PARAM;
    if (DEVICE_CAMERA == devType)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "sFormat %d %d %d\n",sFormat.nHeight,sFormat.nWidth,sFormat.eFormat);
        ret = hal_cam_set_format(devHandle, sFormat);
    }
    if (ret != DEVICE_OK)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "hal_cam_set_format failed \n!!");
        return ret;
    }

    CAMERA_PRINT_INFO("%s:%d] ended!", __FUNCTION__, __LINE__);
    return ret;
}


