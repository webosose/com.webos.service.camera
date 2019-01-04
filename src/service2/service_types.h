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

#ifndef SERVICE_TYPES_H_
#define SERVICE_TYPES_H_

#include "PmLogLib.h"
#include "camera_types.h"
#include "constants.h"
#include <iostream>
#include <stdio.h>
#include <string>

/*constants*/

const std::string invalid_device_id = "-1";
const std::string yuv_format = "YUV";
const std::string h264es_format = "H264ES";
const std::string jpeg_format = "JPEG";
const std::string primary = "primary";
const std::string secondary = "secondary";

const int n_invalid_id = -1;
const int frame_size = 640 * 480 * 2 + 1024;
const int frame_count = 8;

/*defines*/

#define CHECK_BIT_POS(x, p) ((x) & (0x01 << (p - 1)))
#define MAX_DEVICE_COUNT 10

/*Enumerations*/

enum class DeviceError
{
  DEVICE_ERROR_UNDEFINED = -1,
  DEVICE_OK = 0,
  DEVICE_SESSION_OK,
  DEVICE_ERROR_CAN_NOT_CLOSE,
  DEVICE_ERROR_CAN_NOT_OPEN,
  DEVICE_ERROR_CAN_NOT_SET,
  DEVICE_ERROR_CAN_NOT_START,
  DEVICE_ERROR_CAN_NOT_STOP,
  DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED,
  DEVICE_ERROR_DEVICE_IS_ALREADY_OPENED,
  DEVICE_ERROR_DEVICE_IS_ALREADY_STARTED,
  DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED,
  DEVICE_ERROR_DEVICE_IS_BUSY = 10,
  DEVICE_ERROR_DEVICE_IS_NOT_OPENED,
  DEVICE_ERROR_DEVICE_IS_NOT_STARTED,
  DEVICE_ERROR_NODEVICE,
  DEVICE_ERROR_MAX_LIMIT_REACHED,
  // session
  DEVICE_ERROR_SESSION_ERROR,
  DEVICE_ERROR_SESSION_NOT_OWNER,
  DEVICE_ERROR_SESSION_INVALID,
  DEVICE_ERROR_SESSION_UNAUTHORIZED,
  DEVICE_ERROR_NO_RESPONSE,
  DEVICE_ERROR_JSON_PARSING,
  DEVICE_ERROR_OUT_OF_MEMORY,
  DEVICE_ERROR_OUT_OF_PARAM_RANGE,
  DEVICE_ERROR_PARAM_IS_MISSING,
  DEVICE_ERROR_SERVICE_IS_NOT_READY = 20,
  DEVICE_ERROR_SOMETHING_IS_NOT_SET,
  DEVICE_ERROR_TOO_MANY_REQUEST,
  DEVICE_ERROR_TIMEOUT,
  DEVICE_ERROR_UNKNOWN_SERVICE,
  DEVICE_ERROR_UNSUPPORTED_DEVICE,
  DEVICE_ERROR_UNSUPPORTED_FORMAT,
  DEVICE_ERROR_UNSUPPORTED_SAMPLINGRATE,
  DEVICE_ERROR_UNSUPPORTED_VIDEO_SIZE,
  DEVICE_ERROR_WRONG_DEVICE_NUMBER = 30,
  DEVICE_ERROR_WRONG_ID,
  DEVICE_ERROR_WRONG_PARAM,
  DEVICE_ERROR_WRONG_TYPE,       // add 6.20 : type error (camera/mic)
  DEVICE_ERROR_ALREADY_ACQUIRED, // add 12.5 : no acquire allowed from two or more request from same
                                 // process
  DEVICE_ERROR_UNKNOWN,
  // add webos2.0
  DEVICE_ERROR_FAIL_TO_SPECIALEFFECT,
  DEVICE_ERROR_FAIL_TO_PHOTOVIEWEFFECT,
  DEVICE_ERROR_FAIL_TO_OPEN_FILE,
  DEVICE_ERROR_FAIL_TO_WRITE_FILE,
  DEVICE_ERROR_FAIL_TO_REMOVE_FILE = 40,
  DEVICE_ERROR_FAIL_TO_CREATE_DIR,
  DEVICE_ERROR_LACK_OF_STORAGE,
  DEVICE_ERROR_ALREADY_EXISTS_FILE
};

enum class CameraFormat
{
  CAMERA_FORMAT_UNDEFINED = -1,
  CAMERA_FORMAT_YUV = 1,
  CAMERA_FORMAT_H264ES = 2,
  CAMERA_FORMAT_JPEG = 4,
};

enum class DeviceType
{
  DEVICE_DEVICE_UNDEFINED = -1,
  DEVICE_CAMERA = 1,
  DEVICE_MICROPHONE,
  DEVICE_OTHER
};

enum class DeviceEvent
{
  DEVICE_EVENT_NONE = 0,
  DEVICE_EVENT_STATE_PLUGGED = 1,
  DEVICE_EVENT_STATE_UNPLUGGED
};

enum class NotifierClient
{
  NOTIFIER_CLIENT_PDM = 0,
  NOTIFIER_CLIENT_UDEV
};

/*Sturctures*/

typedef struct
{
  std::string str_memorytype;
  std::string str_memorysource;
} camera_memory_source_t;

typedef struct
{
  int n_width;
  int n_height;
  CameraFormat e_format;
} camera_format_t;

typedef struct
{
  std::string str_name;
  DeviceType e_type;
  int b_builtin;
  int n_maxvideowidth;
  int n_maxvideoheight;
  int n_maxpicturewidth;
  int n_maxpictureheight;
  int n_format;
  int n_samplingrate;
  int n_codec;
} camera_info_t;

typedef struct
{
  int device_num;
  int port_num;
  std::string device_node;
  std::string vendor_name;
  std::string product_name;
  std::string serial_number;
  std::string device_type;
  std::string device_subtype;
  bool cam_status;
  DeviceEvent device_state;
  bool power_status;
} device_info_t;

/*Utility functions*/

std::string getErrorString(DEVICE_RETURN_CODE);
void convertFormatToCode(std::string, CAMERA_DATA_FORMAT *);

#endif /* SERVICE_TYPES_H_ */
