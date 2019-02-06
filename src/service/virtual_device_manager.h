// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef VIRTUAL_DEVICE_MANAGER_H_
#define VIRTUAL_DEVICE_MANAGER_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "camera_types.h"
#include "device_controller.h"
#include <map>
#include <string>
#include <vector>

class DeviceStateMap
{
public:
  int ndeviceid_;
  CameraDeviceState ecamstate_;
};

class VirtualDeviceManager
{
private:
  std::map<int, DeviceStateMap> virtualhandle_map_;
  std::map<int, std::string> handlepriority_map_;
  bool bpreviewinprogress_;
  bool bcaptureinprogress_;
  int shmkey_;
  std::vector<int> npreviewhandle_;
  std::vector<int> ncapturehandle_;
  CAMERA_FORMAT sformat_;
  // for multi obj
  DeviceControl objdevicecontrol_;

  bool checkDeviceOpen(int);
  bool checkAppPriorityMap();
  int getVirtualDeviceHandle(int);
  void removeVirtualDeviceHandle(int);
  std::string getAppPriority(int);
  void removeHandlePriorityObj(int);

  DEVICE_RETURN_CODE_T openDevice(int, int *);

public:
  VirtualDeviceManager();
  DEVICE_RETURN_CODE_T open(int, int *, std::string);
  DEVICE_RETURN_CODE_T close(int);
  DEVICE_RETURN_CODE_T startPreview(int, int *);
  DEVICE_RETURN_CODE_T stopPreview(int);
  DEVICE_RETURN_CODE_T captureImage(int, int, CAMERA_FORMAT);
  DEVICE_RETURN_CODE_T startCapture(int, CAMERA_FORMAT);
  DEVICE_RETURN_CODE_T stopCapture(int);
  DEVICE_RETURN_CODE_T getProperty(int, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setProperty(int, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setFormat(int, CAMERA_FORMAT);
  CAMERA_FORMAT getFormat() const;
};

#endif /*VIRTUAL_DEVICE_MANAGER_H_*/
