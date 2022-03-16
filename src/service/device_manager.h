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
    int nDeviceID;         // devicehandle - random number
    int nDevIndex;         // index
    int nDevCount;         // device count (# of device)
    void *pcamhandle;      // HAL handle
    bool isDeviceOpen;     // open or close
    DEVICE_TYPE_T devType; // CAM or MIC
    DEVICE_LIST_T stList;  // name, id, node ...
    std::vector<int> handleList;
} DEVICE_STATUS;

class DeviceManager
{
private:
    std::map<int, DEVICE_STATUS> deviceMap_;
    int findDevNum(int);
    int remoteCamIdx_{0};
    int fakeCamIdx_{0};
    AppCastClient *appCastClient_{nullptr};

public:
    DeviceManager();
    static DeviceManager &getInstance()
    {
        static DeviceManager obj;
        return obj;
    }
    bool deviceStatus(int, DEVICE_TYPE_T, bool);
    bool isDeviceOpen(int *);
    bool isDeviceValid(DEVICE_TYPE_T, int *);
    void getDeviceNode(int *, std::string &);
    void getDeviceHandle(int *, void **);
    int getDeviceId(int *);
    bool addVirtualHandle(int devid, int virtualHandle);
    bool eraseVirtualHandle(int deviceId, int virtualHandle);
    int addDevice(DEVICE_LIST_T *pList);
    bool removeDevice(int devid);

    DEVICE_RETURN_CODE_T getList(int *, int *, int *, int *) const;
    DEVICE_RETURN_CODE_T updateList(DEVICE_LIST_T *, int, DEVICE_EVENT_STATE_T *,
                                    DEVICE_EVENT_STATE_T *);
    DEVICE_RETURN_CODE_T getInfo(int, camera_device_info_t *);
    DEVICE_RETURN_CODE_T updateHandle(int, void *);

    int addRemoteCamera(deviceInfo_t *deviceInfo, bool fakeCamera = false);
    int removeRemoteCamera();
    int set_appcastclient(AppCastClient *);
    AppCastClient *get_appcastclient();
    bool isRemoteCamera(DEVICE_LIST_T &);
    bool isRemoteCamera(void *);
    bool isRemoteCamera(int);
    bool isFakeCamera(int);
    void printCameraStatus();
};

#endif /*SERVICE_DEVICE_MANAGER_H_*/
