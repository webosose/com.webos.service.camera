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
#ifndef SRC_INCLUDE_HAL_H_
#define SRC_INCLUDE_HAL_H_


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "camera_types.h"

typedef enum MIC_PROPERTIES_INDEX
{
    MIC_PROPERTIES_MAXGAIN, MIC_PROPERTIES_MINGAIN, MIC_PROPERTIES_GAIN, MIC_PROPERTIES_MUTE,
} MIC_PROPERTIES_INDEX_T;
typedef enum CAMERA_PROPERTIES_INDEX
{
    CAMERA_PROPERTIES_ZOOM,
    CAMERA_PROPERTIES_GRIDZOOM,
    CAMERA_PROPERTIES_GRIDZOOMX,
    CAMERA_PROPERTIES_GRIDZOOMY,
    CAMERA_PROPERTIES_PAN,
    CAMERA_PROPERTIES_TILT,
    CAMERA_PROPERTIES_CONTRAST,
    CAMERA_PROPERTIES_BRIGHTNESS,
    CAMERA_PROPERTIES_SATURATION,
    CAMERA_PROPERTIES_SHARPNESS,
    CAMERA_PROPERTIES_HUE,
    CAMERA_PROPERTIES_WHITEBALANCETEMPERATURE,
    CAMERA_PROPERTIES_GAIN,
    CAMERA_PROPERTIES_GAMMA,
    CAMERA_PROPERTIES_FREQUENCY,
    CAMERA_PROPERTIES_MIRROR,
    CAMERA_PROPERTIES_EXPOSURE,
    CAMERA_PROPERTIES_BACKLIGHT_COMPENSATION,
    CAMERA_PROPERTIES_AUTOEXPOSURE,
    CAMERA_PROPERTIES_AUTOWHITEBALANCE,
    CAMERA_PROPERTIES_BITRATE,
    CAMERA_PROPERTIES_FRAMERATE,
    CAMERA_PROPERTIES_GOPLENGTH,

    CAMERA_PROPERTIES_LED,
    //  CAMERA_PROPERTIES_TSHEADER,
    CAMERA_PROPERTIES_FIXEDFPS,
    CAMERA_PROPERTIES_CROP,

    CAMERA_PROPERTIES_YUVMODE,
    CAMERA_PROPERTIES_UNKNOWN
} CAMERA_PROPERTIES_INDEX_T;
typedef enum _MICROPHONE_CHANNEL
{
    MICROPHONE_CHANNE_UNDEFINED = -1,

    MICROPHONE_CHANNE_MONO = 1, MICROPHONE_CHANNE_STEREO = 2
} MICROPHONE_CHANNEL_T;

typedef enum _MICROPHONE_SAMPLINGRATE
{
    MICROPHONE_SAMPLINGRATE_UNDEFINED = -1,

    MICROPHONE_SAMPLINGRATE_NARRO_WBAND = 1,
    MICROPHONE_SAMPLINGRATE_WIDE_BAND = 2,
    MICROPHONE_SAMPLINGRATE_SUPER_WIDE_BAND = 4
} MICROPHONE_SAMPLINGRATE_T;

typedef enum _MICROPHONE_FORMAT
{
    MICROPHONE_FORMAT_UNDEFINED = -1,

    MICROPHONE_FORMAT_PCM = 0, MICROPHONE_FORMAT_AAC
} MICROPHONE_FORMAT_T;

typedef struct MIC_INFO
{
    char strName[256];
    int bBuiltin;
    MICROPHONE_FORMAT_T nFormat;
    MICROPHONE_CHANNEL_T nChannel;
    MICROPHONE_SAMPLINGRATE_T nSamplingRate;
    int nCodec;
} MIC_INFO_T;

typedef enum CAMERA_STREAM_TYPE
{
    ST_EVENT_VIDEO = 0x1,
    ST_EVENT_AUDIO = 0x2,
    ST_EVENT_UNPLUG = 0X3,
    ST_PCM = 0x0A,
    ST_AACOLD = 0x3C,
    ST_AAC = 0x27,
    ST_YUY2 = 0x29,         // YUV
    ST_NV12 = 0x2A,
    ST_H264 = 0x2B,         // H264 ES
    ST_MJPG = 0x2C,         // Jpeg
    ST_GRAY = 0x2D,
    ST_H264TS = 0x2E,       // H264 TS
    ST_SECS = 0xEE,         // SECS
} CAMERA_STREAM_TYPE_T;

/*HAL camera API*/
DEVICE_RETURN_CODE_T hal_cam_create_handle(DEVICE_LIST_T sDeviceInfo, DEVICE_HANDLE *pDeviceHandle);
DEVICE_RETURN_CODE_T hal_cam_open(DEVICE_HANDLE sDeviceHandle);
DEVICE_RETURN_CODE_T hal_cam_close(DEVICE_HANDLE sDeviceHandle);
DEVICE_RETURN_CODE_T hal_cam_start(DEVICE_HANDLE sDeviceHandle, int *pKey);
DEVICE_RETURN_CODE_T hal_cam_stop(DEVICE_HANDLE sDeviceHandle);
DEVICE_RETURN_CODE_T hal_cam_get_info(DEVICE_LIST_T sDeviceHandle, CAMERA_INFO_T *pInfo);
DEVICE_RETURN_CODE_T hal_cam_set_property(DEVICE_HANDLE sDeviceHandle,
        CAMERA_PROPERTIES_INDEX_T nProperty, int value);
DEVICE_RETURN_CODE_T hal_cam_get_property(DEVICE_HANDLE sDeviceHandle,
        CAMERA_PROPERTIES_INDEX_T nProperty, int *value);
DEVICE_RETURN_CODE_T hal_cam_get_list(int *nCameraCount, int cameraType[]);
DEVICE_RETURN_CODE_T hal_cam_capture_image(DEVICE_HANDLE sDeviceHandle, int nCount,
        CAMERA_FORMAT sFormat);
DEVICE_RETURN_CODE_T hal_cam_get_format(DEVICE_HANDLE sDeviceHandle, CAMERA_FORMAT *sFormat);
DEVICE_RETURN_CODE_T hal_cam_set_format(DEVICE_HANDLE sDeviceHandle, CAMERA_FORMAT sFormat);
DEVICE_RETURN_CODE_T hal_cam_start_capture(DEVICE_HANDLE sDeviceHandle, CAMERA_FORMAT sFormat);
DEVICE_RETURN_CODE_T hal_cam_stop_capture(DEVICE_HANDLE sDeviceHandle);

/*HAL mic API*/
DEVICE_RETURN_CODE_T hal_mic_open(int nMicNum, int nSamplingRate, int nCodec, int *pKey);
DEVICE_RETURN_CODE_T hal_mic_start(int nMicNum, int *pKey);
DEVICE_RETURN_CODE_T hal_mic_stop(int nMicNum);
DEVICE_RETURN_CODE_T hal_mic_close(int nMicNum);
DEVICE_RETURN_CODE_T hal_mic_get_info(int nMicNum, MIC_INFO_T *pInfo);
DEVICE_RETURN_CODE_T hal_mic_get_list(int *pMicCount);
DEVICE_RETURN_CODE_T hal_mic_set_property(int nMicNum, MIC_PROPERTIES_INDEX_T nProperty,
        long value);
DEVICE_RETURN_CODE_T hal_mic_get_property(int nMicNum, MIC_PROPERTIES_INDEX_T nProperty,
        long *value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
