// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef SERVICE_DEVICE_MANAGER_H_
#define SERVICE_DEVICE_MANAGER_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ----------------------------------------------------------------------------*/
#include "addon.h"
#include "camera_types.h"
#include "luna-service2/lunaservice.h"
#include <map>
#include <vector>

typedef struct DEVICE_STATUS_
{
    bool isDeviceOpen;    // open or close
    DEVICE_LIST_T stList; // name, id, node ...
    std::vector<int> handleList;
    bool isDeviceInfoSaved;
    camera_device_info_t deviceInfoDB;
} DEVICE_STATUS;

class DeviceManager
{
private:
    std::map<int, DEVICE_STATUS> deviceMap_;
    LSHandle *lshandle_{nullptr};
    std::shared_ptr<AddOn> pAddon_;

    bool isDeviceIdValid(int deviceid);

public:
    DeviceManager();
    static DeviceManager &getInstance()
    {
        static DeviceManager obj;
        return obj;
    }

    bool isDeviceOpen(int);
    bool isDeviceValid(int);
    bool setDeviceStatus(int, bool);
    void getDeviceNode(int, std::string &);
    std::string getDeviceType(int);
    std::string getDeviceKey(int);
    int getDeviceCounts(std::string);
    bool getDeviceUserData(int, std::string &);

    int addDevice(const DEVICE_LIST_T &deviceInfo);
    bool removeDevice(int devid);
    bool updateDeviceList(std::string, const std::vector<DEVICE_LIST_T> &);

    DEVICE_RETURN_CODE_T getDeviceIdList(std::vector<int> &);
    DEVICE_RETURN_CODE_T getInfo(int, camera_device_info_t *);

    void setAddon(std::shared_ptr<AddOn> &addon) { pAddon_ = addon; }
    void setLSHandle(LSHandle *sh) { lshandle_ = sh; }
};

#endif /*SERVICE_DEVICE_MANAGER_H_*/
