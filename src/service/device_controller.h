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

#ifndef SRC_SERVICE_DEVICE_CONTROLLER_H_
#define SRC_SERVICE_DEVICE_CONTROLLER_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /*-----------------------------------------------------------------------------
    (File Inclusions)
------------------------------------------------------------------------------*/

#include "service_main.h"
//#include"ose_commandmanager.h"
#include "camera_types.h"
#include "constants.h"

class DeviceControl
{

      private:
        static DeviceControl *devctlinstance;
        DeviceControl() {}

      public:
        static DeviceControl *getInstance()
        {
            if (devctlinstance == 0)
            {
                devctlinstance = new DeviceControl();
            }

            return devctlinstance;
        };

        //Open the Device and return the handle
        DEVICE_RETURN_CODE_T open(DEVICE_HANDLE devHandle,DEVICE_TYPE devType);
        //Close the device
        DEVICE_RETURN_CODE_T close(DEVICE_HANDLE devHandle,DEVICE_TYPE devType);
        //Start Capture
        DEVICE_RETURN_CODE_T startPreview(DEVICE_HANDLE devHandle,DEVICE_TYPE devType,int pKey);
        //Stop Capture
        DEVICE_RETURN_CODE_T stopPreview(DEVICE_HANDLE devHandle,DEVICE_TYPE devType);
        DEVICE_RETURN_CODE_T startCapture(DEVICE_HANDLE devHandle,DEVICE_TYPE devType,FORMAT sFormat);
        //Stop Capture
        DEVICE_RETURN_CODE_T stopCapture(DEVICE_HANDLE devHandle,DEVICE_TYPE devType);
        DEVICE_RETURN_CODE_T captureImage(DEVICE_HANDLE devHandle,DEVICE_TYPE devType,int nCount,FORMAT sFormat);
        bool isUpdatedCameraList();

        DEVICE_RETURN_CODE_T createHandle(DEVICE_LIST_T sDeviceInfo,DEVICE_HANDLE *sDevHandle);

        DEVICE_RETURN_CODE_T getDeviceInfo(DEVICE_HANDLE devHandle,DEVICE_TYPE devType, CAMERA_INFO_T *pInfo);
        DEVICE_RETURN_CODE_T getDeviceList(DEVICE_LIST_T *pList,int *pCamDev, int *pMicDev, int *pCamSupport, int *pMicSupport,int devCount);
        DEVICE_RETURN_CODE_T getDeviceProperty(DEVICE_HANDLE devHandle,DEVICE_TYPE devType, CAMERA_PROPERTIES_T *oParams);
        DEVICE_RETURN_CODE_T setDeviceProperty(DEVICE_HANDLE devHandle,DEVICE_TYPE devType, CAMERA_PROPERTIES_T *oParams);
  };

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SRC_SERVICE_DEVICE_CONTROLLER_H_*/
