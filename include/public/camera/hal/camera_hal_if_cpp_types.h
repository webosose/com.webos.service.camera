// Copyright (c) 2022 LG Electronics, Inc.
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

#ifndef CAMERA_HAL_IF_CPP_TYPES
#define CAMERA_HAL_IF_CPP_TYPES

#include "camera_hal_if_types.h"
#include <string>
#include <vector>

struct camera_resolution_t
{
    std::vector<std::string> c_res;
    camera_format_t e_format;

    camera_resolution_t(std::vector<std::string> res, camera_format_t format)
        : c_res(move(res)), e_format(format)
    {
    }
};

struct camera_properties_t
{
    camera_queryctrl_t stGetData;
    std::vector<camera_resolution_t> stResolution; //To do remove
};

struct camera_device_info_t
{
    std::string str_devicename;
    std::string str_vendorid;
    std::string str_productid;
    device_t n_devicetype;
    int b_builtin;
    std::vector<camera_resolution_t> stResolution;
};
#endif