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

#ifndef CAMERA_HAL_TYPES
#define CAMERA_HAL_TYPES

#include "camera_hal_types_common.h"
#include <string>
#include <vector>

#define CONST_MAX_INDEX 300
#define CONST_MAX_FORMAT 5
#define CONST_MAX_STRING_LENGTH 256
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
    QUERY_VALUE,
    QUERY_END
} query_ctrl_t;

struct camera_queryctrl_t
{
    int data[PROPERTY_END][QUERY_END];
};

struct camera_properties_t
{
    camera_queryctrl_t stGetData;
};

struct camera_resolution_t
{
    std::vector<std::string> c_res;
    camera_format_t e_format;

    camera_resolution_t(std::vector<std::string> res, camera_format_t format)
        : c_res(move(res)), e_format(format)
    {
    }
};

struct camera_device_info_t
{
    std::string str_devicename;
    std::string str_vendorid;
    std::string str_productid;
    device_t n_devicetype{DEVICE_TYPE_UNDEFINED};
    int b_builtin{0};
    std::vector<camera_resolution_t> stResolution;
};

#endif
