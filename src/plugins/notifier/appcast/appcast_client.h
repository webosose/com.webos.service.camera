// Copyright (c) 2022 LG Electronics, Inc.
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

#ifndef APPCAST_CLIENT
#define APPCAST_CLIENT

#include "plugin_interface.hpp"
#include <functional>
#include <luna-service2/lunaservice.hpp>

enum APP_CAST_STATE
{
    INIT  = 0,
    READY = 1,
    PLAY  = 2,

    STATE_END // must be last - used to validate app cast state
};

struct deviceInfo_t
{
    std::string clientKey;
    std::string version;
    std::string type;
    std::string platform;
    std::string manufacturer;
    std::string modelName;
    std::string deviceName;
};

#ifdef __cplusplus
extern "C"
{
#endif

    class AppCastClient : public INotifier
    {
    private:
        static bool subscribeToAppcastService(LSHandle *sh, const char *serviceName, bool connected,
                                              void *ctx);
        LSHandle *lshandle_;

    public:
        AppCastClient();
        virtual ~AppCastClient();

    public:
        virtual bool queryInterface(const char *szName, void **pInterface) override
        {
            *pInterface = static_cast<void *>(static_cast<INotifier *>(this));
            return true;
        }

        virtual void subscribeToClient(handlercb cb, void *mainLoop) override;
        virtual void setLSHandle(void *lshandle) override;

    public:
        void setState(APP_CAST_STATE);
        bool sendConnectSoundInput(bool);
        bool sendSetSoundInput(bool);

        APP_CAST_STATE mState;
        std::string str_state[STATE_END];

        deviceInfo_t mDeviceInfo;

    public:
        handlercb updateDeviceList;
    };

#ifdef __cplusplus
}
#endif

#endif
