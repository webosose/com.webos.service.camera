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

#ifndef SRC_SERVICE_COMMAND_MANAGER_H_
#define SRC_SERVICE_COMMAND_MANAGER_H_
#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "camera_types.h"

class CommandManager
{
private:
    static CommandManager *cmdhandlerinstance;
    CommandManager()
    {
    }

public:
    static CommandManager *getInstance()
    {
        if (cmdhandlerinstance == 0)
        {
            cmdhandlerinstance = new CommandManager();
        }

        return cmdhandlerinstance;
    }
    ;
    //Open the Device and return the handle
    DEVICE_RETURN_CODE_T open(int deviceID, DEVICE_TYPE_T devType);
    //Close the device
    DEVICE_RETURN_CODE_T close(int deviceID, DEVICE_TYPE_T devType);
    // Returns/Notify the devices upluged in/out
    DEVICE_RETURN_CODE_T getDeviceStatus();
    DEVICE_RETURN_CODE_T getDeviceInfo(int deviceID, DEVICE_TYPE_T devType, CAMERA_INFO_T *pInfo);
    DEVICE_RETURN_CODE_T getDeviceList(int *pCamDev, int *pMicDev, int *pCamSupport,
            int *pMicSupport);
    DEVICE_RETURN_CODE_T createHandle(int deviceID, DEVICE_TYPE_T devType, int *devhandle);
    DEVICE_RETURN_CODE_T updateList(DEVICE_LIST_T *pList, int nDevCount,
            DEVICE_EVENT_STATE_T *pCamEvent, DEVICE_EVENT_STATE_T *pMicEvent);
    DEVICE_RETURN_CODE_T joinSession(int deviceID, DEVICE_TYPE_T devType);
    DEVICE_RETURN_CODE_T leaveSession(int deviceID, DEVICE_TYPE_T devType);
    DEVICE_RETURN_CODE_T getProperty(int deviceID, DEVICE_TYPE_T devType,
            CAMERA_PROPERTIES_T *devproperty);
    DEVICE_RETURN_CODE_T setProperty(int deviceID, DEVICE_TYPE_T devType,
            CAMERA_PROPERTIES_T *oInfo);
    DEVICE_RETURN_CODE_T startPreview(int deviceID, DEVICE_TYPE_T devType, int pKey);
    //Stop Capture
    DEVICE_RETURN_CODE_T stopPreview(int deviceID, DEVICE_TYPE_T devType);
    DEVICE_RETURN_CODE_T startCapture(int deviceID, DEVICE_TYPE_T devType, FORMAT sFormat);
    //Stop Capture
    DEVICE_RETURN_CODE_T stopCapture(int deviceID, DEVICE_TYPE_T devType);
    DEVICE_RETURN_CODE_T captureImage(int deviceID, DEVICE_TYPE_T devType, int nCount,
            FORMAT sFormat);
    // DEVICE_RETURN_CODE_T device_capturestreamshot(int deviceID, DEVICE_TYPE_T devType);
};

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SRC_SERVICE_COMMAND_MANAGER_H_*/
