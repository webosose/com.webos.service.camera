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

#ifndef SERVICE_DEVICE_CONTROLLER_H_
#define SERVICE_DEVICE_CONTROLLER_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ----------------------------------------------------------------------------*/
#include "camera_constants.h"
#include "camera_types.h"
#include "ipc_posix_shared_memory.h"
#include "ipc_shared_memory.h"
#include <condition_variable>
#include <plugin_factory.hpp>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>

typedef struct
{
    pid_t pid;
    int sig;
    int handle;
} CLIENT_INFO_T;

class CameraSolutionManager;
struct MemoryListener;
class DeviceControl
{
private:
    DEVICE_RETURN_CODE_T writeImageToFile(const void *, int) const;
    DEVICE_RETURN_CODE_T checkFormat(CAMERA_FORMAT);
    DEVICE_RETURN_CODE_T pollForCapturedImage(int) const;
    DEVICE_RETURN_CODE_T captureImage(int, CAMERA_FORMAT, const std::string &, const std::string &);
    static camera_pixel_format_t getPixelFormat(camera_format_t);
    static camera_format_t getCameraFormat(camera_pixel_format_t);
    void captureThread();
    void previewThread();

    bool b_iscontinuous_capture_;
    bool b_isstreamon_;
    bool b_isposixruning;
    bool b_issystemvruning;
    bool b_issystemvruning_mmap;

    IHal *p_cam_hal;
    int shmemfd_;
    buffer_t *usrpbufs_;

    CAMERA_FORMAT informat_;
    camera_pixel_format_t epixelformat_;
    std::thread tidPreview;
    std::thread tidCapture;
    std::mutex tMutex;
    std::condition_variable tCondVar;
    std::string strdevicenode_;
    SHMEM_HANDLE h_shmsystem_;
    SHMEM_HANDLE h_shmposix_;
    std::string str_imagepath_;
    std::string str_capturemode_;
    std::string str_memtype_;
    std::string str_shmemname_;

    static int n_imagecount_;

    std::vector<CLIENT_INFO_T> client_pool_;
    std::mutex client_pool_mutex_;
    void broadcast_();

    bool cancel_preview_;
    int buf_size_;

    LSHandle *sh_;
    std::string subskey_;
    int camera_id_;
    void notifyDeviceFault_();

    std::string deviceType_{"unknown"};
    std::string payload_{""};

    std::shared_ptr<CameraSolutionManager> pCameraSolution;
    std::shared_ptr<MemoryListener> pMemoryListener;

    PluginFactory pluginFactory_;
    IFeaturePtr pFeature_;
    int halFd_{-1};

public:
    DeviceControl();
    DEVICE_RETURN_CODE_T open(std::string, int, std::string);
    DEVICE_RETURN_CODE_T close();
    DEVICE_RETURN_CODE_T startPreview(std::string, int *, LSHandle *, const char *);
    DEVICE_RETURN_CODE_T stopPreview(int);
    DEVICE_RETURN_CODE_T startCapture(CAMERA_FORMAT, const std::string &, const std::string &, int);
    DEVICE_RETURN_CODE_T stopCapture();
    DEVICE_RETURN_CODE_T createHal(std::string);
    DEVICE_RETURN_CODE_T destroyHal();
    static DEVICE_RETURN_CODE_T getDeviceInfo(std::string, std::string, camera_device_info_t *);
    DEVICE_RETURN_CODE_T getDeviceProperty(CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setDeviceProperty(CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setFormat(CAMERA_FORMAT);
    DEVICE_RETURN_CODE_T getFormat(CAMERA_FORMAT *);
    DEVICE_RETURN_CODE_T getFd(int *);

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
};

#endif /*SERVICE_DEVICE_CONTROLLER_H_*/
