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

#ifndef SRC_HAL_V4L2_HAL_V4L2_H_
#define SRC_HAL_V4L2_HAL_V4L2_H_

//#include "camera_common.h"
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>

#ifdef __cplusplus
extern "C"
{
#endif

// ddi_cam.cpp
typedef void (*pfpDataCB)(int nDevNum, int nStreamType, unsigned int nDataLength,
        unsigned char *pData, unsigned int nTimestamp);
DEVICE_RETURN_CODE_T v4l2_cam_open(char *strDeviceName);
DEVICE_RETURN_CODE_T v4l2_cam_close(char *strDeviceName);
DEVICE_RETURN_CODE_T v4l2_cam_start(char *strDeviceName);
DEVICE_RETURN_CODE_T v4l2_cam_stop(char *strDeviceName);
DEVICE_RETURN_CODE_T v4l2_cam_get_info(char *strDeviceName, CAMERA_INFO_T *pInfo);
DEVICE_RETURN_CODE_T v4l2_cam_get_list(int *cameraNum, int cameraType[]);
void CAMERA_CAM_registerCallback(char *strDeviceName,
        void (*func)(int cameraNum, int nStreamType, unsigned int nDataLen, unsigned char *pData,
                unsigned int nTimeStamp));

void v4l2_cam_registerCallback(char *strDeviceName, pfpDataCB func);
DEVICE_RETURN_CODE_T v4l2_cam_capture_image(char *strDeviceName, int nCount,
        CAMERA_FORMAT sFormat);
DEVICE_RETURN_CODE_T v4l2_cam_set_format(char *strDeviceName, CAMERA_FORMAT sFormat);
DEVICE_RETURN_CODE_T v4l2_cam_get_format(char *strDeviceName, CAMERA_FORMAT*sFormat);
DEVICE_RETURN_CODE_T v4l2_cam_start_capture(char *strDeviceName, CAMERA_FORMAT sFormat);
DEVICE_RETURN_CODE_T v4l2_cam_stop_capture(char *strDeviceName);
DEVICE_RETURN_CODE_T v4l2_cam_set_property(char *strDeviceName, CAMERA_PROPERTIES_INDEX_T nProperty,
        int value);
DEVICE_RETURN_CODE_T v4l2_cam_get_property(char *strDeviceName, CAMERA_PROPERTIES_INDEX_T nProperty,
        int *value);

#ifdef __cplusplus
}
#endif
#endif
