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

#ifndef SERVICE_COMMAND_MANAGER_H_
#define SERVICE_COMMAND_MANAGER_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "camera_types.h"

class CommandManager
{
private:

public:
    static CommandManager &getInstance()
    {
        static CommandManager obj;
        return obj;
    }

    DEVICE_RETURN_CODE_T open(int , DEVICE_TYPE_T ,int *);
    DEVICE_RETURN_CODE_T close(int , DEVICE_TYPE_T );
    DEVICE_RETURN_CODE_T getDeviceInfo(int , DEVICE_TYPE_T , CAMERA_INFO_T *);
    DEVICE_RETURN_CODE_T getDeviceList(int *, int *, int *, int *);
    DEVICE_RETURN_CODE_T createHandle(int , DEVICE_TYPE_T , int *);
    DEVICE_RETURN_CODE_T updateList(DEVICE_LIST_T *, int ,DEVICE_EVENT_STATE_T *, DEVICE_EVENT_STATE_T *);
    DEVICE_RETURN_CODE_T getProperty(int , DEVICE_TYPE_T ,CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setProperty(int , DEVICE_TYPE_T ,CAMERA_PROPERTIES_T *);
    DEVICE_RETURN_CODE_T setFormat(int , DEVICE_TYPE_T ,FORMAT );
    DEVICE_RETURN_CODE_T startPreview(int , DEVICE_TYPE_T , int *);
    DEVICE_RETURN_CODE_T stopPreview(int , DEVICE_TYPE_T );
    DEVICE_RETURN_CODE_T startCapture(int , DEVICE_TYPE_T , FORMAT );
    DEVICE_RETURN_CODE_T stopCapture(int , DEVICE_TYPE_T );
    DEVICE_RETURN_CODE_T captureImage(int , DEVICE_TYPE_T , int , FORMAT );
};

#endif /*SERVICE_COMMAND_MANAGER_H_*/
