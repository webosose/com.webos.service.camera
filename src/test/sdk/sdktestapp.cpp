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

#include <string.h>
#include <iostream>
#include <new>
#include <unistd.h>
#include "camera.h"
#include "notifier.h"

const std::string subsystem = "libv4l2-camera-plugin.so";
const std::string devname = "/dev/video0";
const int nconnect = 1;
const int ndisconnect = 2;

int Disconnect()
{
    DLOG_SDK(std::cout << "Disconnect" << std::endl;)
}

int Connect()
{
    DLOG_SDK(std::cout << "Connect" << std::endl;)
}

void handleDeviceState(camera_info_t *pcam_info)
{
    DLOG_SDK(std::cout << "handleDeviceState callback received" << std::endl;)
    if(NULL != pcam_info)
    {
        for(int i = 0; i < MAX_DEVICE_COUNT ; i++)
        {
          if( (nconnect == pcam_info[i].cam_state) ||
             (ndisconnect == pcam_info[i].cam_state) )
          {
            DLOG_SDK(std::cout << "cam_info cam_state: " << pcam_info[i].cam_state << std::endl;);
            DLOG_SDK(std::cout << "cam_info device_subtype: " << pcam_info[i].device_subtype << std::endl;);
            DLOG_SDK(std::cout << "cam_info device_num: " << pcam_info[i].device_num << std::endl;);
            DLOG_SDK(std::cout << "cam_info device_node: " << pcam_info[i].device_node << std::endl;);
            DLOG_SDK(std::cout << "cam_info vendor_name: " << pcam_info[i].vendor_name << std::endl;);
            DLOG_SDK(std::cout << "cam_info serial_number: " << pcam_info[i].serial_number << std::endl;);
          }
        }
    }
}

int main(int argc, char* argv[])
{
    opterr = 0;
    int c;
    std::string option;

    while ((c = getopt (argc, argv, "o:")) != -1)
    {
        switch (c)
        {
        case 'o':
            if(optarg)
                option = optarg;
            break;
        case '?':
            if (optopt == 'o')
                std::cout << "Option -%o requires an argument" << std::endl;
            else if (isprint (optopt))
                std::cout << "Unknown option" << std::endl;
            else
                std::cout << "Unknown option character" << std::endl;
            return 0;
        }
    }

    Camera *camera = new (std::nothrow) Camera;
    Notifier *notifier = new (std::nothrow) Notifier;
    int retval = 0;

    if (nullptr != notifier)
    {
        if ("PDM" == option)
            retval = notifier->addNotifier(NOTIFIER_CLIENT_PDM);
        else
            retval = notifier->addNotifier(NOTIFIER_CLIENT_UDEV);

        notifier->registerCallback(handleDeviceState);
    }

    if (nullptr != camera)
    {
        retval = camera->init(subsystem);
        retval = camera->addCallbacks(CAMERA_MSG_OPEN,Connect);
        retval = camera->addCallbacks(CAMERA_MSG_CLOSE,Disconnect);

        sleep(10);

        retval = camera->open(devname);
        retval = camera->removeCallbacks(CAMERA_MSG_CLOSE);
        retval = camera->close();
        retval = camera->deinit();
        delete camera;
    }

    if(nullptr != notifier)
        delete notifier;

    return 0;
}
