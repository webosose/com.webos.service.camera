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
#include "udev_client.h"
#include "camera_hal_types.h"
#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

UDEVClient::UDEVClient() : cam_info_(NULL), camudev_(NULL), tid_(0) {}

UDEVClient::~UDEVClient()
{
  if (NULL != cam_info_)
    delete[] cam_info_;

  if (NULL != camudev_)
    udev_unref(camudev_);
}

int UDEVClient::subscribeToClient(udevhandler_cb cb)
{
  DLOG_SDK(std::cout << "subscribeToClient UDEV" << std::endl;);
  subscribeToDeviceState_ = cb;

  int retval = udevCreate();
  if (retval != CAMERA_ERROR_NONE)
  {
    DLOG_SDK(std::cout << "udevCreate failed" << std::endl;);
    return CAMERA_ERROR_UNKNOWN;
  }

  cam_info_ = new (std::nothrow) camera_details_t[MAX_DEVICE_COUNT];

  retval = pthread_create(&tid_, NULL, runMonitorDeviceThread, this);
  if (retval != CAMERA_ERROR_NONE)
  {
    DLOG_SDK(std::cout << "pthread_create failed" << std::endl;);
    return CAMERA_ERROR_UNKNOWN;
  }

  return CAMERA_ERROR_NONE;
}

int UDEVClient::udevCreate()
{
  camudev_ = udev_new();
  if (!camudev_)
  {
    DLOG_SDK(std::cout << "udevCreate : Fail to create udev" << std::endl;);
    return CAMERA_ERROR_UNKNOWN;
  }

  return CAMERA_ERROR_NONE;
}

void *UDEVClient::runMonitorDeviceThread(void *arg)
{
  UDEVClient *ptr = reinterpret_cast<UDEVClient *>(arg);
  ptr->monitorDevice();
  return 0;
}

void UDEVClient::monitorDevice()
{
  struct udev_monitor *monitor = NULL;
  monitor = udev_monitor_new_from_netlink(camudev_, "udev");
  udev_monitor_filter_add_match_subsystem_devtype(monitor, "video4linux", NULL);
  udev_monitor_enable_receiving(monitor);
  int fd = udev_monitor_get_fd(monitor);
  int nCamCount = 0;

  while (1)
  {
    fd_set fds;
    struct timeval tv;
    int ret;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    ret = select(fd + 1, &fds, NULL, NULL, &tv);
    if (ret > 0 && FD_ISSET(fd, &fds))
    {
      struct udev_device *dev;
      dev = udev_monitor_receive_device(monitor);
      if (dev)
      {
        getDevice(dev, &cam_info_[nCamCount]);
        if (NULL != subscribeToDeviceState_)
          subscribeToDeviceState_(&cam_info_[nCamCount]);
        nCamCount++;

        udev_device_unref(dev);
      }
      else
      {
        DLOG_SDK(std::cout << "No Device from receive_device(). An error occured" << std::endl;);
      }
    }
  }
  return;
}

void UDEVClient::getDevice(struct udev_device *device, camera_details_t *caminfo)
{
  const char *str;
  dev_t devnum;
  int count;
  struct udev_list_entry *list_entry;

  DLOG_SDK(std::cout << "*** device:  ***" << device << std::endl;);
  str = udev_device_get_action(device);
  if (str != NULL)
  {
    DLOG_SDK(std::cout << "action:    " << str << std::endl;);
    if (strcmp(str, "remove") == 0)
      caminfo->cam_state = DEV_EVENT_STATE_UNPLUGGED;
    else if (strcmp(str, "add") == 0)
      caminfo->cam_state = DEV_EVENT_STATE_PLUGGED;
  }

  str = udev_device_get_subsystem(device);
  if (str != NULL)
  {
    DLOG_SDK(std::cout << "subsystem: " << str << std::endl;);
    caminfo->device_subtype = str;
  }

  str = udev_device_get_devtype(device);
  if (str != NULL)
  {
    DLOG_SDK(std::cout << "devtype: " << str << std::endl;);
    caminfo->device_type = str;
  }

  str = udev_device_get_devnode(device);
  if (str != NULL)
  {
    DLOG_SDK(std::cout << "devname: " << str << std::endl;);
    caminfo->device_node = str;
  }

  devnum = udev_device_get_devnum(device);
  if (major(devnum) > 0)
  {
    DLOG_SDK(std::cout << "minor devnum:  " << minor(devnum) << std::endl;);
    DLOG_SDK(std::cout << "major devnum:  " << major(devnum) << std::endl;);
    caminfo->device_num = minor(devnum);
  }

  count = 0;
  udev_list_entry_foreach(list_entry, udev_device_get_properties_list_entry(device))
  {
    if (strcmp(udev_list_entry_get_name(list_entry), "ID_SERIAL") == 0)
    {
      caminfo->serial_number = udev_list_entry_get_value(list_entry);
      DLOG_SDK(std::cout << "serial number: " << caminfo->serial_number << std::endl;);
    }
    else if (strcmp(udev_list_entry_get_name(list_entry), "ID_VENDOR") == 0)
    {
      caminfo->vendor_name = udev_list_entry_get_value(list_entry);
      DLOG_SDK(std::cout << "vendor name: " << caminfo->vendor_name << std::endl;);
    }
    else if (strcmp(udev_list_entry_get_name(list_entry), "PRODUCT") == 0)
    {
      caminfo->product_name = udev_list_entry_get_value(list_entry);
      DLOG_SDK(std::cout << "product name: " << caminfo->product_name << std::endl;);
    }
    count++;
  }
}
