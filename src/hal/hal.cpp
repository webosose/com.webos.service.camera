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

/** @file hal.cpp
 *
 *  API functions for Camera
 *
 */
/******************************************************************************
 File Inclusions
 ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "libudev.h"
#include <time.h>
#include "hal.h"
#include "camshm.h"
#include "hal_v4l2.h"
#include "hal_alsa.h"

/******************************************************************************
 Macro Definitions
 ******************************************************************************/

//#define CONST_MAX_STRING_LENGTH       100
const int AUDIO_FRAME_SIZE = 640;    // (2 * 2 * 16000 * 10 / 1000 * 10) ?
const int AUDIO_FRAME_COUNT = 200;

/******************************************************************************/
const int VIDEO_FRAME_SIZE = 640 * 480 * 2 + 1024;
const int VIDEO_FRAME_COUNT = 8;
/******************************************************************************
 Local Type Definitions
 ******************************************************************************/

typedef enum _PLAY_STATE
{
    STATE_STOP = 0, STATE_START = 1, STATE_OPEN = 2, STATE_CLOSE = 3,
} PLAY_STATE;

typedef struct _CAMERA_DEVICE
{
    //Device
    char strDeviceName[256];
    int nDeviceID;  // device num
    int isDeviceOpen;   // Device status
    int nMode;      // open mode video(ES/TS/YUV)
    PLAY_STATE nState;     // state of camera device (PLAY, STOP)

    // Shared Memory
    SHMEM_HANDLE hShm;       // handle of shared memory for internal engine
    pthread_mutex_t Mutex;
    pthread_cond_t Cond;

} CAMERA_DEVICE;

typedef struct _MIC_DEVICE
{
    // For device
    char strDeviceName[256];
    int nDeviceID;  // device num
    int isDeviceOpen;   // device num
    PLAY_STATE nState;     // state of camera device (PLAY, STOP)

    // For modules
    SHMEM_HANDLE hShm;       // handle of shared memory for internal engine
    pthread_mutex_t Mutex;
    pthread_cond_t Cond;

} MIC_DEVICE;

static int gCamCount = 0;
/******************************************************************************
 Static Variables & Function Prototypes Declarations
 ******************************************************************************/

// Global strcuture for mmap buffers
static CAMERA_DEVICE gCameraCamList[10];
static MIC_DEVICE gMicInfo[10];

// Callback from HAL
static void _device_init(DEVICE_LIST_T sDeviceInfo, DEVICE_HANDLE *sDevHandle);
static void _cb_michal(int nDeviceID, int stream_type, unsigned int data_length, unsigned char *data,
        unsigned int timestamp);
static int _camera_find(int nDeviceId);
static void _cb_camhal(int nDeviceID, int stream_type, unsigned int data_length,
        unsigned char *data, unsigned int timestamp);

/******************************************************************************
 Function Definitions
 ******************************************************************************/

DEVICE_RETURN_CODE_T hal_cam_capture_image(DEVICE_HANDLE sDevHandle, int nCount,
        CAMERA_FORMAT sFormat)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int camNum;

    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    PMLOG_INFO(CONST_MODULE_HAL, "camera Number:%d\n!!", camNum);
    PMLOG_INFO(CONST_MODULE_HAL, "%d: Started!camera name=%s\n", __LINE__,
            gCameraCamList[camNum].strDeviceName);
    nRet = v4l2_cam_capture_image(gCameraCamList[camNum].strDeviceName, nCount);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: cam capture image failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_get_format(DEVICE_HANDLE sDevHandle, CAMERA_FORMAT *sFormat)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int camNum = 0;

    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    nRet = v4l2_cam_get_format(gCameraCamList[camNum].strDeviceName, sFormat);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: set format failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_stop_capture(DEVICE_HANDLE sDevHandle)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int camNum;

    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    if (gCameraCamList[camNum].nState == STATE_STOP)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "camera is already stopped\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
    }
    else if (gCameraCamList[camNum].nState == STATE_CLOSE)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "camera is already closed\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
    }
    PMLOG_INFO(CONST_MODULE_HAL, "%d: Stop capture!strDeviceName=%s\n", __LINE__,
            gCameraCamList[camNum].strDeviceName);
    nRet = v4l2_cam_stop_capture(gCameraCamList[camNum].strDeviceName);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: stop capture failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_start_capture(DEVICE_HANDLE sDevHandle, CAMERA_FORMAT sFormat)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int camNum;

    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    PMLOG_INFO(CONST_MODULE_HAL, "%d: Started!strDeviceName=%s\n", __LINE__,
            gCameraCamList[camNum].strDeviceName);
    nRet = v4l2_cam_start_capture(gCameraCamList[camNum].strDeviceName, sFormat);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: start capture failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_set_format(DEVICE_HANDLE sDevHandle, CAMERA_FORMAT sFormat)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int camNum;

    PMLOG_INFO(CONST_MODULE_HAL, "sDevHandle.nDevId:%d\n", sDevHandle.nDevId);
    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    PMLOG_INFO(CONST_MODULE_HAL, "camNum:%d\n", camNum);
    PMLOG_INFO(CONST_MODULE_HAL, "%d: Started!strDeviceName=%s\n", __LINE__,
            gCameraCamList[camNum].strDeviceName);
    nRet = v4l2_cam_set_format(gCameraCamList[camNum].strDeviceName, sFormat);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: set format failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_get_list(int* nCameraCount, int cameraType[])
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int i;

    nRet = v4l2_cam_get_list(nCameraCount, cameraType);
    PMLOG_INFO(CONST_MODULE_HAL, "nCameraCount = %d\n", *nCameraCount);
    for (i = 0; i < *nCameraCount; i++)
        PMLOG_INFO(CONST_MODULE_HAL, "cameraType:%d\n", cameraType[i]);

    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: query device list failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_get_property(DEVICE_HANDLE sDevHandle,
        CAMERA_PROPERTIES_INDEX_T nProperty, int *value)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int camNum;

    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    nRet = v4l2_cam_get_property(gCameraCamList[camNum].strDeviceName, nProperty, value);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: get property failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_set_property(DEVICE_HANDLE sDevHandle,
        CAMERA_PROPERTIES_INDEX_T nProperty, int value)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int camNum;

    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    nRet = v4l2_cam_set_property(gCameraCamList[camNum].strDeviceName, nProperty, value);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: set property failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_get_info(DEVICE_LIST_T sDeviceInfo, CAMERA_INFO_T *pInfo)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    DEVICE_HANDLE sDevHandle;
    int camNum = 0;

    _device_init(sDeviceInfo, &sDevHandle);
    PMLOG_INFO(CONST_MODULE_DC, "%d: Started!strDeviceName=%s\n", __LINE__,
            sDevHandle.strDeviceNode);
    nRet = v4l2_cam_get_info(sDevHandle.strDeviceNode, pInfo);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: query device info failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_stop(DEVICE_HANDLE sDevHandle)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int camNum = 0;

    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    if (gCameraCamList[camNum].nState == STATE_STOP)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "camera is already stopped\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
    }
    else if (gCameraCamList[camNum].nState == STATE_CLOSE)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "camera is already closed\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
    }
    PMLOG_INFO(CONST_MODULE_HAL, "%d: Started!strDeviceName=%s\n", __LINE__,
            gCameraCamList[camNum].strDeviceName);
    nRet = v4l2_cam_stop(gCameraCamList[camNum].strDeviceName);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: cam stop failed\n", __LINE__);
    else
    {
        gCameraCamList[camNum].nState = STATE_STOP;
    }
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_start(DEVICE_HANDLE sDevHandle, int *pKey)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int nMemSize = VIDEO_FRAME_SIZE;
    int nMemCount = VIDEO_FRAME_COUNT;
    int camNum = 0;

    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    if (gCameraCamList[camNum].nState == STATE_START)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "camera is already started\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STARTED;
    }
    else if (gCameraCamList[camNum].nState == STATE_CLOSE)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "camera is already closed\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
    }
    PMLOG_INFO(CONST_MODULE_HAL, "%d: Started!strDeviceName=%s\n", __LINE__,
            gCameraCamList[camNum].strDeviceName);

    CreateShmemEx(&gCameraCamList[camNum].hShm, pKey, nMemSize, nMemCount, sizeof(unsigned int));

    PMLOG_INFO(CONST_MODULE_HAL, "%d: shared mem key:%d\n", __LINE__, *pKey);
    nRet = v4l2_cam_start(gCameraCamList[camNum].strDeviceName);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: cam start failed\n", __LINE__);
    else
    {
        gCameraCamList[camNum].nState = STATE_START;
    }
    return nRet;
}

DEVICE_RETURN_CODE_T hal_cam_close(DEVICE_HANDLE sDevHandle)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    int nRetShmem;
    int camNum = 0;
    //Parse device Id and get the camera Num
    camNum = _camera_find(sDevHandle.nDevId);
    PMLOG_INFO(CONST_MODULE_HAL, "Closing device!strDeviceName=%s\n",
            gCameraCamList[camNum].strDeviceName);

    if (gCameraCamList[camNum].nState == STATE_CLOSE)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "camera is already closed\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED;
    }
    if (gCameraCamList[camNum].nState != STATE_STOP)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "camera is not stopped\nstopping cam before closing\n");
        nRet = hal_cam_stop(sDevHandle);
        if (DEVICE_OK != nRet)
        {
            PMLOG_INFO(CONST_MODULE_HAL, "camera stop failed so proceeding with close\n");
        }
    }
    /*Close shared memory*/
    nRetShmem = CloseShmem(&gCameraCamList[camNum].hShm);
    if (nRetShmem != SHMEM_COMM_OK)
        PMLOG_INFO(CONST_MODULE_HAL, "%d:CloseShmem err : %d", __LINE__, nRetShmem);

    gCameraCamList[camNum].nState = STATE_CLOSE;

    gCameraCamList[camNum].nMode = 0;

    gCameraCamList[camNum].hShm = NULL;
    pthread_mutex_destroy(&gCameraCamList[camNum].Mutex);
    pthread_cond_destroy(&gCameraCamList[camNum].Cond);

    nRet = v4l2_cam_close(gCameraCamList[camNum].strDeviceName);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "v4l2 cam close failed\n");
    else
    {
        PMLOG_INFO(CONST_MODULE_HAL, "Camera_close success");
        gCameraCamList[camNum].isDeviceOpen = false;
    }
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: cam close failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_mic_get_property(int nMicNum, MIC_PROPERTIES_INDEX_T nProperty,
        long *value)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_OK;
    int micNum;
    PMLOG_INFO(CONST_MODULE_HAL, "set Property\n");

    micNum = nMicNum - 1;
    nRet = alsa_mic_get_property(micNum, nProperty, value);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: set property failed\n", __LINE__);
    return nRet;

}

DEVICE_RETURN_CODE_T hal_mic_set_property(int nMicNum, MIC_PROPERTIES_INDEX_T nProperty, long value)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_OK;
    int micNum;
    PMLOG_INFO(CONST_MODULE_HAL, "set Property\n");

    micNum = nMicNum - 1;
    nRet = alsa_mic_set_property(micNum, nProperty, value);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: set property failed\n", __LINE__);
    return nRet;

}

DEVICE_RETURN_CODE_T hal_mic_get_list(int *pDevCount)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_OK;
    PMLOG_INFO(CONST_MODULE_HAL, "Get list\n");

    nRet = alsa_mic_get_list(pDevCount);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: get listfailed\n", __LINE__);
    return nRet;

}

DEVICE_RETURN_CODE_T hal_mic_get_info(int nMicNum, MIC_INFO_T *pInfo)
{
    DEVICE_RETURN_CODE_T nRet = DEVICE_OK;
    int micNum = nMicNum - 1;
    PMLOG_INFO(CONST_MODULE_HAL, "Querying device info!nDeviceID=%d\n", micNum);

    nRet = alsa_mic_get_info(micNum, pInfo);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: query device info failed\n", __LINE__);
    return nRet;
}

DEVICE_RETURN_CODE_T hal_mic_close(int nMicNum)
{
    PMLOG_INFO(CONST_MODULE_HAL, "%d: in hal_mic_stop\n", __LINE__);
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    MIC_DEVICE *pDev;
    int micNum = 0;

    /*Parse the mic Num from device id*/
    micNum = nMicNum - 1;
    pDev = &gMicInfo[micNum];
    if (pDev->nState != STATE_STOP)
    {
        ret = hal_mic_stop(nMicNum);
    }
    if (DEVICE_OK == ret)
    {
        ret = alsa_mic_close(micNum);
        if (ret != DEVICE_OK)
        {
            PMLOG_INFO(CONST_MODULE_HAL, "mic_open failed\n");
            return ret;
        }
        pDev->nState = STATE_CLOSE;
    }
    return ret;
}

DEVICE_RETURN_CODE_T hal_mic_stop(int nMicNum)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    MIC_DEVICE *pDev;
    int micNum = nMicNum - 1;
    int nRetShmem;

    /*Parse the mic Num from device id*/
    pDev = &gMicInfo[micNum];
    ret = alsa_mic_stop(micNum);
    if (ret != DEVICE_OK)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "mic_stop failed\n");
        return ret;
    }
    nRetShmem = CloseShmem(&gCameraCamList[0].hShm);
    if (nRetShmem != SHMEM_COMM_OK)
        PMLOG_INFO(CONST_MODULE_HAL, "%d:CloseShmem err : %d", __LINE__, nRetShmem);
    pDev->nState = STATE_STOP;
    return ret;
}

DEVICE_RETURN_CODE_T hal_mic_start(int nMicNum, int *pKey)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    MIC_DEVICE *pDev;
    int nMemSize = 0;
    int nMemCount = 0;
    int micNum = nMicNum - 1;

    nMemSize = AUDIO_FRAME_SIZE;
    nMemCount = AUDIO_FRAME_COUNT;

    PMLOG_INFO(CONST_MODULE_HAL, "%d: in hal_mic_start\n", __LINE__);
    /*Parse the mic Num from device id*/
    pDev = &gMicInfo[micNum];
    PMLOG_INFO(CONST_MODULE_HAL, "%d: in hal_mic_start\n", __LINE__);
    CreateShmemEx(&gMicInfo[micNum].hShm, pKey, nMemSize, nMemCount, sizeof(unsigned int));
    PMLOG_INFO(CONST_MODULE_HAL, "%d:shared memopry key is :%d\n", __LINE__, *pKey);
    ret = alsa_mic_start(micNum);
    if (ret != DEVICE_OK)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "mic_open failed\n");
        return ret;
    }
    pDev->nState = STATE_START;
    return ret;
}

DEVICE_RETURN_CODE_T hal_mic_open(int nMicNum, int samplingRate, int codec, int *pKey)
{

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int micNum = nMicNum - 1;

    /*Parse the mic Num from device id*/
    pthread_mutex_init(&gMicInfo[micNum].Mutex, NULL);
    pthread_cond_init(&gMicInfo[micNum].Cond, NULL);
    ret = alsa_mic_open(micNum, samplingRate, codec);
    if (ret != DEVICE_OK)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "mic_open failed\n");
        return ret;
    }

    alsa_mic_register_callback(micNum, _cb_michal);
    PMLOG_INFO(CONST_MODULE_HAL, "Ended!\n");
    gMicInfo[micNum].isDeviceOpen = true;
    gMicInfo[micNum].nState = STATE_OPEN;
    gMicInfo[micNum].nDeviceID = micNum;
    return ret;
}

static void _device_init(DEVICE_LIST_T sDeviceInfo, DEVICE_HANDLE *sDevHandle)
{
    struct udev *camudev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    camudev = udev_new();
    if (!camudev)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "FAIL: To create udev for camera\n");
    }
    enumerate = udev_enumerate_new(camudev);
    udev_enumerate_add_match_subsystem(enumerate, "video4linux");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices)
    {
        const char *path;
        const char *strDeviceNode;
        const char *subSystemType;
        const char *devnum;
        const char *busnum;
        int nDeviceNumber;

        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(camudev, path);
//Information required
        strDeviceNode = udev_device_get_devnode(dev);
        PMLOG_INFO(CONST_MODULE_HAL, "Device Node:%s\n", path);
        PMLOG_INFO(CONST_MODULE_HAL, "Device Node:%s\n", strDeviceNode);
        subSystemType = udev_device_get_subsystem(dev);
        PMLOG_INFO(CONST_MODULE_HAL, "devsubsystem:%s\n", subSystemType);
//Information required

        dev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
        if (!dev)
        {
            PMLOG_INFO(CONST_MODULE_HAL, "Unable to find parent usb device.");
            exit(1);
        }
        devnum = udev_device_get_sysattr_value(dev, "devnum");
        nDeviceNumber = atoi(devnum);
        busnum = udev_device_get_sysattr_value(dev, "busnum");
        /*PDM support is not there for device number*/
        //if(sDeviceInfo.nDeviceNum == nDeviceNumber)
        {
            strcpy(sDevHandle->strDeviceNode, strDeviceNode);
        }

        udev_device_unref(dev);
    }
}

DEVICE_RETURN_CODE_T hal_cam_create_handle(DEVICE_LIST_T sDeviceInfo, DEVICE_HANDLE *pDevHandle)
{
    int cameraNum = 0;
    int nDeviceId = -1;

    _device_init(sDeviceInfo, pDevHandle);
    cameraNum = _camera_find(nDeviceId);
    PMLOG_INFO(CONST_MODULE_HAL, "cameraNum:%d\n", cameraNum);
    PMLOG_INFO(CONST_MODULE_HAL, "cameraNum->deviceId:%d\n", gCameraCamList[cameraNum].nDeviceID);
    pDevHandle->nDevId = gCameraCamList[cameraNum].nDeviceID;
    strcpy(gCameraCamList[cameraNum].strDeviceName, pDevHandle->strDeviceNode);
    return DEVICE_OK;
}
DEVICE_RETURN_CODE_T hal_cam_open(DEVICE_HANDLE pDevHandle)
{

    int nMemSize = 0;
    int cameraNum = pDevHandle.nDevId - 1;
    int nMemCount = 0;
    DEVICE_RETURN_CODE_T nRet = DEVICE_ERROR_UNKNOWN;
    CAMERA_DEVICE *pDev;
    //struct usb_device *usb_dev;
    //struct usb_dev_handle *usb_handle;

    //todo : parse deviceId string and pass the number

    //check for the present camera and assign a new device id to the new camera

    cameraNum = _camera_find(pDevHandle.nDevId);
    PMLOG_INFO(CONST_MODULE_HAL, "634:cameraNum:%d\n", cameraNum);

    if ((gCameraCamList[cameraNum].nState == STATE_OPEN)
            || (gCameraCamList[cameraNum].nState == STATE_START)
            || (gCameraCamList[cameraNum].nState == STATE_STOP))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "camera is already open\n");
        return DEVICE_ERROR_DEVICE_IS_ALREADY_OPENED;
    }

    nMemSize = VIDEO_FRAME_SIZE;
    nMemCount = VIDEO_FRAME_COUNT;

    nRet = v4l2_cam_open(gCameraCamList[cameraNum].strDeviceName);
    if (DEVICE_OK != nRet)
        PMLOG_INFO(CONST_MODULE_HAL, "%d: v4l2 cam open failed\n", __LINE__);
    else
    {
        PMLOG_INFO(CONST_MODULE_HAL, "Camera_open success");
        gCameraCamList[cameraNum].isDeviceOpen = true;
    }

    v4l2_cam_registerCallback(gCameraCamList[cameraNum].strDeviceName, _cb_camhal);
    if (nRet == DEVICE_OK)
    {
        pDev = &gCameraCamList[cameraNum];
        //CreateShmemEx(&gCameraCamList[cameraNum].hShm, pKey, nMemSize, nMemCount, sizeof(unsigned int));
        gCameraCamList[cameraNum].nState = STATE_OPEN;
    }

    PMLOG_INFO(CONST_MODULE_HAL, "%d: Ended!\n", __LINE__);
    return nRet;
}

/******************************************************************************
 Local Function Definitions
 ******************************************************************************/

static void _cb_michal(int nDeviceID, int stream_type, unsigned int data_length, unsigned char *data,
        unsigned int timestamp)
{
    MIC_DEVICE *pDev;
    int i;
    int nStatus;

    for (i = 0; i < 10; i++)
    {
        pDev = &gMicInfo[nDeviceID];
        pthread_mutex_lock(&pDev->Mutex);
        pthread_cond_signal(&pDev->Cond);
        pthread_mutex_unlock(&pDev->Mutex);

        nStatus = WriteShmemEx(pDev->hShm, data, data_length, (unsigned char *) &timestamp,
                sizeof(timestamp));
    }
}

static void _cb_camhal(int nDeviceID, int stream_type, unsigned int data_length,
        unsigned char *data, unsigned int timestamp)
{
    CAMERA_DEVICE *pDev;
    int nStatus;

    {
        pDev = &gCameraCamList[nDeviceID];
        pthread_mutex_lock(&pDev->Mutex);
        pthread_cond_signal(&pDev->Cond);
        pthread_mutex_unlock(&pDev->Mutex);

        nStatus = WriteShmemEx(pDev->hShm, data, data_length, (unsigned char *) &timestamp,
                sizeof(timestamp));
    }
}

static int _camera_find(int nCameraNum)
{
    int i = 0;
    int nDeviceID = 0;
    for (i = 0; i < gCamCount; i++)
    {
        if (gCameraCamList[i].nDeviceID == nCameraNum)
        {
            return i;
        }
    }
    PMLOG_INFO(CONST_MODULE_HAL, "In camera allocate:%d\n", i);
    gCamCount++;

    nDeviceID = i;
    gCameraCamList[nDeviceID].nDeviceID = i + 1;
    gCameraCamList[nDeviceID].nState = STATE_CLOSE;

    gCameraCamList[nDeviceID].nMode = CAMERA_FORMAT_YUV;

    gCameraCamList[nDeviceID].hShm = NULL;
    pthread_mutex_init(&gCameraCamList[nDeviceID].Mutex, NULL);
    pthread_cond_init(&gCameraCamList[nDeviceID].Cond, NULL);

    return nDeviceID;

}
