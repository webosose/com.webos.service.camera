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

#ifndef SERVICE_DEVICE_MANAGER_H_
#define SERVICE_DEVICE_MANAGER_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "camera_types.h"

class DeviceManager
{
private:

public:
    static DeviceManager &getInstance()
    {
        static DeviceManager obj;
        return obj;
    };
    bool deviceStatus(int ,DEVICE_TYPE_T ,bool );
    bool isDeviceOpen(DEVICE_TYPE_T , int *);
    bool isDeviceValid(DEVICE_TYPE_T , int *);
    bool isUpdatedList();
    DEVICE_RETURN_CODE_T getList(int *, int *, int *, int *);
    DEVICE_RETURN_CODE_T updateList(DEVICE_LIST_T *, int ,DEVICE_EVENT_STATE_T *,DEVICE_EVENT_STATE_T *);
    DEVICE_RETURN_CODE_T getInfo(int , CAMERA_INFO_T *);
    DEVICE_RETURN_CODE_T createHandle(int ,DEVICE_TYPE_T ,int *);
    DEVICE_RETURN_CODE_T getHandle(int ,DEVICE_TYPE_T ,DEVICE_HANDLE *);
};

#endif /*SERVICE_DEVICE_MANAGER_H_*/
