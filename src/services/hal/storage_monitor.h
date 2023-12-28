// Copyright (c) 2023 LG Electronics, Inc.
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

#ifndef HAL_SERVICE_STORAGE_MONITOR_H_
#define HAL_SERVICE_STORAGE_MONITOR_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ----------------------------------------------------------------------------*/
#include "camera_types.h"
#include <condition_variable>
#include <functional>
#include <string>
#include <thread>

class StorageMonitor
{
public:
    using HandlerCb = std::function<bool(const DEVICE_RETURN_CODE_T, void *)>;

private:
    std::string path_;

    std::thread tidMonitor_;

    std::condition_variable cv;
    std::mutex cv_m;

    bool monitoring_;

    void *deviceControl_;
    HandlerCb callback_;

private:
    void run();

public:
    StorageMonitor();
    ~StorageMonitor();

public:
    bool monitorInProgress();

    void setPath(std::string path) { path_ = std::move(path); }
    void registerCallback(HandlerCb callback, void *dev)
    {
        callback_      = std::move(callback);
        deviceControl_ = dev;
    }

    bool startMonitor();
    bool stopMonitor();

    static bool isEnoughSpaceAvailable(std::string path);
};

#endif /* HAL_SERVICE_STORAGE_MONITOR_H_ */
