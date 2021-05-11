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

#include "notifier.h"

DEVICE_LIST_T st_dev_info_[MAX_DEVICE_COUNT];

static void updateDeviceList(DEVICE_LIST_T *st_dev_list)
{
  PMLOG_INFO(CONST_MODULE_LUNA, "updateDeviceList callback received\n");
  for (int i = 0; i < MAX_DEVICE_COUNT; i++)
  {
    strncpy(st_dev_info_[i].strVendorName, st_dev_list[i].strVendorName,
            CONST_MAX_STRING_LENGTH - 1);
    st_dev_info_[i].strVendorName[CONST_MAX_STRING_LENGTH - 1] = '\0';

    strncpy(st_dev_info_[i].strProductName, st_dev_list[i].strProductName,
            CONST_MAX_STRING_LENGTH - 1);
    st_dev_info_[i].strProductName[CONST_MAX_STRING_LENGTH - 1] = '\0';

    strncpy(st_dev_info_[i].strDeviceType, st_dev_list[i].strDeviceType,
            CONST_MAX_STRING_LENGTH - 1);
    st_dev_info_[i].strDeviceType[CONST_MAX_STRING_LENGTH - 1] = '\0';

    strncpy(st_dev_info_[i].strDeviceSubtype, st_dev_list[i].strDeviceSubtype,
            CONST_MAX_STRING_LENGTH - 1);
    st_dev_info_[i].strDeviceSubtype[CONST_MAX_STRING_LENGTH - 1] = '\0';

    st_dev_info_[i].nDeviceNum = st_dev_list[i].nDeviceNum;
    PMLOG_INFO(CONST_MODULE_LUNA, "updateDeviceList device_num[%d] : %d \n", i,
               st_dev_info_[i].nDeviceNum);

    st_dev_info_[i].nPortNum = st_dev_list[i].nPortNum;
    PMLOG_INFO(CONST_MODULE_LUNA, "updateDeviceList port_num[%d] : %d \n", i,
               st_dev_info_[i].nPortNum);

    st_dev_info_[i].isPowerOnConnect = st_dev_list[i].isPowerOnConnect;
    PMLOG_INFO(CONST_MODULE_LUNA, "updateDeviceList power_status[%d] : %d \n", i,
               st_dev_info_[i].isPowerOnConnect);
  }
}

void Notifier::addNotifier(NotifierClient client, GMainLoop *loop)
{
  PMLOG_INFO(CONST_MODULE_LUNA, "addNotifier client : %d\n", (int)client);

  if (client == NotifierClient::NOTIFIER_CLIENT_PDM)
  {
    p_client_notifier_ = &pdm_; // points to PDM object
    if (nullptr != p_client_notifier_)
    {
      p_client_notifier_->setLSHandle(lshandle_);
      registerCallback(updateDeviceList, loop);
    }
  }
}

void Notifier::registerCallback(handlercb deviceinfo, GMainLoop *loop)
{
  if (nullptr != p_client_notifier_)
        p_client_notifier_->subscribeToClient(deviceinfo, loop);
}

void Notifier::setLSHandle(LSHandle *handle) { lshandle_ = handle; }
