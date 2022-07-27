// Copyright (c) 2019 LG Electronics, Inc.
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

#include "ipcshm_func.h"
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string>
#include <unistd.h>

bool service_is_abnormal = false;

std::string getCommandOutput(std::string cmd)
{
    FILE *fp;
    std::string str = "";
    char path[1035];
    fp = popen(cmd.c_str(), "r");
    if (fp == NULL)
    {
        std::cout << "Failed to run command" << std::endl;
        ;
        exit(1);
    }
    while (fgets(path, sizeof(path) - 1, fp) != NULL)
    {
        str = str + path;
    }
    pclose(fp);
    return str;
}

std::string getStringFromNumber(int number)
{
    std::ostringstream str1;
    str1 << number;
    return str1.str();
}

std::string cmd_getcameralist =
    "luna-send -n 1 -f luna://com.webos.service.camera2/getCameraList '{}'";
std::string cmd_getinfo = "luna-send -n 1 -f luna://com.webos.service.camera2/getInfo '{\"id\": \"";
std::string cmd_open    = "luna-send -n 1 -f luna://com.webos.service.camera2/open '{\"id\": \"";
std::string cmd_setformat = "luna-send -n 1 -f "
                            "luna://com.webos.service.camera2/setFormat "
                            "'{\"handle\":";
std::string cmd_startpreview = "luna-send -n 1 -f "
                               "luna://com.webos.service.camera2/startPreview "
                               "'{\"handle\":";
std::string cmd_stoppreview = "luna-send -n 1 -f "
                              "luna://com.webos.service.camera2/stopPreview "
                              "'{\"handle\":";
std::string cmd_close = "luna-send -n 1 -f luna://com.webos.service.camera2/close '{\"handle\":";

int main(int argc, char const *argv[])
{
    std::string id;
    pid_t pid = (pid_t)-1;
    int sig;
    int handle;
    std::string output;

    std::cout << "Enter signal number" << std::endl;
    std::cin >> sig;

    sigset_t set;
    int signum;
    sigemptyset(&set);
    sigaddset(&set, sig);
    sigprocmask(SIG_SETMASK, &set, NULL);

    // getCameraList
    std::cout << cmd_getcameralist << std::endl;
    output = getCommandOutput(cmd_getcameralist);
    std::cout << output << std::endl;

    // getInfo
    std::cout << "Enter camera id received in getCameraList API" << std::endl;
    std::cin >> id;
    cmd_getinfo = cmd_getinfo + id + "\"}'";
    std::cout << cmd_getinfo << std::endl;
    output = getCommandOutput(cmd_getinfo);
    std::cout << output << std::endl;

    // open API
    std::cout << "Enter camera id received in getCameraList API" << std::endl;
    std::cin >> id;
    pid      = getpid();
    cmd_open = cmd_open + id + "\", \"pid\": " + getStringFromNumber(pid) +
               ", \"sig\": " + getStringFromNumber(sig) + " }'";
    std::cout << cmd_open << std::endl;
    output = getCommandOutput(cmd_open);
    std::cout << output << std::endl;

    // need handle to continue
    std::cout << "Enter handle to continue : " << std::endl;
    std::cin >> handle;

    // setFormat
    std::cout << "Enter width : " << std::endl;
    int width;
    std::cin >> width;
    std::cout << "Enter height : " << std::endl;
    int height;
    std::cin >> height;
    std::cout << "Enter format : " << std::endl;
    std::string format;
    std::cin >> format;
    int fps;
    std::cout << "Enter fps : " << std::endl;
    std::cin >> fps;
    cmd_setformat = cmd_setformat + getStringFromNumber(handle) +
                    ",\"params\":{\"width\":" + getStringFromNumber(width) +
                    ",\"height\":" + getStringFromNumber(height) + ",\"format\":\"" + format +
                    "\"" + ",\"fps\":" + getStringFromNumber(fps) + "}}'";
    std::cout << cmd_setformat << std::endl;
    output = getCommandOutput(cmd_setformat);
    std::cout << output << std::endl;

    // startPreview
    cmd_startpreview = cmd_startpreview + getStringFromNumber(handle) +
                       ", \"params\": {\"type\":\"sharedmemory\",\"source\":\"0\"}}'";
    std::cout << cmd_startpreview << std::endl;
    output = getCommandOutput(cmd_startpreview);
    std::cout << output << std::endl;

    // start of synchronization test
    struct timespec timeout;
    timeout.tv_sec  = 10;
    timeout.tv_nsec = 0;

    key_t smkey;
    SHMEM_HANDLE hShmem;
    SHMEM_STATUS_T smResult;

    // open shared memory
    std::cout << "Enter handle to shared memory key : " << std::endl;
    std::cin >> smkey;
    smResult = OpenShmem(&hShmem, smkey);

    if (smResult != SHMEM_COMM_OK)
    {
        std::cout << "shared memory open failed!!" << std::endl;
    }
    else
    {
        std::cout << "shared memory open success!!" << std::endl;

        // test for 300 frames
        for (int i = 0; i < 300; i++)
        {
            std::cout << "[i: " + std::to_string(i) + "] waiting for signal ...... ";
            signum = sigtimedwait(&set, NULL, &timeout);
            if (signum != -1)
            {
                std::cout << "received" << std::endl;
                // Read Shared Memeory here and do somothing
                unsigned char *databuf = 0;
                int datasize           = 0;
                smResult               = ReadShmem(hShmem, &databuf, &datasize);
                if (smResult != SHMEM_COMM_OK)
                {
                    std::cout << "fail to read shared memory" << std::endl;
                }
                else
                {
                    std::cout << "read data size : " << std::to_string(datasize) + " Bytes"
                              << std::endl;
                }
            }
            else
            {
                service_is_abnormal = true;
                std::cout << "timeout (10sec)" << std::endl;
                std::cout << "server probably dead." << std::endl;
                std::cout << "app termination sequence starts ..." << std::endl;
                break;
            }
        }

        smResult = CloseShmem(&hShmem);
        if (smResult != SHMEM_COMM_OK)
        {
            std::cout << "fail to close shared memory" << std::endl;
        }
        else
        {
            std::cout << "successfully closed shared memory" << std::endl;
        }
    }
    // end of synchronization test

    if (service_is_abnormal == false)
    {
        // stopPreview
        cmd_stoppreview = cmd_stoppreview + getStringFromNumber(handle) + "}'";
        std::cout << cmd_stoppreview << std::endl;
        output = getCommandOutput(cmd_stoppreview);
        std::cout << output << std::endl;

        // close
        cmd_close = cmd_close + getStringFromNumber(handle) +
                    ", \"pid\": " + getStringFromNumber(pid) + "}'";
        std::cout << cmd_close << std::endl;
        output = getCommandOutput(cmd_close);
        std::cout << output << std::endl;
    }

    return 0;
}
