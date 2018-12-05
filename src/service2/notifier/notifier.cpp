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

#include "notifier.h"

device_info_t st_dev_info_[MAX_DEVICE_COUNT];

static void updateDeviceList(device_info_t *st_dev_list)
{
    SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList callback received\n");
    for (int i = 0; i < MAX_DEVICE_COUNT; i++)
    {
        st_dev_info_[i].vendor_name = st_dev_list[i].vendor_name;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList vendor_name[%d] : %s \n", i, st_dev_info_[i].vendor_name);

        st_dev_info_[i].product_name = st_dev_list[i].product_name;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList product_name[%d] : %s \n", i, st_dev_info_[i].product_name);

        st_dev_info_[i].serial_number = st_dev_list[i].serial_number;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList serial_number[%d] : %s \n", i, st_dev_info_[i].serial_number);

        st_dev_info_[i].device_subtype = st_dev_list[i].device_subtype;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList device_subtype[%d] : %s \n", i, st_dev_info_[i].device_subtype);

        st_dev_info_[i].device_type = st_dev_list[i].device_type;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList device_type[%d] : %s \n", i, st_dev_info_[i].device_type);

        st_dev_info_[i].device_node = st_dev_list[i].device_node;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList device_node[%d] : %s \n", i, st_dev_info_[i].device_node);

        st_dev_info_[i].device_num = st_dev_list[i].device_num;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList device_num[%d] : %d \n", i, st_dev_info_[i].device_num);

        st_dev_info_[i].port_num = st_dev_list[i].port_num;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList port_num[%d] : %d \n", i, st_dev_info_[i].port_num);

        st_dev_info_[i].device_state = st_dev_list[i].device_state;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList device_state[%d] : %d \n", i, (int)st_dev_info_[i].device_state);

        st_dev_info_[i].power_status = st_dev_list[i].power_status;
        SRV_LOG_INFO(CONST_MODULE_LUNA, "updateDeviceList power_status[%d] : %d \n", i, st_dev_info_[i].power_status);
    }
}

void Notifier::addNotifier(NotifierClient client)
{
    SRV_LOG_INFO(CONST_MODULE_LUNA, "addNotifier client : %d\n", (int)client);

    if (client == NotifierClient::NOTIFIER_CLIENT_PDM)
    {
        p_client_notifier_ = &pdm_; //points to PDM object
        if (nullptr != p_client_notifier_)
        {
            p_client_notifier_->setLSHandle(lshandle_);
            registerCallback(updateDeviceList);
        }
    }
    else if (client == NotifierClient::NOTIFIER_CLIENT_UDEV)
    {
        //points to UDEV object
    }
}

void Notifier::registerCallback(handlercb deviceinfo)
{
    if (nullptr != p_client_notifier_)
        p_client_notifier_->subscribeToClient(deviceinfo);
}

void Notifier::setLSHandle(LSHandle *handle)
{
    lshandle_ = handle;
}

int Notifier::getDeviceInfo(int dev_num, device_info_t *pst_info)
{
    int count = 0;

    for (count = 0; count < MAX_DEVICE_COUNT; count++)
    {
        if (dev_num == st_dev_info_[count].device_num)
            break;
    }
    pst_info->vendor_name = st_dev_info_[count].vendor_name;
    pst_info->product_name = st_dev_info_[count].product_name;
    pst_info->serial_number = st_dev_info_[count].serial_number;
    pst_info->device_subtype = st_dev_info_[count].device_subtype;
    pst_info->device_type = st_dev_info_[count].device_type;
    pst_info->device_node = st_dev_info_[count].device_node;
    pst_info->device_num = st_dev_info_[count].device_num;
    pst_info->port_num = st_dev_info_[count].port_num;
    pst_info->device_state = st_dev_info_[count].device_state;

    return 0;
}
