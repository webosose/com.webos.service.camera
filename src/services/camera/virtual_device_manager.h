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

#ifndef VIRTUAL_DEVICE_MANAGER_H_
#define VIRTUAL_DEVICE_MANAGER_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ----------------------------------------------------------------------------*/
#include "addon.h"
#include "camera_hal_proxy.h"
#include "camera_types.h"
#include <map>
#include <string>
#include <vector>

class DeviceStateMap
{
public:
    int ndeviceid_;
    std::string shmemtype;
    CameraDeviceState ecamstate_;
    DeviceStateMap()
        : ndeviceid_(0), shmemtype(""), ecamstate_(CameraDeviceState::CAM_DEVICE_STATE_CLOSE){};
};

class PreviewDisplayControl;
class VirtualDeviceManager
{
private:
    std::map<int, DeviceStateMap> virtualhandle_map_;
    std::map<int, std::string> handlepriority_map_;
    bool bcaptureinprogress_;
    int shmkey_;
    std::vector<int> nstreaminghandle_;
    std::vector<int> ncapturehandle_;
    CAMERA_FORMAT sformat_;

    // for render preview
    struct UMSControl
    {
        int handle;
        std::string mediaId;
        std::unique_ptr<PreviewDisplayControl> display_control;
    };
    std::vector<UMSControl> ums_controls;

    // for multi obj
    CameraHalProxy objcamerahalproxy_;
    std::shared_ptr<AddOn> pAddon_;

    bool checkDeviceOpen(int);
    bool checkAppPriorityMap();
    int getVirtualDeviceHandle(int);
    void removeVirtualDeviceHandle(int);
    std::string getAppPriority(int);
    void removeHandlePriorityObj(int);
    void updateFormat(CAMERA_FORMAT &, int);
    DEVICE_RETURN_CODE_T openDevice(int, int *);
    DEVICE_RETURN_CODE_T singleCapture(int, CAMERA_FORMAT, const std::string &, const std::string &,
                                       int);
    DEVICE_RETURN_CODE_T continuousCapture(int, CAMERA_FORMAT, const std::string &);
    std::string startPreviewDisplay(int, std::string, std::string, int);
    bool stopPreviewDisplay(int);
    inline bool isValidMemtype(const std::string &memtype)
    {
        return (memtype == kMemtypeShmemMmap || memtype == kMemtypeShmem ||
                memtype == kMemtypePosixshm);
    }

public:
    VirtualDeviceManager();
    ~VirtualDeviceManager();
    DEVICE_RETURN_CODE_T open(int, int *, std::string, std::string);
    DEVICE_RETURN_CODE_T close(int);
    DEVICE_RETURN_CODE_T startCamera(int, std::string, int *, LSHandle *, const char *);
    DEVICE_RETURN_CODE_T stopCamera(int);
    DEVICE_RETURN_CODE_T startPreview(int, std::string, int *, std::string, std::string *,
                                      LSHandle *, const char *);
    DEVICE_RETURN_CODE_T stopPreview(int);
    DEVICE_RETURN_CODE_T startCapture(int, CAMERA_FORMAT, const std::string &, const std::string &,
                                      int);
    DEVICE_RETURN_CODE_T stopCapture(int, bool request = true);
    DEVICE_RETURN_CODE_T capture(int, int, const std::string &, std::vector<std::string> &);
    DEVICE_RETURN_CODE_T getProperty(int, CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setProperty(int, CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setFormat(int, CAMERA_FORMAT);
    DEVICE_RETURN_CODE_T getFormat(int, CAMERA_FORMAT *);
    DEVICE_RETURN_CODE_T getFd(int, int *);

    DEVICE_RETURN_CODE_T registerClient(int, int, int, std::string &);
    DEVICE_RETURN_CODE_T unregisterClient(int, std::string &);
    bool isRegisteredClient(int);

    void requestPreviewCancel();

    DEVICE_RETURN_CODE_T getSupportedCameraSolutionInfo(int, std::vector<std::string> &);
    DEVICE_RETURN_CODE_T getEnabledCameraSolutionInfo(int, std::vector<std::string> &);
    DEVICE_RETURN_CODE_T enableCameraSolution(int, const std::vector<std::string> &);
    DEVICE_RETURN_CODE_T disableCameraSolution(int, const std::vector<std::string> &);

    void setAddon(std::shared_ptr<AddOn> &addon) { pAddon_ = addon; }

    CameraDeviceState getDeviceState(int);
};

#endif /*VIRTUAL_DEVICE_MANAGER_H_*/
