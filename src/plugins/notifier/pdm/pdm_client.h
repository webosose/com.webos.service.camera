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

#pragma once

#include "camera/luna_client.h"
#include "plugin_interface.hpp"

class PDMClient : public INotifier
{
public:
    PDMClient();
    virtual ~PDMClient();

public:
    virtual bool queryInterface(const char *szName, void **pInterface) override
    {
        *pInterface = static_cast<void *>(static_cast<INotifier *>(this));
        return true;
    }

    virtual void subscribeToClient(handlercb cb, void *mainLoop) override;
    virtual void setLSHandle(void *lshandle) override;

private:
    bool registerToServiceCallback(const char *serviceName, bool connected);
    bool getDeviceListCallback(const char *message);

    handlercb updateDeviceList{nullptr};

    std::unique_ptr<LunaClient> lunaClient_;
    unsigned long subscribeKey_{0};
};
