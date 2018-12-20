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
 -- ----------------------------------------------------------------------------*/
#include "service_types.h"

#include <iostream>
#include <map>

const std::string error_outof_range = "error code is out of range";

std::map<DEVICE_RETURN_CODE, std::string> g_error_string = {
    {DEVICE_ERROR_CAN_NOT_CLOSE, "Can not close"},
    {DEVICE_ERROR_CAN_NOT_OPEN, "Can not open"},
    {DEVICE_ERROR_CAN_NOT_SET, "Can not set"},
    {DEVICE_ERROR_CAN_NOT_START, "Can not start"},
    {DEVICE_ERROR_CAN_NOT_STOP, "Can not stop"},
    {DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED, "Camera device is already closed"},
    {DEVICE_ERROR_DEVICE_IS_ALREADY_OPENED, "Camera device is already opened"},
    {DEVICE_ERROR_DEVICE_IS_ALREADY_STARTED, "Camera device is already started"},
    {DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED, "Camera device is already stopped"},
    {DEVICE_ERROR_DEVICE_IS_BUSY, "Camera device is busy"},
    {DEVICE_ERROR_DEVICE_IS_NOT_OPENED, "Camera device is not opened"},
    {DEVICE_ERROR_DEVICE_IS_NOT_STARTED, "Camera device is not started"},
    {DEVICE_ERROR_NODEVICE, "There is no device"},
    {DEVICE_ERROR_SESSION_ERROR, "Sesssion error"},
    {DEVICE_ERROR_SESSION_NOT_OWNER, "Not Owner"},
    {DEVICE_ERROR_SESSION_INVALID, "Session is invalid"},
    {DEVICE_ERROR_SESSION_UNAUTHORIZED, "Session unauthirized"},
    {DEVICE_ERROR_NO_RESPONSE, "Device is no response"},
    {DEVICE_ERROR_JSON_PARSING, "Parsing error"},
    {DEVICE_ERROR_OUT_OF_MEMORY, "Out of memory"},
    {DEVICE_ERROR_OUT_OF_PARAM_RANGE, "Out of param range"},
    {DEVICE_ERROR_PARAM_IS_MISSING, "Param is missing"},
    {DEVICE_ERROR_SERVICE_IS_NOT_READY, "Service is not ready"},
    {DEVICE_ERROR_SOMETHING_IS_NOT_SET, "Some property is not set"},
    {DEVICE_ERROR_TIMEOUT, "Request timeout"},
    {DEVICE_ERROR_TOO_MANY_REQUEST, "Too many request"},
    {DEVICE_ERROR_UNKNOWN_SERVICE, "Unknown service"},
    {DEVICE_ERROR_UNSUPPORTED_DEVICE, "Unsupported device"},
    {DEVICE_ERROR_UNSUPPORTED_FORMAT, "Unsupported format"},
    {DEVICE_ERROR_UNSUPPORTED_SAMPLINGRATE, "Unsupported samplingrate"},
    {DEVICE_ERROR_UNSUPPORTED_VIDEO_SIZE, "Unsupported video size"},
    {DEVICE_ERROR_UNKNOWN_SERVICE, "Unknown service"},
    {DEVICE_ERROR_WRONG_DEVICE_NUMBER, "Wrong device number"},
    {DEVICE_ERROR_WRONG_ID, "Session id error"},
    {DEVICE_ERROR_WRONG_PARAM, "Wrong param"},
    {DEVICE_ERROR_WRONG_TYPE, "Wrong type"},
    {DEVICE_ERROR_ALREADY_ACQUIRED, "Already acquired"},
    {DEVICE_ERROR_UNKNOWN, "Unknown error"},
    {DEVICE_ERROR_FAIL_TO_OPEN_FILE, "Fail to open file"},
    {DEVICE_ERROR_FAIL_TO_REMOVE_FILE, "Fail to remove file"},
    {DEVICE_ERROR_FAIL_TO_CREATE_DIR, "Fail to create directory"},
    {DEVICE_ERROR_LACK_OF_STORAGE, "Lack of storage"},
    {DEVICE_ERROR_ALREADY_EXISTS_FILE, "Already exists file"},
    {DEVICE_OK, "No error"}};

extern PmLogContext getCameraLunaPmLogContext()
{
  static PmLogContext usLogContext = 0;
  if (0 == usLogContext)
  {
    PmLogGetContext("camera", &usLogContext);
  }
  return usLogContext;
}

std::string getErrorString(DEVICE_RETURN_CODE error_code)
{
  std::string retstring;
  std::map<DEVICE_RETURN_CODE, std::string>::iterator it;

  it = g_error_string.find(error_code);
  if (it != g_error_string.end())
    retstring = it->second;
  else
    retstring = error_outof_range;

  return retstring;
}

void convertFormatToCode(std::string format, CAMERA_DATA_FORMAT *pformatcode)
{
  *pformatcode = CAMERA_FORMAT_UNDEFINED;

  if (format == yuv_format)
    *pformatcode = CAMERA_FORMAT_YUV;
  else if (format == h264es_format)
    *pformatcode = CAMERA_FORMAT_H264ES;
  else if (format == jpeg_format)
    *pformatcode = CAMERA_FORMAT_JPEG;
  else
    *pformatcode = CAMERA_FORMAT_UNDEFINED;
}

std::string getTypeString(DEVICE_TYPE_T etype)
{
  std::string retstring;

  switch (etype)
  {
  case DEVICE_MICROPHONE:
    retstring = "microphone";
    break;
  case DEVICE_CAMERA:
    retstring = "camera";
    break;
  default:
    retstring = "type is out of range";
    break;
  }

  return retstring;
}
