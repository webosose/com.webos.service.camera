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

#ifndef __CAMERA_HAL_PROXY__
#define __CAMERA_HAL_PROXY__

#include "camera_types.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <thread>

#define COMMAND_TIMEOUT 2700      // ms
#define COMMAND_TIMEOUT_LONG 9700 // ms

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
                                        int timeout = COMMAND_TIMEOUT);

public:
    CameraHalProxy();
    ~CameraHalProxy();

    DEVICE_RETURN_CODE_T open(std::string devicenode, int ndev_id, std::string payload);
    DEVICE_RETURN_CODE_T close();
    DEVICE_RETURN_CODE_T startPreview(std::string memtype, int *pkey, LSHandle *sh,
                                      const char *subskey);
    DEVICE_RETURN_CODE_T stopPreview(int memtype);
    DEVICE_RETURN_CODE_T startCapture(CAMERA_FORMAT sformat, const std::string &imagepath);
    DEVICE_RETURN_CODE_T stopCapture();
    DEVICE_RETURN_CODE_T captureImage(int ncount, CAMERA_FORMAT sformat,
                                      const std::string &imagepath, const std::string &mode);
    DEVICE_RETURN_CODE_T createHandle(std::string subsystem);
    DEVICE_RETURN_CODE_T destroyHandle();
    static DEVICE_RETURN_CODE_T getDeviceInfo(std::string strdevicenode, std::string strdevicetype,
                                              camera_device_info_t *pinfo);
    DEVICE_RETURN_CODE_T getDeviceProperty(CAMERA_PROPERTIES_T *oparams);
    DEVICE_RETURN_CODE_T setDeviceProperty(CAMERA_PROPERTIES_T *inparams);
    DEVICE_RETURN_CODE_T setFormat(CAMERA_FORMAT sformat);
    DEVICE_RETURN_CODE_T getFormat(CAMERA_FORMAT *pformat);
    DEVICE_RETURN_CODE_T getFd(int *posix_shm_fd);

    DEVICE_RETURN_CODE_T registerClient(pid_t, int, int, std::string &outmsg);
    DEVICE_RETURN_CODE_T unregisterClient(pid_t, std::string &outmsg);
    bool isRegisteredClient(int devhandle);

    void requestPreviewCancel();

    //[Camera Solution Manager] integration start
    DEVICE_RETURN_CODE_T getSupportedCameraSolutionInfo(std::vector<std::string> &);
    DEVICE_RETURN_CODE_T getEnabledCameraSolutionInfo(std::vector<std::string> &);
    DEVICE_RETURN_CODE_T enableCameraSolution(const std::vector<std::string>);
    DEVICE_RETURN_CODE_T disableCameraSolution(const std::vector<std::string>);
    //[Camera Solution Manager] integration end

    bool subscribe();
    bool unsubscribe();
    LSHandle *sh_{nullptr};
    std::string subsKey_;
};

#endif // __CAMERA_HAL_PROXY__
