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

#ifndef VIRTUAL_DEVICE_MANAGER_H_
#define VIRTUAL_DEVICE_MANAGER_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "camera_types.h"
#include <map>
#include <string>
#include <vector>

const std::string empty = "";

class AppDetails
{
public:
  std::string apppriority;
  int virtualhandle;
};

class VirtualDeviceManager
{
private:
  std::map<int, int> virtualhandle_map_;
  std::map<std::string, AppDetails> appdetails_map_;
  bool bpreviewinprogress_;
  bool bcaptureinprogress_;
  int shmkey_;
  std::vector<int> npreviewhandle_;
  std::vector<int> ncapturehandle_;
  FORMAT sformat_;

  bool checkAppIdMap(std::string);
  bool checkAppPriorityMap();
  int getDeviceHandle(int);
  void removeDeviceHandle(int);
  std::string getAppPriority(int);

  DEVICE_RETURN_CODE_T openDevice(int, int *);

public:
  VirtualDeviceManager();
  static VirtualDeviceManager &getInstance()
  {
    static VirtualDeviceManager obj;
    return obj;
  }

  DEVICE_RETURN_CODE_T open(int, int *, std::string, std::string);
  DEVICE_RETURN_CODE_T close(int, std::string);
  DEVICE_RETURN_CODE_T startPreview(int, int *);
  DEVICE_RETURN_CODE_T stopPreview(int);
  DEVICE_RETURN_CODE_T captureImage(int, int, FORMAT);
  DEVICE_RETURN_CODE_T startCapture(int, FORMAT);
  DEVICE_RETURN_CODE_T stopCapture(int);
  DEVICE_RETURN_CODE_T getProperty(int, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setProperty(int, CAMERA_PROPERTIES_T *);
  DEVICE_RETURN_CODE_T setFormat(int, FORMAT);
};

#endif /*VIRTUAL_DEVICE_MANAGER_H_*/
