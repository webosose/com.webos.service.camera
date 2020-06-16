// Copyright (c) 2019-2020 LG Electronics, Inc.
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

#include <thread>
#include <string>
#include <condition_variable>

class DeviceControl
{
private:
  DEVICE_RETURN_CODE_T writeImageToFile(const void *, int) const;
  DEVICE_RETURN_CODE_T checkFormat(void *, CAMERA_FORMAT);
  DEVICE_RETURN_CODE_T pollForCapturedImage(void *, int) const;
  static camera_pixel_format_t getPixelFormat(camera_format_t);
  static camera_format_t getCameraFormat(camera_pixel_format_t);
  void captureThread();
  void previewThread();

  bool b_iscontinuous_capture_;
  bool b_isstreamon_;
  bool b_isshmwritedone_;
  void *cam_handle_;
  CAMERA_FORMAT informat_;
  camera_pixel_format_t epixelformat_;
  std::thread tidPreview;
  std::thread tidCapture;
  std::mutex tMutex;
  std::condition_variable tCondVar;
  std::string strdevicenode_;
  SHMEM_HANDLE h_shm_;
  std::string str_imagepath_;
  std::string str_capturemode_;

  static int n_imagecount_;

public:
  DeviceControl();
  DEVICE_RETURN_CODE_T open(void *, std::string);
  DEVICE_RETURN_CODE_T close(void *);
  DEVICE_RETURN_CODE_T startPreview(void *, int *);
  DEVICE_RETURN_CODE_T stopPreview(void *);
  DEVICE_RETURN_CODE_T startCapture(void *, CAMERA_FORMAT, const std::string&);
  DEVICE_RETURN_CODE_T stopCapture(void *);
  DEVICE_RETURN_CODE_T captureImage(void *,int, CAMERA_FORMAT, const std::string&,
                                    const std::string&);
  DEVICE_RETURN_CODE_T createHandle(void **, std::string);
  DEVICE_RETURN_CODE_T destroyHandle(void *);
  static DEVICE_RETURN_CODE_T getDeviceInfo(std::string, camera_device_info_t *);
  static DEVICE_RETURN_CODE_T getDeviceList(DEVICE_LIST_T *, int *, int *, int *, int *, int);
  DEVICE_RETURN_CODE_T getDeviceProperty(void *, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setDeviceProperty(void *, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setFormat(void *, CAMERA_FORMAT);
  DEVICE_RETURN_CODE_T getFormat(void *, CAMERA_FORMAT *);
};

#endif /*SERVICE_DEVICE_CONTROLLER_H_*/
