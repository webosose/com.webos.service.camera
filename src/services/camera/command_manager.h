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

#ifndef SERVICE_COMMAND_MANAGER_H_
#define SERVICE_COMMAND_MANAGER_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ----------------------------------------------------------------------------*/
#include "addon.h"
#include "camera_types.h"
#include "virtual_device_manager.h"
#include <map>
#include <string>

class Device
{
public:
    VirtualDeviceManager *ptr;
    int devicehandle;
    int deviceid;
    std::string clientName;
};

class CommandManager
{
private:
    std::multimap<std::string, Device> virtualdevmgrobj_map_;

    VirtualDeviceManager *getVirtualDeviceMgrObj(int);
    void removeVirtualDevMgrObj(int);
    std::shared_ptr<AddOn> pAddon_;

public:
    static CommandManager &getInstance()
    {
        static CommandManager obj;
        return obj;
    }

    DEVICE_RETURN_CODE_T open(int, int *, std::string = "", std::string = "");
    DEVICE_RETURN_CODE_T close(int);
    static DEVICE_RETURN_CODE_T getDeviceInfo(int, camera_device_info_t *);
    static DEVICE_RETURN_CODE_T getDeviceList(std::vector<int> &);
    DEVICE_RETURN_CODE_T getProperty(int, CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setProperty(int, CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setFormat(int, CAMERA_FORMAT);
    DEVICE_RETURN_CODE_T startPreview(int, std::string, int *, LSHandle *, const char *);
    DEVICE_RETURN_CODE_T stopPreview(int);
    DEVICE_RETURN_CODE_T startCapture(int, CAMERA_FORMAT, const std::string &, const std::string &,
                                      int);
    DEVICE_RETURN_CODE_T stopCapture(int);
    DEVICE_RETURN_CODE_T getFormat(int, CAMERA_FORMAT *);
    DEVICE_RETURN_CODE_T getFd(int, int *);
    DEVICE_RETURN_CODE_T getSupportedCameraSolutionInfo(int, std::vector<std::string> &);
    DEVICE_RETURN_CODE_T getEnabledCameraSolutionInfo(int, std::vector<std::string> &);
    DEVICE_RETURN_CODE_T enableCameraSolution(int, const std::vector<std::string>);
    DEVICE_RETURN_CODE_T disableCameraSolution(int, const std::vector<std::string>);

    int getCameraId(int);
    int getCameraHandle(int);

    DEVICE_RETURN_CODE_T registerClientPid(int, int, int, std::string &);
    DEVICE_RETURN_CODE_T unregisterClientPid(int, int, std::string &);
    bool isRegisteredClientPid(int);

    bool setClientDevice(int, std::string);
    DEVICE_RETURN_CODE_T checkDeviceClient(int, std::string);
    void closeClientDevice(std::string);

    void handleCrash();

    void release(int);
    void setAddon(std::shared_ptr<AddOn> &addon) { pAddon_ = addon; }
};

#endif /*SERVICE_COMMAND_MANAGER_H_*/
