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
#include "camera.h"
#include <new>

const std::string subsystem = "libv4l2-camera-plugin.so";
const std::string devname = "/dev/video0";

int Disconnect()
{
    DLOG_SDK(std::cout << "Disconnect" << std::endl;)
}

int Connect()
{
    DLOG_SDK(std::cout << "Connect" << std::endl;)
}

int main(int argc, char const *argv[])
{
    int retval = 0;
    Camera *camera = new (std::nothrow) Camera;

    if(nullptr != camera)
    {
        retval = camera->init(subsystem);
        retval = camera->addCallbacks(CAMERA_MSG_OPEN,Connect);
        retval = camera->addCallbacks(CAMERA_MSG_CLOSE,Disconnect);

        retval = camera->open(devname);
        retval = camera->removeCallbacks(CAMERA_MSG_CLOSE);
        retval = camera->close();
        retval = camera->deinit();

        delete camera;
    }
    return 0;
}

