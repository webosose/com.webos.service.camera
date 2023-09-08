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

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "camera_types.h"
#include <map>
#include <string.h>
#include <fstream>

const std::string error_outof_range = "error code is out of range";

std::map<DEVICE_RETURN_CODE_T, std::string> g_error_string = {
    {DEVICE_ERROR_CAN_NOT_CLOSE, "Can not close"},
    {DEVICE_ERROR_CAN_NOT_OPEN, "Can not open"},
    {DEVICE_ERROR_CAN_NOT_SET, "Can not set"},
    {DEVICE_ERROR_CAN_NOT_START, "Can not start"},
    {DEVICE_ERROR_CAN_NOT_STOP, "Can not stop"},
    {DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED, "Camera device is already closed"},
    {DEVICE_ERROR_DEVICE_IS_ALREADY_OPENED, "Camera device is already opened"},
    {DEVICE_ERROR_DEVICE_IS_ALREADY_STARTED, "Camera device is already started"},
    {DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED, "Camera device is already stopped"},
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
    {DEVICE_ERROR_UNKNOWN_SERVICE, "Unknown service"},
    {DEVICE_ERROR_UNSUPPORTED_DEVICE, "Unsupported device"},
    {DEVICE_ERROR_UNSUPPORTED_FORMAT, "Unsupported format"},
    {DEVICE_ERROR_UNSUPPORTED_SAMPLINGRATE, "Unsupported samplingrate"},
    {DEVICE_ERROR_UNSUPPORTED_VIDEO_SIZE, "Unsupported video size"},
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
    {DEVICE_ERROR_ALREADY_OEPENED_PRIMARY_DEVICE, "Already another device opened as primary"},
    {DEVICE_ERROR_CANNOT_WRITE, "Cannot write at specified location"},
    {DEVICE_OK, "No error"},
    {DEVICE_ERROR_UNSUPPORTED_MEMORYTYPE, "Unsupported Memory Type"},
    {DEVICE_ERROR_HANDLE_NOT_EXIST, "Wrong handle"},
    {DEVICE_ERROR_PREVIEW_NOT_STARTED, "Preview not started"},
    {DEVICE_ERROR_NOT_POSIXSHM, "Handle is not in POSIXSHM mode"},
    {DEVICE_ERROR_APP_PERMISSION, "app permission fail"},
    {DEVICE_ERROR_FAIL_TO_REGISTER_SIGNAL, "Failed to register pid with specified signal"},
    {DEVICE_ERROR_CLIENT_PID_IS_MISSING, "Must specify client pid"},
    {DEVICE_ERROR_SUBSCIRPTION_FAIL_DEVICE_DISCONNETED, "Subscribed camera has been disconnected"},
    {DEVICE_ERROR_DEVICE_IS_BUSY, "Camera device is busy"},
    {DEVICE_ERROR_SERVICE_IS_NOT_READY, "Service is not ready"},
    {DEVICE_ERROR_SOMETHING_IS_NOT_SET, "Some property is not set"},
    {DEVICE_ERROR_TOO_MANY_REQUEST, "Too many request"},
    {DEVICE_ERROR_TIMEOUT, "Request timeout"},
    {DEVICE_ERROR_INVALID_WINDOW_ID, "Invalid windowId"}
};

std::map<EventType, std::string> g_event_string = {
    {EventType::EVENT_TYPE_FORMAT, cstr_format},
    {EventType::EVENT_TYPE_PROPERTIES, cstr_properties},
    {EventType::EVENT_TYPE_CONNECT, cstr_connect},
    {EventType::EVENT_TYPE_DISCONNECT, cstr_disconnect},
    {EventType::EVENT_TYPE_DEVICE_FAULT, cstr_devicefault}};

std::map<camera_format_t, std::string> g_format_string = {
    {CAMERA_FORMAT_UNDEFINED, "Unsupported format"},
    {CAMERA_FORMAT_YUV, "YUV"},
    {CAMERA_FORMAT_H264ES, "H264ES"},
    {CAMERA_FORMAT_JPEG, "JPEG"},
    {CAMERA_FORMAT_NV12, "NV12"},};

std::map<int, std::string> g_param_string = {
    {properties_t::PROPERTY_BRIGHTNESS, CONST_PARAM_NAME_BIRGHTNESS},
    {properties_t::PROPERTY_CONTRAST, CONST_PARAM_NAME_CONTRAST},
    {properties_t::PROPERTY_SATURATION, CONST_PARAM_NAME_SATURATION},
    {properties_t::PROPERTY_HUE, CONST_PARAM_NAME_HUE},
    {properties_t::PROPERTY_AUTOWHITEBALANCE, CONST_PARAM_NAME_AUTOWHITEBALANCE},
    {properties_t::PROPERTY_GAMMA, CONST_PARAM_NAME_GAMMA},
    {properties_t::PROPERTY_GAIN, CONST_PARAM_NAME_GAIN},
    {properties_t::PROPERTY_FREQUENCY, CONST_PARAM_NAME_FREQUENCY},
    {properties_t::PROPERTY_SHARPNESS, CONST_PARAM_NAME_SHARPNESS},
    {properties_t::PROPERTY_BACKLIGHTCOMPENSATION, CONST_PARAM_NAME_BACKLIGHT_COMPENSATION},
    {properties_t::PROPERTY_AUTOEXPOSURE, CONST_PARAM_NAME_AUTOEXPOSURE},
    {properties_t::PROPERTY_PAN, CONST_PARAM_NAME_PAN},
    {properties_t::PROPERTY_TILT, CONST_PARAM_NAME_TILT},
    {properties_t::PROPERTY_AUTOFOCUS, CONST_PARAM_NAME_AUTOFOCUS},
    {properties_t::PROPERTY_ZOOMABSOLUTE, CONST_PARAM_NAME_ZOOM_ABSOLUTE},
    {properties_t::PROPERTY_WHITEBALANCETEMPERATURE, CONST_PARAM_NAME_WHITEBALANCETEMPERATURE},
    {properties_t::PROPERTY_EXPOSURE, CONST_PARAM_NAME_EXPOSURE},
    {properties_t::PROPERTY_FOCUSABSOLUTE, CONST_PARAM_NAME_FOCUS_ABSOLUTE},
};

int getRandomNumber()
{
    static unsigned int random_value = 0;
    std::ifstream urandom("/dev/urandom", std::ios::in|std::ios::binary);
    if(urandom)
    {
        urandom.read(reinterpret_cast<char*>(&random_value),sizeof(random_value));

        if(urandom)
        {
            random_value = random_value % 10000;
        }
        urandom.close();
    }
    else
    {
        random_value++;
    }
    return random_value;
}
void getFormatString(int nFormat, char *pFormats)
{
  int i;

  memset(pFormats, 0, 100);

  for (i = 1; i < 8; i++)
  {
    switch (CHECK_BIT_POS(nFormat, i))
    {
    case CAMERA_FORMAT_YUV:
        strncat(pFormats, "YUV|", strlen("YUV|") + 1);
        break;
    case CAMERA_FORMAT_H264ES:
        strncat(pFormats, "H264ES|", strlen("H264ES|") + 1);
        break;
    case CAMERA_FORMAT_JPEG:
        strncat(pFormats, "JPEG|", strlen("JPEG|") + 1);
        break;
    case CAMERA_FORMAT_NV12:
        strncat(pFormats, "NV12|", strlen("NV12|") + 1);
        break;
    default:
        break;
    }
  }

  if (!strstr(pFormats, "|"))
    strncat(pFormats, "Format is out of range", 100);

  return;
}

char *getTypeString(device_t etype)
{
  char *pszRetString = nullptr;

  switch (etype)
  {
  case DEVICE_MICROPHONE:
    pszRetString = (char*)"microphone";
    break;
  case DEVICE_CAMERA:
    pszRetString = (char*)"camera";
    break;
  default:
    pszRetString = (char*)"type is out of range";
    break;
  }

  return pszRetString;
}

std::string getErrorString(DEVICE_RETURN_CODE_T error_code)
{
  std::string retstring;
  std::map<DEVICE_RETURN_CODE_T, std::string>::iterator it;

  it = g_error_string.find(error_code);
  if (it != g_error_string.end())
    retstring = it->second;
  else
    retstring = error_outof_range;

  return retstring;
}

void convertFormatToCode(std::string format, camera_format_t *pformatcode)
{
  *pformatcode = CAMERA_FORMAT_UNDEFINED;

  if (format == cstr_yuvformat)
    *pformatcode = CAMERA_FORMAT_YUV;
  else if (format == cstr_h264esformat)
    *pformatcode = CAMERA_FORMAT_H264ES;
  else if (format == cstr_jpegformat)
    *pformatcode = CAMERA_FORMAT_JPEG;
  else if (format == cstr_nv12format)
    *pformatcode = CAMERA_FORMAT_NV12;
  else
    *pformatcode = CAMERA_FORMAT_UNDEFINED;
}

std::string getEventNotificationString(EventType etype)
{
  std::string retstring;
  std::map<EventType, std::string>::iterator it;

  it = g_event_string.find(etype);
  if (it != g_event_string.end())
    retstring = it->second;
  else
    retstring = cstr_empty;

  return retstring;
}

std::string getFormatStringFromCode(camera_format_t format)
{
  std::string retstring;
  std::map<camera_format_t, std::string>::iterator it;

  it = g_format_string.find(format);
  if (it != g_format_string.end())
    retstring = it->second;
  else
    retstring = cstr_empty;

  return retstring;
}

std::string getResolutionString(camera_format_t eformat)
{
  std::string str_resolution;
  switch (eformat)
  {
  case CAMERA_FORMAT_YUV:
    str_resolution = cstr_yuvformat;
    break;
  case CAMERA_FORMAT_JPEG:
    str_resolution = cstr_jpegformat;
    break;
  case CAMERA_FORMAT_H264ES:
    str_resolution = cstr_h264esformat;
    break;
  case CAMERA_FORMAT_NV12:
    str_resolution = cstr_nv12format;
    break;
  default:
    break;
  }
  return str_resolution;
}

std::string getParamString(int properties_enum)
{
    std::string retstring;
    std::map<int, std::string>::iterator it;

    it = g_param_string.find(properties_enum);
    if (it != g_param_string.end())
        retstring = it->second;
    else
        retstring = "";

    return retstring;
}

int getParamNumFromString(std::string str)
{
    int ret = -1;

    for (auto it = g_param_string.begin(); it != g_param_string.end(); ++it)
    {
        if (it->second == str)
            return it->first;
    }

    return ret;
}

bool CAMERA_PROPERTIES_T::operator != (const CAMERA_PROPERTIES_T &new_property)
{
  for (int i = 0; i < PROPERTY_END; i++)
  {
    if (this->stGetData.data[i][QUERY_VALUE] != new_property.stGetData.data[i][QUERY_VALUE])
    {
      return true;
    }
  }
  return false;
}

bool CAMERA_FORMAT::operator != (const CAMERA_FORMAT &new_format)
{
  if ((this->eFormat != new_format.eFormat) || (this->nFps != new_format.nFps) ||
      (this->nHeight != new_format.nHeight) || (this->nWidth != new_format.nWidth))
         return true;
  else
    return false;
}
