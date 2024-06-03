// Copyright (c) 2019-2022 LG Electronics, Inc.
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

#define LOG_TAG "WhitelistChecker"
#include "whitelist_checker.h"
#include "camera_types.h"

bool WhitelistChecker::check(const std::string &productId, const std::string &vendorId)
{
    PLOGI("productId=[%s], vendorId=[%s]", productId.c_str(), vendorId.c_str());
    bool retValue = true;
    PLOGI("supported = %d", retValue);
    return retValue;
}

bool WhitelistChecker::isSupportedCamera(const std::string &productId, const std::string &vendorId)
{
    PLOGI("productId=[%s], vendorId=[%s]", productId.c_str(), vendorId.c_str());
    bool retValue = true;
    PLOGI("supported = %d", retValue);
    return retValue;
}
