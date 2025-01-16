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
#pragma once

#include "camera_hal_types.h"
#include "camera_solution.h"
#include <atomic>
#include <condition_variable>
#include <glib.h>
#include <memory>
#include <string>
#include <thread>

class LunaClient;
class Process;
struct CameraSolutionEvent;
class CameraSolutionProxy
{
    int solutionProperty_ = LG_SOLUTION_NONE;
    bool preRun_{false};
    bool enableStatus_{false};
    stream_format_t streamFormat_{CAMERA_PIXEL_FORMAT_JPEG, 0, 0, 0, 0};
    std::string solution_name_;

    std::unique_ptr<LunaClient> luna_client{nullptr};
    std::unique_ptr<Process> process_{nullptr};
    std::string service_uri_;

    GMainLoop *loop_{nullptr};
    std::unique_ptr<std::thread> loopThread_;
    unsigned long subscribeKey_{0};

    std::string shmName_;
    LSHandle *sh_{nullptr};
    void *cookie{nullptr};
    std::string uid_;

    bool job_ready{false};
    std::condition_variable cv_;
    std::mutex m_;
    std::mutex mtxJob_;
    std::queue<int> queueJob_;
    std::unique_ptr<std::thread> threadJob_;
    std::atomic<bool> bAlive_{false};
    std::atomic<bool> bThreadStarted_{false};

    bool startProcess();
    bool stopProcess();
    bool create();
    bool init();
    bool subscribe();
    bool unsubscribe();
    bool luna_call_sync(const char *func, const std::string &payload);

    void run();
    void processing(bool enableValue);
    void startThread();
    void stopThread();
    void notify();
    bool wait();
    bool checkAlive() { return bAlive_; }
    void setAlive(bool bAlive) { bAlive_ = bAlive; }

    void pushJob(int inValue);
    void popJob();

public:
    CameraSolutionProxy(const std::string &solution_name);
    ~CameraSolutionProxy();

    void setEventListener(CameraSolutionEvent *pEvent) { pEvent_ = pEvent; }
    int32_t getMetaSizeHint(void);
    void initialize(stream_format_t streamFormat, const std::string &shmName, LSHandle *sh);
    void setEnableValue(bool enableValue);
    int getProperty(void) { return solutionProperty_; }
    bool isEnabled(void) { return bAlive_ ? enableStatus_ : preRun_; };

    std::string getSolutionStr(void) { return solution_name_; };
    void processForSnapshot(buffer_t inBuf){};
    void processForPreview(buffer_t inBuf){};
    void release(void);

    std::atomic<CameraSolutionEvent *> pEvent_{nullptr};
};
