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

#ifndef SERVICE_DEVICE_CONTROLLER_H_
#define SERVICE_DEVICE_CONTROLLER_H_

/*-----------------------------------------------------------------------------
    (File Inclusions)
------------------------------------------------------------------------------*/
#include "camera_hal_types.h"
#include "camera_types.h"
#include "cam_posixshm.h"
#include "constants.h"
#include <unistd.h>
#include <thread>
#include <string>
#include <condition_variable>
#include <vector>
#include <solutions/CameraSolutionManager.h>


typedef struct
{
    pid_t pid;
    int sig;
    int handle;
} CLIENT_INFO_T;


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
  bool b_isposixruning;
  bool b_issystemvruning;
  bool b_isshmwritedone_;
  bool b_issyshmwritedone_;
  bool b_isposhmwritedone_;
  void *cam_handle_;
  int shmemfd_;
  CAMERA_FORMAT informat_;
  camera_pixel_format_t epixelformat_;
  std::thread tidPreview;
  std::thread tidCapture;
  std::mutex tMutex;
  std::condition_variable tCondVar;
  std::string strdevicenode_;
  SHMEM_HANDLE h_shmposix_;
  std::string str_imagepath_;
  std::string str_capturemode_;
  std::string str_memtype_;
  std::string str_shmemname_;

  static int n_imagecount_;

  std::vector<CLIENT_INFO_T> client_pool_;
  std::mutex client_pool_mutex_;
  void broadcast_();

  bool cancel_preview_;
  int buf_size_;

  LSHandle *sh_;
  std::string subskey_;
  int camera_id_;
  void notifyDeviceFault_();

  std::unique_ptr<CameraSolutionManager> pCameraSolution;

public:
  DeviceControl();
  DEVICE_RETURN_CODE_T open(void *, std::string, int);
  DEVICE_RETURN_CODE_T close(void *);
  DEVICE_RETURN_CODE_T startPreview(void *, std::string, int *, LSHandle*, const char*);
  DEVICE_RETURN_CODE_T stopPreview(void *, int);
  DEVICE_RETURN_CODE_T startCapture(void *, CAMERA_FORMAT, const std::string&);
  DEVICE_RETURN_CODE_T stopCapture(void *);
  DEVICE_RETURN_CODE_T captureImage(void *, int, CAMERA_FORMAT, const std::string&,
                                    const std::string&);
  DEVICE_RETURN_CODE_T createHandle(void **, std::string);
  DEVICE_RETURN_CODE_T destroyHandle(void *);
  static DEVICE_RETURN_CODE_T getDeviceInfo(std::string, camera_device_info_t *);
  static DEVICE_RETURN_CODE_T getDeviceList(DEVICE_LIST_T *, int *, int *, int *, int *, int);
  DEVICE_RETURN_CODE_T getDeviceProperty(void *, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setDeviceProperty(void *, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setFormat(void *, CAMERA_FORMAT);
  DEVICE_RETURN_CODE_T getFormat(void *, CAMERA_FORMAT *);

  bool registerClient(pid_t, int, int, std::string& outmsg);
  bool unregisterClient(pid_t, std::string& outmsg);
  bool isRegisteredClient(int devhandle);

  void requestPreviewCancel();

  //[Camera Solution Manager] integration start
  DEVICE_RETURN_CODE_T getSupportedCameraSolutionInfo(std::vector<std::string>&);
  DEVICE_RETURN_CODE_T enableCameraSolution(const std::vector<std::string>, std::vector<std::string>&);
  DEVICE_RETURN_CODE_T disableCameraSolution(const std::vector<std::string>, std::vector<std::string>&);
  //[Camera Solution Manager] integration end

};

#endif /*SERVICE_DEVICE_CONTROLLER_H_*/
