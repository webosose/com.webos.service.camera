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

void Notifier::addNotifier(NotifierClient client, GMainLoop *loop)
{
    PMLOG_INFO(CONST_MODULE_NOTIFIER, "client : %d\n", (int)client);

    if (client == NotifierClient::NOTIFIER_CLIENT_PDM)
    {
        p_client_notifier_ = &pdm_; // points to PDM object
        if (nullptr != p_client_notifier_)
        {
            p_client_notifier_->setLSHandle(lshandle_);
            registerCallback(NULL, loop);
        }
    }
    else if (client == NotifierClient::NOTIFIER_CLIENT_APPCAST)
    {
        p_client_notifier_ = &appcast_; // points to appcast object
        if (nullptr != p_client_notifier_)
        {
            p_client_notifier_->setLSHandle(lshandle_);
            registerCallback(NULL, loop);
        }
    }
}

void Notifier::registerCallback(DeviceNotifier::handlercb deviceinfo, GMainLoop *loop)
{
    if (nullptr != p_client_notifier_)
        p_client_notifier_->subscribeToClient(deviceinfo, loop);
}

void Notifier::setLSHandle(LSHandle *handle) { lshandle_ = handle; }
