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
#include <map>
#include <iostream>

const std::string error_outof_range = "error code is out of range";

std::map<DeviceError, std::string> g_error_string = {
    {DeviceError::DEVICE_ERROR_CAN_NOT_CLOSE, "Can not close"},
    {DeviceError::DEVICE_ERROR_CAN_NOT_OPEN, "Can not open"},
    {DeviceError::DEVICE_ERROR_CAN_NOT_SET, "Can not set"},
    {DeviceError::DEVICE_ERROR_CAN_NOT_START, "Can not start"},
    {DeviceError::DEVICE_ERROR_CAN_NOT_STOP, "Can not stop"},
    {DeviceError::DEVICE_ERROR_DEVICE_IS_ALREADY_CLOSED, "Camera device is already closed"},
    {DeviceError::DEVICE_ERROR_DEVICE_IS_ALREADY_OPENED, "Camera device is already opened"},
    {DeviceError::DEVICE_ERROR_DEVICE_IS_ALREADY_STARTED, "Camera device is already started"},
    {DeviceError::DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED, "Camera device is already stopped"},
    {DeviceError::DEVICE_ERROR_DEVICE_IS_BUSY, "Camera device is busy"},
    {DeviceError::DEVICE_ERROR_DEVICE_IS_NOT_OPENED, "Camera device is not opened"},
    {DeviceError::DEVICE_ERROR_DEVICE_IS_NOT_STARTED, "Camera device is not started"},
    {DeviceError::DEVICE_ERROR_NODEVICE, "There is no device"},
    {DeviceError::DEVICE_ERROR_SESSION_ERROR, "Sesssion error"},
    {DeviceError::DEVICE_ERROR_SESSION_NOT_OWNER, "Not Owner"},
    {DeviceError::DEVICE_ERROR_SESSION_INVALID, "Session is invalid"},
    {DeviceError::DEVICE_ERROR_SESSION_UNAUTHORIZED, "Session unauthirized"},
    {DeviceError::DEVICE_ERROR_NO_RESPONSE, "Device is no response"},
    {DeviceError::DEVICE_ERROR_JSON_PARSING, "Parsing error"},
    {DeviceError::DEVICE_ERROR_OUT_OF_MEMORY, "Out of memory"},
    {DeviceError::DEVICE_ERROR_OUT_OF_PARAM_RANGE, "Out of param range"},
    {DeviceError::DEVICE_ERROR_PARAM_IS_MISSING, "Param is missing"},
    {DeviceError::DEVICE_ERROR_SERVICE_IS_NOT_READY, "Service is not ready"},
    {DeviceError::DEVICE_ERROR_SOMETHING_IS_NOT_SET, "Some property is not set"},
    {DeviceError::DEVICE_ERROR_TIMEOUT, "Request timeout"},
    {DeviceError::DEVICE_ERROR_TOO_MANY_REQUEST, "Too many request"},
    {DeviceError::DEVICE_ERROR_UNKNOWN_SERVICE, "Unknown service"},
    {DeviceError::DEVICE_ERROR_UNSUPPORTED_DEVICE, "Unsupported device"},
    {DeviceError::DEVICE_ERROR_UNSUPPORTED_FORMAT, "Unsupported format"},
    {DeviceError::DEVICE_ERROR_UNSUPPORTED_SAMPLINGRATE, "Unsupported samplingrate"},
    {DeviceError::DEVICE_ERROR_UNSUPPORTED_VIDEO_SIZE, "Unsupported video size"},
    {DeviceError::DEVICE_ERROR_UNKNOWN_SERVICE, "Unknown service"},
    {DeviceError::DEVICE_ERROR_WRONG_DEVICE_NUMBER, "Wrong device number"},
    {DeviceError::DEVICE_ERROR_WRONG_ID, "Session id error"},
    {DeviceError::DEVICE_ERROR_WRONG_PARAM, "Wrong param"},
    {DeviceError::DEVICE_ERROR_WRONG_TYPE, "Wrong type"},
    {DeviceError::DEVICE_ERROR_ALREADY_ACQUIRED, "Already acquired"},
    {DeviceError::DEVICE_ERROR_UNKNOWN, "Unknown error"},
    {DeviceError::DEVICE_ERROR_FAIL_TO_OPEN_FILE, "Fail to open file"},
    {DeviceError::DEVICE_ERROR_FAIL_TO_REMOVE_FILE, "Fail to remove file"},
    {DeviceError::DEVICE_ERROR_FAIL_TO_CREATE_DIR, "Fail to create directory"},
    {DeviceError::DEVICE_ERROR_LACK_OF_STORAGE, "Lack of storage"},
    {DeviceError::DEVICE_ERROR_ALREADY_EXISTS_FILE, "Already exists file"},
    {DeviceError::DEVICE_OK, "No error"}};

extern PmLogContext getCameraLunaPmLogContext()
{
    static PmLogContext usLogContext = 0;
    if (0 == usLogContext)
    {
        PmLogGetContext("camera", &usLogContext);
    }
    return usLogContext;
}

std::string getErrorString(DeviceError error_code)
{
    std::string retstring;
    std::map<DeviceError, std::string>::iterator it;

    it = g_error_string.find(error_code);
    if (it != g_error_string.end())
        retstring = it->second;
    else
        retstring = error_outof_range;

    return retstring;
}

DeviceError convertFormatToCode(std::string format, CameraFormat *pformatcode)
{
    *pformatcode = CameraFormat::CAMERA_FORMAT_UNDEFINED;

    if (format == yuv_format)
        *pformatcode = CameraFormat::CAMERA_FORMAT_YUV;
    else if (format == h264es_format)
        *pformatcode = CameraFormat::CAMERA_FORMAT_H264ES;
    else if (format == jpeg_format)
        *pformatcode = CameraFormat::CAMERA_FORMAT_JPEG;
    else
        return DeviceError::DEVICE_ERROR_WRONG_PARAM;

    return DeviceError::DEVICE_OK;
}

void getFormatString(int nformat, std::string formats)
{
    int i;

    for (i = 1; i < 8; i++)
    {
        switch (CHECK_BIT_POS(nformat, i))
        {
        case (int)CameraFormat::CAMERA_FORMAT_YUV:
            formats += "YUV|";
            break;
        case (int)CameraFormat::CAMERA_FORMAT_H264ES:
            formats += "H264ES|";
            break;
        case (int)CameraFormat::CAMERA_FORMAT_JPEG:
            formats += "JPEG|";
            break;
        default:
            break;
        }
    }

    if (std::string::npos != formats.find("|"))
        formats += "Format is out of range";

    return;
}

std::string getTypeString(DeviceType etype)
{
    std::string retstring;

    switch (etype)
    {
    case DeviceType::DEVICE_MICROPHONE:
        retstring = "microphone";
        break;
    case DeviceType::DEVICE_CAMERA:
        retstring = "camera";
        break;
    default:
        retstring = "type is out of range";
        break;
    }

    return retstring;
}
