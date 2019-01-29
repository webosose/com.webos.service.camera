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

#ifndef CAMERA_BASE
#define CAMERA_BASE

#include "camera_hal_types.h"
#include <iostream>
#include <string.h>

class CameraBase
{
public:
  virtual int openDevice(std::string) = 0;
  virtual int closeDevice() = 0;
  virtual int setFormat(stream_format_t) = 0;
  virtual int getFormat(stream_format_t *) = 0;
  virtual int setBuffer(int, int) = 0;
  virtual int getBuffer(buffer_t *) = 0;
  virtual int releaseBuffer(buffer_t) = 0;
  virtual int destroyBuffer() = 0;
  virtual int startCapture() = 0;
  virtual int stopCapture() = 0;
  virtual int setProperties(const camera_properties_t *) = 0;
  virtual int getProperties(camera_properties_t *) = 0;
  virtual int getInfo(camera_device_info_t *, std::string) = 0;
};

#endif
