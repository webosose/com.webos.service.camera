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

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ----------------------------------------------------------------------------*/
#include "storage_monitor.h"
#include <sys/statvfs.h>
#include <chrono>

using namespace std::chrono_literals;

StorageMonitor::StorageMonitor()
    : path_(""), monitoring_(false),
      deviceControl_(nullptr), callback_(nullptr)
{
}

StorageMonitor::~StorageMonitor()
{
    stopMonitor();
}

bool StorageMonitor::monitorInProgress()
{
    std::unique_lock<std::mutex> lock(cv_m);
    return monitoring_;
}

void StorageMonitor::run()
{
    PLOGI("Monitoring thread started");
    do {
        if (!isEnoughSpaceAvailable(path_) && callback_) {
            callback_(DEVICE_ERROR_FAIL_TO_WRITE_FILE, deviceControl_);
        }
        {
            std::unique_lock<std::mutex> lock(cv_m);
            if (!monitoring_)
                break;
            cv.wait_for(lock, 100ms, [this] { return !monitoring_; });
        }
    } while (1);
    PLOGI("Monitoring thread terminated");
}

bool StorageMonitor::startMonitor()
{
    std::unique_lock<std::mutex> lock(cv_m);

    if (monitoring_)
        return true;

    monitoring_ = true;
    tidMonitor_ = std::thread{[this]() { this->run(); }};
    tidMonitor_.detach();
    return true;
}

bool StorageMonitor::stopMonitor()
{
    std::unique_lock<std::mutex> lock(cv_m);

    if (!monitoring_)
        return true;

    monitoring_ = false;
    if (tidMonitor_.joinable()) {
        tidMonitor_.join();
    }
    callback_      = nullptr;
    deviceControl_ = nullptr;

    PLOGI("Storage monitoring stopped");
    return true;
}

bool StorageMonitor::isEnoughSpaceAvailable(std::string path)
{
    struct statvfs fiData;
    unsigned long freeSpace = 0;

    if (path.empty())
        return false;

    if (statvfs(path.c_str(), &fiData) < 0 )
    {
        PLOGE("Failed to stat! : %s", path.c_str());
        return false;
    }

    unsigned long f_blocks = fiData.f_blocks;
    unsigned long f_frsize = fiData.f_frsize;

    unsigned long f_bavail = fiData.f_bavail;
    unsigned long f_bsize  = fiData.f_bsize;

    freeSpace = (f_bavail * 100 / f_blocks) * f_bsize / f_frsize;

    PLOGI("free space limit %lu", freeSpace);
    if(freeSpace < MEMORY_SPACE_THRESHOLD)
        return false;

    return true;
}
