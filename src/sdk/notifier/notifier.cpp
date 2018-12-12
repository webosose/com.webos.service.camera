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

/******************************************************************************
 File Inclusions
 ******************************************************************************/
 #include "notifier.h"
 #include "camera_hal_types.h"
 #include <string.h>

int Notifier::addNotifier(notifier_client_t client)
{
    DLOG_SDK(std::cout << "addNotifier client : " << client << std::endl;);

    if(client == NOTIFIER_CLIENT_PDM)
    {
        p_client_notifier_ = &pdm_;//points to PDM object
    }
    else if(client == NOTIFIER_CLIENT_UDEV)
    {
        p_client_notifier_ = &udev_;//points to UDEV object
    }

    return CAMERA_ERROR_NONE;
}

int Notifier::registerCallback(handler_cb_ camera_info)
{
    int retval = CAMERA_ERROR_NONE;

    if (nullptr != p_client_notifier_)
        retval = p_client_notifier_->subscribeToClient(camera_info);
    else
        retval = CAMERA_ERROR_UNKNOWN;

    return retval;
}

int Notifier::updateDeviceList(std::string dev_type,int dev_count,camera_info_t *st_dev_list)
{
    device_event_state_t nstatus = DEVICE_EVENT_NONE;

    if(dev_count_ < dev_count)
        nstatus = DEVICE_EVENT_STATE_PLUGGED;
    else if(dev_count_ > dev_count)
        nstatus = DEVICE_EVENT_STATE_UNPLUGGED;
    else
        DLOG_SDK(std::cout <<"No event changed!!" << std::endl;);

    if(dev_count_ != dev_count)
    {
        DLOG_SDK(std::cout <<"Update device info!!" << std::endl;);
        for (int i = 0; i < dev_count_; i++)
        {
             st_dev_info_[i].vendor_name = st_dev_list[i].vendor_name;
             st_dev_info_[i].product_name = st_dev_list[i].product_name;
             st_dev_info_[i].serial_number = st_dev_list[i].serial_number;
             st_dev_info_[i].device_subtype = st_dev_list[i].device_subtype;
             st_dev_info_[i].device_type = st_dev_list[i].device_type;
             st_dev_info_[i].device_node = st_dev_list[i].device_node;
             st_dev_info_[i].device_num = st_dev_list[i].device_num;
             st_dev_info_[i].port_num = st_dev_list[i].port_num;
             st_dev_info_[i].cam_state = nstatus;
        }
        dev_count_ = dev_count;
    }

    return CAMERA_ERROR_NONE;
}

int Notifier::getDeviceInfo(int dev_num,camera_info_t *pst_info)
{
    int count = 0;

    for(count=0; count < dev_count_; count++)
    {
        if(dev_num == st_dev_info_[count].device_num)
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
    pst_info->cam_state = st_dev_info_[count].cam_state;

    return CAMERA_ERROR_NONE;
}

