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

#ifdef __cplusplus
extern "C" {
#endif

#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h> //access


/******************************************************************************
    Macro Definitions
******************************************************************************/

/******************************************************************************
    Local Constant Definitions
******************************************************************************/
#define DEFAULT_FRAME_SIZE          640*480*2
#define DEFAULT_WIDTH               640
#define DEFAULT_HEIGHT              480
#define DEFAULT_BITRATE         3000*1024
#define DEFAULT_FRAMERATE           30  // 0~30 fps
#define DEFAULT_GOPLENGTH           480
#define DEFAULT_ZOOM_LEVEL      0   // 0~8
// 0~100
#define DEFAULT_BRIGHTNESS          128
#define DEFAULT_CONTRAST            128
#define DEFAULT_EXPOSURE            128
#define DEFAULT_GAIN                128
#define DEFAULT_GAMMA               128
#define DEFAULT_HUE             0
#define DEFAULT_SATURATION      128
#define DEFAULT_SHARPNESS           128
#define DEFAULT_WBTEMPERATURE   5500

#define DEFAULT_FREQUENCY           60  // 50Hz, 60Hz
#define DEFAULT_GRIDZOOM_X      0   // -10~10
#define DEFAULT_GRIDZOOM_Y      0   // -10~10

#define MAX_MEMORY_MAP          4

#define V4L_PATH                    "/sys/class/video4linux/"
#define ASM_PICTURE_PATH            "/mnt/lg/appstore/cam/"
#define USB_PICTURE_PATH            "/LG SMART TV/DCIM/"

#define FLOAT_FROM_Q8_23_T(val) ((float)(val)*(1.0f/(float)(1<<23)))
#define CHANGE_VALUE_FOR_CNXT_DATA(val) (float)(int)(((val)>>23)|(0xFFFFFE00))


typedef void (*pfpDataCB)(int nDevNum, int nStreamType, unsigned int nDataLength, unsigned char *pData, unsigned int nTimestamp);
/******************************************************************************
    Local Type Definitions
******************************************************************************/
typedef enum {
    NO_DEVICE,
    NOT_OPENED,
    OPEN,
    STREAM_ON,
}CAM_STATUS;


typedef struct CAM_MM_BUFFER{
    void *start;
    size_t length;
} CAM_MM_BUFFER_T;

typedef struct CAM_DEVICE_INFO
{
    char strCameraName[256];
    unsigned int IsBuiltIn;
    unsigned int IsRecognitionSupport;
    unsigned int maxPictureWidth;
    unsigned int maxPictureHeight;
    unsigned int maxVideoWidth;
    unsigned int maxVideoHeight;
    unsigned int supportPictureFormat;
    unsigned int supportVideoFormat;
}CAM_DEVICE_INFO_T;


typedef struct CAM_DEVICE
{
    char strDeviceName[256];

    int cameraNum;
    int usbPortNum;
    int usbDevNum;

    int hCamfd;
    CAM_STATUS CameraStatus;

    CAM_MM_BUFFER_T hMMbuffers[MAX_MEMORY_MAP];
    unsigned int    nMMbuffers;

    pfpDataCB pDataCB;

    int nVideoWidth, nVideoHeight, nBitrate, nFramerate, nGOPLength;
    CAMERA_FORMAT_T nVideoMode;
    int nProperties[64];

    int isStreamOn;
    int isDeviceOpen;
    pthread_mutex_t hCaptureThread;
    pthread_cond_t hCaptureThreadCond;

    pthread_t loop_thread;
    pthread_t capture_thread;

    int bCameraStreamDebugOn[3];
    bool isCapturing;

} CAM_DEVICE_T;


#ifdef __cplusplus
}
#endif
