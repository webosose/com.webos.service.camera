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

#ifndef SERVICE_DEVICE_CONTROLLER_H_
#define SERVICE_DEVICE_CONTROLLER_H_

/*-----------------------------------------------------------------------------
    (File Inclusions)
------------------------------------------------------------------------------*/
#include "camera_hal_types.h"
#include "camera_types.h"
#include "camshm.h"
#include "constants.h"
#include "service_types.h"

#include <pthread.h>
#include <string>

class DeviceControl
{
private:
  void writeImageToFile(const void *, int);
  DEVICE_RETURN_CODE_T checkFormat(void *, FORMAT);
  int pollForCapturedImage(void *, int);
  camera_pixel_format_t getPixelFormat(CAMERA_FORMAT_T);
  void captureThread();
  void previewThread();
  static void *runCaptureImageThread(void *);
  static void *runPreviewThread(void *);

  bool b_iscontinuous_capture_;
  bool b_isstreamon_;
  void *cam_handle_;
  FORMAT informat_;
  camera_pixel_format_t epixelformat_;
  pthread_t tid_capture_;
  pthread_t tid_preview_;
  std::string strdevicenode_;
  SHMEM_HANDLE h_shm_;

public:
  DeviceControl();
  static DeviceControl &getInstance()
  {
    static DeviceControl obj;
    return obj;
  }

  DEVICE_RETURN_CODE_T open(void *, std::string);
  DEVICE_RETURN_CODE_T close(void *);
  DEVICE_RETURN_CODE_T startPreview(void *, int *);
  DEVICE_RETURN_CODE_T stopPreview(void *);
  DEVICE_RETURN_CODE_T startCapture(void *, FORMAT);
  DEVICE_RETURN_CODE_T stopCapture(void *);
  DEVICE_RETURN_CODE_T captureImage(void *, int, FORMAT);
  DEVICE_RETURN_CODE_T createHandle(void **, std::string);
  DEVICE_RETURN_CODE_T destroyHandle(void *);
  DEVICE_RETURN_CODE_T getDeviceInfo(std::string, CAMERA_INFO_T *);
  DEVICE_RETURN_CODE_T getDeviceList(DEVICE_LIST_T *, int *, int *, int *, int *, int);
  DEVICE_RETURN_CODE_T getDeviceProperty(void *, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setDeviceProperty(void *, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setFormat(void *, FORMAT);
};

#endif /*SERVICE_DEVICE_CONTROLLER_H_*/
