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
#ifndef CAMERA_TYPES_H_
#define CAMERA_TYPES_H_

#include "PmLogLib.h"
#include "constants.h"
#include "luna-service2/lunaservice.h"

#define PMLOG_ERROR(module, args...) PmLogMsg(getCameraLunaPmLogContext(), Error, module, 0, ##args)
#define PMLOG_INFO(module, args...) PmLogMsg(getCameraLunaPmLogContext(), Info, module, 0, ##args)
#define PMLOG_DEBUG(args...) PmLogMsg(getCameraLunaPmLogContext(), Debug, NULL, 0, ##args)

#define CHECK_BIT_POS(x, p) ((x) & (0x01 << (p - 1)))
#define MAX_DEVICE_COUNT 10

/*-----------------------------------------------------------------------------
 (Type Definitions)
 ------------------------------------------------------------------------------*/
typedef enum
{
  DEVICE_DEVICE_UNDEFINED = -1,
  DEVICE_CAMERA = 1,
  DEVICE_MICROPHONE,
  DEVICE_OTHER
} DEVICE_TYPE_T;

typedef enum
{
  DEVICE_RETURN_UNDEFINED = -1,
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
  DEVICE_ERROR_ALREADY_OEPENED_PRIMARY_DEVICE
} DEVICE_RETURN_CODE_T;

typedef enum
{
  DEVICE_EVENT_NONE,
  DEVICE_EVENT_STATE_PLUGGED = 1,
  DEVICE_EVENT_STATE_UNPLUGGED,
  DEVICE_EVENT_STATE_UNSUPPORTED_PLUGGED,
  DEVICE_EVENT_STATE_UNSUPPORTED_UNPLUGGED,
  DEVICE_EVENT_STATE_DUPLICATED_PLUGGED,
  DEVICE_EVENT_STATE_DUPLICATED_UNPLUGGED,
  DEVICE_EVENT_STATE_BUILT_IN_UP,
  DEVICE_EVENT_STATE_BUILT_IN_DOWN,
  DEVICE_EVENT_STATE_UPDATING,
  DEVICE_EVENT_STATE_UPDATE_FINISHED,
  DEVICE_EVENT_STATE_UPDATE_FAIL
} DEVICE_EVENT_STATE_T;

typedef enum
{
  CAMERA_TYPE_UNDEFINED = -1,
  CAMERA_TYPE_V4L2 = 1,
  CAMERA_TYPE_OMX = 2,
} CAMERA_TYPE_T;

typedef enum
{
  CAMERA_FORMAT_UNDEFINED = -1,
  CAMERA_FORMAT_YUV = 1,
  CAMERA_FORMAT_H264ES = 2,
  CAMERA_FORMAT_JPEG = 4,
} CAMERA_FORMAT_T;

enum class NotifierClient
{
  NOTIFIER_CLIENT_PDM = 0,
  NOTIFIER_CLIENT_UDEV
};

enum class EventType
{
  EVENT_TYPE_NONE = -1,
  EVENT_TYPE_FORMAT = 0,
  EVENT_TYPE_PROPERTIES,
  EVENT_TYPE_CONNECT,
  EVENT_TYPE_DISCONNECT
};

/*Structures*/
typedef struct
{
  int nWidth;
  int nHeight;
  CAMERA_FORMAT_T eFormat;
} CAMERA_FORMAT;

typedef struct
{
  char strName[CONST_MAX_STRING_LENGTH];
  DEVICE_TYPE_T nType;
  int bBuiltin;
  int nMaxVideoWidth;
  int nMaxVideoHeight;
  int nMaxPictureWidth;
  int nMaxPictureHeight;
  int nFormat;
  int nSamplingRate;
  int nCodec;
} CAMERA_INFO_T;

typedef struct
{
  int nZoom;
  int nGridZoomX;
  int nGridZoomY;
  int nPan;
  int nTilt;
  int nContrast;
  int nBrightness;
  int nSaturation;
  int nSharpness;
  int nHue;
  int nWhiteBalanceTemperature;
  int nGain;
  int nGamma;
  int nFrequency;
  int bMirror;
  int nExposure;
  int bAutoExposure;
  int bAutoWhiteBalance;
  int nBitrate;
  int nFramerate;
  int ngopLength;
  int bLed;
  int bYuvMode;
  int nIllumination;
  int bBacklightCompensation;

  int nMicMaxGain;
  int nMicMinGain;
  int nMicGain;
  int bMicMute;
} CAMERA_PROPERTIES_T;

typedef struct
{
  int nDeviceNum;
  int nPortNum;
  char strVendorName[CONST_MAX_STRING_LENGTH];
  char strProductName[CONST_MAX_STRING_LENGTH];
  char strSerialNumber[CONST_MAX_STRING_LENGTH];
  char strDeviceType[CONST_MAX_STRING_LENGTH];
  char strDeviceSubtype[CONST_MAX_STRING_LENGTH];
  int isPowerOnConnect;
  char strDeviceNode[CONST_MAX_STRING_LENGTH];
} DEVICE_LIST_T;

typedef struct
{
  std::string str_memorytype;
  std::string str_memorysource;
} camera_memory_source_t;

typedef struct
{
  EventType event;
  void *pdata;
} event_notification_t;

PmLogContext getCameraLunaPmLogContext();
void getFormatString(int, char *);
char *getTypeString(DEVICE_TYPE_T);
std::string getErrorString(DEVICE_RETURN_CODE_T);
void convertFormatToCode(std::string, CAMERA_FORMAT_T *);
std::string getEventNotificationString(EventType);
std::string getFormatStringFromCode(CAMERA_FORMAT_T);

#endif /* CAMERA_TYPES_H_ */
