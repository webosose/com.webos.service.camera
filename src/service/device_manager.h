// Copyright (c) 2018 LG Electronics, Inc.
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

#ifndef SRC_SERVICE_DEVICE_MANAGER_H_
#define SRC_SERVICE_DEVICE_MANAGER_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/

#include "service_main.h"

class DeviceManager
{
private:
    static DeviceManager *devInfoinstance;
    DeviceManager()
    {}

public:
    static DeviceManager *getInstance()
    {
        if (devInfoinstance == 0)
        {
            devInfoinstance = new DeviceManager();
        }
        return devInfoinstance;
    };
    int deviceID;
    int devType;
    char buf[CONST_MAX_PATH];
    bool deviceStatus(int deviceID,DEVICE_TYPE_T devType,bool status);
    bool isDeviceOpen(DEVICE_TYPE_T devType, int deviceID);
    bool isDeviceValid(DEVICE_TYPE_T devType, int deviceID);
    bool isUpdatedList();
    DEVICE_RETURN_CODE_T getList(int *pCamDev, int *pMicDev, int *pCamSupport, int *pMicSupport);
    DEVICE_RETURN_CODE_T updateList(DEVICE_LIST_T *pList, int nDevCount,DEVICE_EVENT_STATE_T *pCamEvent,DEVICE_EVENT_STATE_T *pMicEvent);
    DEVICE_RETURN_CODE_T getInfo(int ndevID, CAMERA_INFO_T *pInfo);
    DEVICE_RETURN_CODE_T createHandle(int nDeviceID,DEVICE_TYPE_T devType,int *ndevID);
    DEVICE_RETURN_CODE_T getHandle(int nDeviceID,DEVICE_TYPE_T devType,DEVICE_HANDLE *devHandle);
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SRC_SERVICE_DEVICE_MANAGER_H_*/
