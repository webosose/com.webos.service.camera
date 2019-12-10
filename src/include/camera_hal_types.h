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

#ifndef CAMERA_HAL_TYPES
#define CAMERA_HAL_TYPES

#include <stdio.h>
#include <mutex>
#include "PmLogLib.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define CONST_MODULE_HAL "HAL"

#define CONST_PARAM_DEFAULT_VALUE -999

typedef enum
{
  CAMERA_ERROR_UNKNOWN = -1,
  CAMERA_ERROR_NONE = 0,
  CAMERA_ERROR_DEVICE_OPEN,
  CAMERA_ERROR_DEVICE_CLOSE,
  CAMERA_ERROR_PLUGIN_NOT_FOUND,
  CAMERA_ERROR_CREATE_HANDLE,
  CAMERA_ERROR_DESTROY_HANDLE,
  CAMERA_ERROR_SET_FORMAT,
  CAMERA_ERROR_GET_FORMAT,
  CAMERA_ERROR_SET_BUFFER,
  CAMERA_ERROR_GET_BUFFER,
  CAMERA_ERROR_RELEASE_BUFFER,
  CAMERA_ERROR_DESTROY_BUFFER,
  CAMERA_ERROR_START_CAPTURE,
  CAMERA_ERROR_STOP_CAPTURE,
  CAMERA_ERROR_SET_PROPERTIES,
  CAMERA_ERROR_GET_PROPERTIES,
  CAMERA_ERROR_GET_INFO,
} camera_error_t;

typedef enum
{
  CAMERA_HAL_STATE_UNKNOWN = 0,
  CAMERA_HAL_STATE_INIT,
  CAMERA_HAL_STATE_OPEN,
  CAMERA_HAL_STATE_STREAMING
} camera_hal_state_t;

/*Structures*/

typedef struct
{
  void *h_plugin;
  void *handle;
  int fd;
  int current_state;
  std::mutex lock;
} camera_handle_t;

static PmLogContext getHALLunaPmLogContext()
{
  static PmLogContext usLogContext = 0;
  if (0 == usLogContext)
  {
    PmLogGetContext("HAL", &usLogContext);
  }
  return usLogContext;
}

#define HAL_LOG_INFO(module, args...) PmLogMsg(getHALLunaPmLogContext(), Info, module, 0, ##args)

#endif
