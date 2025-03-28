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
#include "camera_shared_memory_ex.h"
#include "camera_types.h"
#include "storage_monitor.h"
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
    // deprecated
    DEVICE_RETURN_CODE_T writeImageToFile(const void *, int, int cnt = 0) const;
    // deprecated
    DEVICE_RETURN_CODE_T saveShmemory(int ncount = 0) const;
    DEVICE_RETURN_CODE_T writeImageToFile(const void *, unsigned long, int,
                                          std::vector<std::string> &) const;
    DEVICE_RETURN_CODE_T saveShmemory(int, std::vector<std::string> &) const;
    static camera_pixel_format_t getPixelFormat(camera_format_t);
    static camera_format_t getCameraFormat(camera_pixel_format_t);

    void captureThread();
    void previewThread();
    std::string createCaptureFileName(int) const;
    void closeShmemoryIfNeeded();

    bool b_iscontinuous_capture_;
    bool b_isstreamon_;

    IHal *p_cam_hal;

    CAMERA_FORMAT capture_format_;
    std::thread tidPreview;
    std::thread tidCapture;
    std::mutex tMutex;
    std::string strdevicenode_;
    std::string str_imagepath_;
    std::string str_capturemode_;

    int solutionTextSize_{0};
    int solutionBinarySize_{0};

    LSHandle *sh_;
    std::string subskey_;
    int camera_id_;
    void notifyDeviceFault_(EventType eventType, DEVICE_RETURN_CODE_T error = DEVICE_OK);

    StorageMonitor storageMonitor_;

    std::string payload_{""};

    std::shared_ptr<CameraSolutionManager> pCameraSolution;
    std::shared_ptr<MemoryListener> pMemoryListener;

    PluginFactory pluginFactory_;
    IFeaturePtr pFeature_;
    int halFd_{-1};

    std::unique_ptr<CameraSharedMemoryEx> shmem_;
    buffer_t *shmDataBuffers;
    std::vector<buffer_t> shmMetaBuffers_;
    std::vector<buffer_t> shmExtraBuffers_;
    std::vector<buffer_t> shmSolutionBuffers_;
    int shmBufferFd_{-1};
    std::map<int, int> shmSignalFdMap_;

public:
    DeviceControl();
    DEVICE_RETURN_CODE_T open(std::string, int, std::string);
    DEVICE_RETURN_CODE_T close();
    DEVICE_RETURN_CODE_T startPreview(LSHandle *, const char *);
    DEVICE_RETURN_CODE_T stopPreview(bool = false);
    // deprecated
    DEVICE_RETURN_CODE_T startCapture(CAMERA_FORMAT, const std::string &, const std::string &, int);
    // deprecated
    DEVICE_RETURN_CODE_T stopCapture();
    DEVICE_RETURN_CODE_T capture(int, const std::string &, std::vector<std::string> &);
    DEVICE_RETURN_CODE_T createHal(std::string);
    DEVICE_RETURN_CODE_T destroyHal();
    static DEVICE_RETURN_CODE_T getDeviceInfo(std::string, std::string, camera_device_info_t *);
    DEVICE_RETURN_CODE_T getDeviceProperty(CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setDeviceProperty(CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setFormat(CAMERA_FORMAT);
    DEVICE_RETURN_CODE_T getFormat(CAMERA_FORMAT *);
    DEVICE_RETURN_CODE_T addClient(int id);
    DEVICE_RETURN_CODE_T removeClient(int id);
    DEVICE_RETURN_CODE_T getShmBufferFd(int *fd);
    DEVICE_RETURN_CODE_T getShmSignalFd(int id, int *fd);

    bool notifyStorageError(const DEVICE_RETURN_CODE_T);

    //[Camera Solution Manager] integration start
    DEVICE_RETURN_CODE_T getSupportedCameraSolutionInfo(std::vector<std::string> &);
    DEVICE_RETURN_CODE_T getEnabledCameraSolutionInfo(std::vector<std::string> &);
    DEVICE_RETURN_CODE_T enableCameraSolution(const std::vector<std::string> &);
    DEVICE_RETURN_CODE_T disableCameraSolution(const std::vector<std::string> &);
    //[Camera Solution Manager] integration end

private:
    bool updateMetaBuffer(const buffer_t &buffer, const json &videoMeta, const json &extraMeta);
    bool updateSolutionBuffer(const buffer_t &buffer);
};

#endif /*SERVICE_DEVICE_CONTROLLER_H_*/
