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

#pragma once

#include <pbnjson.hpp>

/// WhitelistChecker class for camera whitelist from json file.
class WhitelistChecker
{
public:
    WhitelistChecker();
    static WhitelistChecker &getInstance()
    {
        static WhitelistChecker obj;
        return obj;
    }
    ~WhitelistChecker();
    bool check(LSHandle *, const std::string &, const std::string &);
    bool isSupportedCamera(std::string, std::string);

 private:
    bool createToast(LSHandle *,    const std::string &);
    pbnjson::JValue getListFromConfigd();

    /// whitelist file path
    std::string confPath_;
};
