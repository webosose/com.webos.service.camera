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

#ifndef CAMERA_TYPES_H_
#define CAMERA_TYPES_H_

#include "PmLogLib.h"
#include "camera_constants.h"
#include "camera_device_types.h"
#include "camera_event_types.h"
#include "camera_hal_if_cpp_types.h"
#include "camera_hal_if_types.h"
#include "camera_log.h"
#include "luna-service2/lunaservice.h"

#define CHECK_BIT_POS(x, p) ((x) & (0x01 << (p - 1)))
#define MAX_DEVICE_COUNT 10

const std::string kMemtypeShmemMmap = "sharedmemory_mmap";
const std::string kMemtypeShmem     = "sharedmemory";
const std::string kMemtypePosixshm  = "posixshm";

/*-----------------------------------------------------------------------------
 (Type Definitions)
 ----------------------------------------------------------------------------*/
typedef enum
{
    DEVICE_RETURN_UNDEFINED = -1,
    DEVICE_OK               = 0,
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
    DEVICE_ERROR_WRONG_TYPE,
    DEVICE_ERROR_ALREADY_ACQUIRED,
    DEVICE_ERROR_UNKNOWN,
    // add webos2.0
    DEVICE_ERROR_FAIL_TO_SPECIALEFFECT,
    DEVICE_ERROR_FAIL_TO_PHOTOVIEWEFFECT,
    DEVICE_ERROR_FAIL_TO_OPEN_FILE,
    DEVICE_ERROR_FAIL_TO_WRITE_FILE,
    DEVICE_ERROR_FAIL_TO_REMOVE_FILE = 40,
    DEVICE_ERROR_FAIL_TO_CREATE_DIR,
    DEVICE_ERROR_LACK_OF_STORAGE,
    DEVICE_ERROR_ALREADY_EXISTS_FILE,
    DEVICE_ERROR_ALREADY_OEPENED_PRIMARY_DEVICE,
    DEVICE_ERROR_CANNOT_WRITE,
    DEVICE_ERROR_UNSUPPORTED_MEMORYTYPE,
    DEVICE_ERROR_HANDLE_NOT_EXIST,
    DEVICE_ERROR_PREVIEW_NOT_STARTED,
    DEVICE_ERROR_NOT_POSIXSHM,
    DEVICE_ERROR_APP_PERMISSION,
    DEVICE_ERROR_FAIL_TO_REGISTER_SIGNAL,
    DEVICE_ERROR_CLIENT_PID_IS_MISSING
} DEVICE_RETURN_CODE_T;

typedef enum
{
    CAMERA_TYPE_UNDEFINED = -1,
    CAMERA_TYPE_V4L2      = 1,
    CAMERA_TYPE_OMX       = 2,
} CAMERA_TYPE_T;

typedef enum
{
    SHMEME_UNKNOWN = -1,
    SHMEM_SYSTEMV  = 0,
    SHMEM_POSIX
} SHMEM_TYPE_T;

enum class NotifierClient
{
    NOTIFIER_CLIENT_PDM = 0,
    NOTIFIER_CLIENT_UDEV,
    NOTIFIER_CLIENT_APPCAST,
};

enum class EventType
{
    EVENT_TYPE_NONE   = -1,
    EVENT_TYPE_FORMAT = 0,
    EVENT_TYPE_PROPERTIES,
    EVENT_TYPE_CONNECT,
    EVENT_TYPE_DISCONNECT,
    EVENT_TYPE_PREVIEW_FAULT,
};

enum class CameraDeviceState
{
    CAM_DEVICE_STATE_UNKNOWN = 0,
    CAM_DEVICE_STATE_OPEN,
    CAM_DEVICE_STATE_PREVIEW
};

/*Structures*/
struct CAMERA_FORMAT
{
    unsigned int nWidth;
    unsigned int nHeight;
    int nFps;
    camera_format_t eFormat;
    bool operator!=(const CAMERA_FORMAT &);
};

struct CAMERA_PROPERTIES_T
{
    camera_queryctrl_t stGetData;
    bool operator!=(const CAMERA_PROPERTIES_T &);
    CAMERA_PROPERTIES_T() : stGetData()
    {
        for (int i = 0; i < PROPERTY_END; i++)
        {
            stGetData.data[i][QUERY_VALUE] = CONST_PARAM_DEFAULT_VALUE;
        }
    }
};

typedef struct
{
    std::string str_memorytype;
    std::string str_memorysource;
} camera_memory_source_t;

void getFormatString(int, char *);
char *getTypeString(device_t);
int getRandomNumber();
std::string getErrorString(DEVICE_RETURN_CODE_T);
void convertFormatToCode(std::string, camera_format_t *);
std::string getEventNotificationString(EventType);
std::string getFormatStringFromCode(camera_format_t);
std::string getResolutionString(camera_format_t);
std::string getParamString(int properties_enum);
int getParamNumFromString(std::string str);
std::string getQueryString(int query_enum);
int getQueryNumFromString(std::string str);

#endif /* CAMERA_TYPES_H_ */
