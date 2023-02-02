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

#ifndef __CAMERA_SOLUTION_PROXY__
#define __CAMERA_SOLUTION_PROXY__

#include "camera_hal_if_types.h"
#include "camera_solution.h"
#include <atomic>
#include <glib.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

using namespace nlohmann;

class LunaClient;
class Process;
struct CameraSolutionEvent;
class CameraSolutionProxy
{
    Property solutionProperty_{LG_SOLUTION_NONE};
    bool supportStatus_{false};
    bool enableStatus_{false};
    stream_format_t streamFormat_{CAMERA_PIXEL_FORMAT_JPEG, 0, 0, 0, 0};
    std::string solution_name_;

    std::unique_ptr<LunaClient> luna_client{nullptr};
    std::unique_ptr<Process> process_{nullptr};
    std::string service_uri_;

    GMainLoop *loop_{nullptr};
    std::unique_ptr<std::thread> loopThread_;
    unsigned long subscribeKey_{0};

    int shmKey_{0};

    bool luna_call_sync(const char *func, const std::string &payload, json *j = nullptr);

    bool subscribe();
    bool unsubscribe();
    bool createSolution(void);
    bool destorySolution(void);

public:
    CameraSolutionProxy(const std::string solution_name);
    ~CameraSolutionProxy();

    void setEventListener(CameraSolutionEvent *pEvent) { pEvent_ = pEvent; }
    int32_t getMetaSizeHint(void);
    void initialize(stream_format_t streamFormat, int shmKey);
    void setEnableValue(bool enableValue);
    Property getProperty(void);
    bool isEnabled(void);

    std::string getSolutionStr(void);
    void processForSnapshot(buffer_t inBuf);
    void processForPreview(buffer_t inBuf);
    void release(void);

    std::atomic<CameraSolutionEvent *> pEvent_{nullptr};
};

#endif // __CAMERA_SOLUTION_PROXY__
