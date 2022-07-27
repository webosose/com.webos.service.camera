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
/*Only Use for HAL*/

#ifndef CAMERA_HAL_TYPES
#define CAMERA_HAL_TYPES

#include "PmLogLib.h"
#include <mutex>
#include <stdio.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define CONST_MODULE_HAL "HAL"

#define CONST_PARAM_DEFAULT_VALUE -999

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

static inline PmLogContext getHALLunaPmLogContext()
{
    static PmLogContext usLogContext = 0;
    if (0 == usLogContext)
    {
        PmLogGetContext("HAL", &usLogContext);
    }
    return usLogContext;
}

#define HAL_LOG_INFO(module, FORMAT__, ...)                                                        \
    PmLogInfo(getHALLunaPmLogContext(), module, 0, "%s():%d " FORMAT__, __FUNCTION__, __LINE__,    \
              ##__VA_ARGS__)

#endif
