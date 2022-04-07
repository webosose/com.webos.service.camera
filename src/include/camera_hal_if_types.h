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

#ifndef CAMERA_HAL_IF_TYPES
#define CAMERA_HAL_IF_TYPES

#define CONST_MAX_INDEX 300
#define CONST_MAX_FORMAT 5
#define CONST_MAX_STRING_LENGTH 256
#define CONST_MAX_STRING_RESOLUTION_LENGTH 20
#define CONST_MAX_BUFFER_NUM 6
#define CONST_MAX_PROPERTY_NUM 20

typedef enum
{
    CAMERA_ERROR_UNKNOWN = -1,
    CAMERA_ERROR_NONE    = 0,
    CAMERA_ERROR_INVALID_PARAMETER,
    CAMERA_ERROR_INVALID_STATE,
    CAMERA_ERROR_DEVICE_OPEN,
    CAMERA_ERROR_DEVICE_CLOSE,
    CAMERA_ERROR_DEVICE_NOT_FOUND,
    CAMERA_ERROR_PLUGIN_NOT_FOUND,
    CAMERA_ERROR_CREATE_HANDLE,
    CAMERA_ERROR_DESTROY_HANDLE,
    CAMERA_ERROR_SET_FORMAT,
    CAMERA_ERROR_GET_FORMAT,
    CAMERA_ERROR_SET_BUFFER,
    CAMERA_ERROR_GET_BUFFER,
    CAMERA_ERROR_RELEASE_BUFFER,
    CAMERA_ERROR_DESTROY_BUFFER,
    CAMERA_ERROR_START_CAPTURE,
    CAMERA_ERROR_STOP_CAPTURE,
    CAMERA_ERROR_GET_IMAGE,
    CAMERA_ERROR_SET_PROPERTIES,
    CAMERA_ERROR_GET_PROPERTIES,
    CAMERA_ERROR_GET_INFO,
    CAMERA_ERROR_SET_CALLBACK,
    CAMERA_ERROR_REMOVE_CALLBACK,
    CAMERA_ERROR_NO_DEVICE,
    CAMERA_ERROR_GET_BUFFER_FD
} camera_error_t;

typedef enum
{
    /* YUV */
    CAMERA_PIXEL_FORMAT_NV12 = 0x0000,
    CAMERA_PIXEL_FORMAT_NV21,
    CAMERA_PIXEL_FORMAT_I420,
    CAMERA_PIXEL_FORMAT_YV12,
    CAMERA_PIXEL_FORMAT_YUYV,
    CAMERA_PIXEL_FORMAT_UYVY,

    /* RGB */
    CAMERA_PIXEL_FORMAT_BGRA8888,
    CAMERA_PIXEL_FORMAT_ARGB8888,

    /* ENCODED */
    CAMERA_PIXEL_FORMAT_JPEG,
    CAMERA_PIXEL_FORMAT_H264,

    /* MAX */
    CAMERA_PIXEL_FORMAT_MAX
} camera_pixel_format_t;

typedef enum
{
    DEVICE_TYPE_UNDEFINED = -1,
    DEVICE_TYPE_CAMERA    = 1,
    DEVICE_TYPE_MICROPHONE,
    DEVICE_TYPE_OTHER
} device_t;

typedef enum
{
    CAMERA_FORMAT_UNDEFINED = -1,
    CAMERA_FORMAT_YUV       = 1,
    CAMERA_FORMAT_H264ES    = 2,
    CAMERA_FORMAT_JPEG      = 4,
} camera_format_t;

typedef enum
{
    IOMODE_UNKNOWN = -1,
    IOMODE_READ    = 0,
    IOMODE_MMAP,
    IOMODE_USERPTR,
    IOMODE_DMABUF
} io_mode_t;

/*Structures*/

typedef struct
{
    camera_pixel_format_t pixel_format;
    unsigned int stream_width;
    unsigned int stream_height;
    int stream_fps;
    unsigned int buffer_size;
    const char *userdata;
} stream_format_t;

typedef struct
{
    void *start;
    unsigned long length;
    int index;
    int fd;
} buffer_t;

typedef enum
{
    PROPERTY_BRIGHTNESS = 0,
    PROPERTY_CONTRAST,
    PROPERTY_SATURATION,
    PROPERTY_HUE,
    PROPERTY_AUTOWHITEBALANCE,
    PROPERTY_GAMMA,
    PROPERTY_GAIN,
    PROPERTY_FREQUENCY,
    PROPERTY_SHARPNESS,
    PROPERTY_BACKLIGHTCOMPENSATION,
    PROPERTY_AUTOEXPOSURE,
    PROPERTY_PAN,
    PROPERTY_TILT,
    PROPERTY_AUTOFOCUS,
    PROPERTY_ZOOMABSOLUTE,
    PROPERTY_WHITEBALANCETEMPERATURE,
    PROPERTY_EXPOSURE,
    PROPERTY_FOCUSABSOLUTE,
    PROPERTY_END
} properties_t;

typedef enum
{
    QUERY_MIN = 0,
    QUERY_MAX,
    QUERY_STEP,
    QUERY_DEFAULT,
    QUERY_END
} query_ctrl_t;

typedef struct
{
    int n_width[CONST_MAX_FORMAT][CONST_MAX_INDEX];
    int n_height[CONST_MAX_FORMAT][CONST_MAX_INDEX];
    char c_res[CONST_MAX_FORMAT][CONST_MAX_INDEX][CONST_MAX_STRING_RESOLUTION_LENGTH];
    camera_format_t e_format[CONST_MAX_FORMAT];
    int n_frameindex[CONST_MAX_FORMAT];
    int n_framecount[CONST_MAX_FORMAT];
    int n_formatindex;
} camera_resolution_t;

typedef struct
{
    int data[PROPERTY_END][QUERY_END];
} camera_queryctrl_t;

typedef struct
{
    int nBrightness;
    int nContrast;
    int nSaturation;
    int nHue;
    int nAutoWhiteBalance;
    int nGamma;
    int nGain;
    int nFrequency;
    int nWhiteBalanceTemperature;
    int nSharpness;
    int nBacklightCompensation;
    int nAutoExposure;
    int nExposure;
    int nPan;
    int nTilt;
    int nFocusAbsolute;
    int nAutoFocus;
    int nZoomAbsolute;
    camera_queryctrl_t stGetData;
    camera_resolution_t stResolution;
} camera_properties_t;

typedef struct
{
    char str_devicename[CONST_MAX_STRING_LENGTH];
    char str_vendorid[CONST_MAX_STRING_LENGTH];
    char str_productid[CONST_MAX_STRING_LENGTH];
    device_t n_devicetype;
    int b_builtin;
    int n_cur_fps; // fps currently set in camera(v4l2)
    int n_maxvideowidth;
    int n_maxvideoheight;
    int n_maxpicturewidth;
    int n_maxpictureheight;
    int n_format;
    int n_samplingrate;
    int n_codec;
} camera_device_info_t;

#endif
