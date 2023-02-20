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
#include "device_manager.h"

static bool updateDeviceListCb(std::string deviceType, const void *deviceList)
{
    return DeviceManager::getInstance().updateDeviceList(
        deviceType, *((std::vector<DEVICE_LIST_T> *)deviceList));
}

Notifier::~Notifier()
{
    if (pPluginFactory_)
    {
        delete pPluginFactory_;
        pPluginFactory_ = nullptr;
    }
}

void Notifier::addNotifier(NotifierClient client, GMainLoop *loop)
{
    PMLOG_INFO(CONST_MODULE_NOTIFIER, "client : %d\n", (int)client);

    if (client == NotifierClient::NOTIFIER_CLIENT_PDM)
    {
        IFeaturePtr pFeature = pPluginFactory_->createFeature("pdm");
        if (pFeature)
        {
            void *pInterface = nullptr;
            if (pFeature->queryInterface("pdm", &pInterface))
            {
                pFeatureList_.push_back(std::move(pFeature));
                notifierMap_["pdm"] = static_cast<INotifier *>(pInterface);
                INotifier *pdm      = notifierMap_["pdm"]; // points to PDM object
                pdm->setLSHandle(lshandle_);
                registerCallback(pdm, updateDeviceListCb, loop);
                PMLOG_INFO(CONST_MODULE_NOTIFIER,
                           "Notifier \'%s\' instance created and ready : OK!", "pdm");
            }
        }
    }
    else if (client == NotifierClient::NOTIFIER_CLIENT_APPCAST)
    {
        IFeaturePtr pFeature = pPluginFactory_->createFeature("appcast");
        if (pFeature)
        {
            void *pInterface = nullptr;
            if (pFeature->queryInterface("appcast", &pInterface))
            {
                pFeatureList_.push_back(std::move(pFeature));
                notifierMap_["appcast"] = static_cast<INotifier *>(pInterface);
                INotifier *appcast      = notifierMap_["appcast"];
                appcast->setLSHandle(lshandle_);
                registerCallback(appcast, updateDeviceListCb, loop);
                PMLOG_INFO(CONST_MODULE_NOTIFIER,
                           "Notifier \'%s\' instance created and ready : OK!", "appcast");
            }
        }
    }
}

void Notifier::registerCallback(INotifier *notifier, INotifier::handlercb updateDeviceList,
                                GMainLoop *loop)
{
    if (nullptr != notifier)
        notifier->subscribeToClient(updateDeviceList, loop);
}

void Notifier::setLSHandle(LSHandle *handle) { lshandle_ = handle; }
