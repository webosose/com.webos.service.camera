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

#include "camera_types.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>
#include <vector>

#define COMMAND_TIMEOUT 4000       // ms
#define COMMAND_TIMEOUT_LONG 12000 // ms

class LunaClient;
class Process;
class CameraHalProxy
{
    std::unique_ptr<LunaClient> luna_client{nullptr};
    std::unique_ptr<Process> process_{nullptr};
    std::string service_uri_;

    GMainLoop *loop_{nullptr};
    std::unique_ptr<std::thread> loopThread_;
    unsigned long subscribeKey_{0};
    void *cookie{nullptr};
    std::string uid_;
    using json = nlohmann::json;
    json jOut;

    enum class State
    {
        INIT,
        CREATE,
        DESTROY
    } state_;

    DEVICE_RETURN_CODE_T luna_call_sync(const char *func, const std::string &payload,
                                        int timeout = COMMAND_TIMEOUT, int *fd = nullptr);

public:
    CameraHalProxy();
    ~CameraHalProxy();

    DEVICE_RETURN_CODE_T open(std::string devicenode, int ndev_id, std::string payload);
    DEVICE_RETURN_CODE_T close();
    DEVICE_RETURN_CODE_T startPreview(LSHandle *sh);
    DEVICE_RETURN_CODE_T stopPreview(bool forceComplete);
    DEVICE_RETURN_CODE_T startCapture(CAMERA_FORMAT sformat, const std::string &imagepath,
                                      const std::string &mode, int ncount, const int devHandle = 0);
    DEVICE_RETURN_CODE_T stopCapture(const int devHandle);
    DEVICE_RETURN_CODE_T capture(int ncount, const std::string &imagepath,
                                 std::vector<std::string> &capturedFiles);
    DEVICE_RETURN_CODE_T createHal(std::string subsystem);
    DEVICE_RETURN_CODE_T destroyHal();
    static DEVICE_RETURN_CODE_T getDeviceInfo(std::string strdevicenode, std::string strdevicetype,
                                              camera_device_info_t *pinfo);
    DEVICE_RETURN_CODE_T getDeviceProperty(CAMERA_PROPERTIES_T *oparams);
    DEVICE_RETURN_CODE_T setDeviceProperty(CAMERA_PROPERTIES_T *inparams);
    DEVICE_RETURN_CODE_T setFormat(CAMERA_FORMAT sformat);
    DEVICE_RETURN_CODE_T getFormat(CAMERA_FORMAT *pformat);
    DEVICE_RETURN_CODE_T addClient(int id);
    DEVICE_RETURN_CODE_T removeClient(int id);
    DEVICE_RETURN_CODE_T getFd(const std::string &type, int id, int *fd);

    //[Camera Solution Manager] integration start
    DEVICE_RETURN_CODE_T getSupportedCameraSolutionInfo(std::vector<std::string> &);
    DEVICE_RETURN_CODE_T getEnabledCameraSolutionInfo(std::vector<std::string> &);
    DEVICE_RETURN_CODE_T enableCameraSolution(const std::vector<std::string> &);
    DEVICE_RETURN_CODE_T disableCameraSolution(const std::vector<std::string> &);
    //[Camera Solution Manager] integration end

    bool subscribe();
    bool unsubscribe();
    LSHandle *sh_{nullptr};
    std::vector<int> devHandles_; /* used to stop capture from callback */
};
