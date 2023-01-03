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
#include "appcast_client.h"
#include "camera_types.h"
#include <map>
#include <vector>

typedef struct _DEVICE_STATUS
{
    // Device
    void *pcamhandle;     // HAL handle
    bool isDeviceOpen;    // open or close
    DEVICE_LIST_T stList; // name, id, node ...
    std::vector<int> handleList;
} DEVICE_STATUS;

class DeviceManager
{
private:
    std::map<int, DEVICE_STATUS> deviceMap_;
    int findDevNum(int);
    AppCastClient *appCastClient_{nullptr};
    bool isDeviceIdValid(int deviceid);

public:
    DeviceManager();
    static DeviceManager &getInstance()
    {
        static DeviceManager obj;
        return obj;
    }
    bool setDeviceStatus(int, bool);
    bool isDeviceOpen(int);
    bool isDeviceValid(int);
    void getDeviceNode(int, std::string &);
    void getDeviceHandle(int, void **);
    std::string getDeviceType(int);
    int getDeviceCounts(std::string);
    bool addVirtualHandle(int devid, int virtualHandle);
    bool eraseVirtualHandle(int deviceId, int virtualHandle);
    int addDevice(DEVICE_LIST_T *pList);
    bool removeDevice(int devid);

    bool getCurrentDeviceInfo(std::string &productId, std::string &vendorId,
                              std::string &productName);
    DEVICE_RETURN_CODE_T getDeviceIdList(std::vector<int> &);
    DEVICE_RETURN_CODE_T getInfo(int, camera_device_info_t *);
    DEVICE_RETURN_CODE_T updateHandle(int, void *);

    int addRemoteCamera(deviceInfo_t *deviceInfo);
    int removeRemoteCamera(int);
    int set_appcastclient(AppCastClient *);
    AppCastClient *get_appcastclient();
    bool isRemoteCamera(DEVICE_LIST_T &);
    bool isRemoteCamera(void *);
    void printCameraStatus();
};

#endif /*SERVICE_DEVICE_MANAGER_H_*/
