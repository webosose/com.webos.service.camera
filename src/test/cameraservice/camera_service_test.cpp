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

#include <iostream>
#include <sstream>
#include <string>

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

int main(int argc, char const *argv[])
{
    int choice = -1;

testcase:

    std::string getcameralist =
        "luna-send -n 1 -f luna://com.webos.service.camera2/getCameraList '{}'";
    std::string getinfo = "luna-send -n 1 -f "
                          "luna://com.webos.service.camera2/getInfo '{\"id\": "
                          "\"";
    std::string open  = "luna-send -n 1 -f luna://com.webos.service.camera2/open '{\"id\": \"";
    std::string close = "luna-send -n 1 -f "
                        "luna://com.webos.service.camera2/close '{\"handle\":";
    std::string getproperties = "luna-send -n 1 -f "
                                "luna://com.webos.service.camera2/"
                                "getProperties '{\"handle\":";
    std::string setproperties = "luna-send -n 1 -f "
                                "luna://com.webos.service.camera2/"
                                "setProperties '{\"handle\":";
    std::string setformat = "luna-send -n 1 -f "
                            "luna://com.webos.service.camera2/setFormat "
                            "'{\"handle\":";
    std::string startpreview = "luna-send -n 1 -f "
                               "luna://com.webos.service.camera2/startPreview "
                               "'{\"handle\":";
    std::string stoppreview = "luna-send -n 1 -f "
                              "luna://com.webos.service.camera2/stopPreview "
                              "'{\"handle\":";
    std::string startcapture = "luna-send -n 1 -f "
                               "luna://com.webos.service.camera2/startCapture "
                               "'{\"handle\":";
    std::string stopcapture = "luna-send -n 1 -f "
                              "luna://com.webos.service.camera2/stopCapture "
                              "'{\"handle\":";

    std::cout << "List of test cases : " << std::endl;
    std::cout << "1. GetCameraList" << std::endl << "2. GetInfo" << std::endl;
    std::cout << "3. Open" << std::endl << "4. close" << std::endl;
    std::cout << "5. GetProperties" << std::endl << "6. SetProperties" << std::endl;
    std::cout << "7. SetFormat" << std::endl;
    std::cout << "8. StartPreview" << std::endl << "9. StopPreview" << std::endl;
    std::cout << "10. StartCapture" << std::endl << "11. StopCapture" << std::endl;
    std::cout << "Enter choice" << std::endl;

    std::cin >> choice;

    if (choice == 1)
    {
        std::cout << getcameralist << std::endl;
        std::string output = getCommandOutput(getcameralist);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 2)
    {
        std::cout << "Enter camera id received in getCameraList API" << std::endl;
        std::string id;
        std::cin >> id;
        getinfo = getinfo + id + "\"}'";
        std::cout << getinfo << std::endl;
        std::string output = getCommandOutput(getinfo);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 3)
    {
        std::cout << "Enter camera id received in getCameraList API" << std::endl;
        std::string id;
        std::cin >> id;
        open = open + id + "\"}'";
        std::cout << open << std::endl;
        std::string output = getCommandOutput(open);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 4)
    {
        int handle;
        std::cout << "Enter handle to close : " << std::endl;
        std::cin >> handle;
        close = close + getStringFromNumber(handle) + "}'";
        std::cout << close << std::endl;
        std::string output = getCommandOutput(close);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 5)
    {
        int handle;
        std::cout << "Enter handle to getProperties : " << std::endl;
        std::cin >> handle;
        getproperties = getproperties + getStringFromNumber(handle) + "}'";
        std::cout << getproperties << std::endl;
        std::string output = getCommandOutput(getproperties);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 6)
    {
        int handle;
        std::cout << "Enter handle to set properties of device : " << std::endl;
        std::cin >> handle;
        setproperties =
            setproperties + getStringFromNumber(handle) + ",\"params\":{\"contrast\": 100}}'";
        std::cout << setproperties << std::endl;
        std::string output = getCommandOutput(setproperties);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 7)
    {
        int handle;
        std::cout << "Enter handle to setformat : " << std::endl;
        std::cin >> handle;
        std::cout << "Enter width : " << std::endl;
        int width;
        std::cin >> width;
        std::cout << "Enter height : " << std::endl;
        int height;
        std::cin >> height;
        std::cout << "Enter format : " << std::endl;
        std::string format;
        std::cin >> format;
        setformat = setformat + getStringFromNumber(handle) +
                    ",\"params\":{\"width\":" + getStringFromNumber(width) +
                    ",\"height\":" + getStringFromNumber(height) + ",\"format\":\"" + format +
                    "\"}}'";
        std::cout << setformat << std::endl;
        std::string output = getCommandOutput(setformat);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 8)
    {
        int handle;
        std::cout << "Enter handle to start preview : " << std::endl;
        std::cin >> handle;
        startpreview = startpreview + getStringFromNumber(handle) +
                       ", \"params\": {\"type\":\"sharedmemory\",\"source\":\"0\"}}'";
        std::cout << startpreview << std::endl;
        std::string output = getCommandOutput(startpreview);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 9)
    {
        int handle;
        std::cout << "Enter handle to stop preview: " << std::endl;
        std::cin >> handle;
        stoppreview = stoppreview + getStringFromNumber(handle) + "}'";
        std::cout << stoppreview << std::endl;
        std::string output = getCommandOutput(stoppreview);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 10)
    {
        int handle;
        std::cout << "Enter handle to start capture : " << std::endl;
        std::cin >> handle;
        std::cout << "Enter width : " << std::endl;
        int width;
        std::cin >> width;
        std::cout << "Enter height : " << std::endl;
        int height;
        std::cin >> height;
        std::cout << "Enter format : " << std::endl;
        std::string format;
        std::cin >> format;
        std::cout << "Enter capture mode : " << std::endl;
        std::string mode;
        std::cin >> mode;
        int nimage = 0;
        if (mode == "MODE_BURST")
        {
            std::cout << "Enter no of images to be captured : " << std::endl;
            std::cin >> nimage;
        }
        startcapture = startcapture + getStringFromNumber(handle) +
                       ",\"params\":{\"width\":" + getStringFromNumber(width) +
                       ",\"height\":" + getStringFromNumber(height) + ",\"format\": \"" + format +
                       "\",\"mode\":\"" + mode + "\",\"nimage\":" + getStringFromNumber(nimage) +
                       "}}'";
        std::cout << startcapture << std::endl;
        std::string output = getCommandOutput(startcapture);
        std::cout << output << std::endl;
        goto testcase;
    }
    else if (choice == 11)
    {
        int handle;
        std::cout << "Enter handle to stop capture: " << std::endl;
        std::cin >> handle;
        stopcapture = stopcapture + getStringFromNumber(handle) + "}'";
        std::cout << stopcapture << std::endl;
        std::string output = getCommandOutput(stopcapture);
        std::cout << output << std::endl;
        goto testcase;
    }
    else
        std::cout << "Wrong choice : exiting testapp" << std::endl;

    return 0;
}
