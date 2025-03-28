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

#define LOG_TAG "Notifier"
#include "notifier.h"
#include "device_manager.h"

static bool updateDeviceListCb(std::string deviceType, const void *deviceList)
{
    return DeviceManager::getInstance().updateDeviceList(
        std::move(deviceType), *static_cast<const std::vector<DEVICE_LIST_T> *>(deviceList));
}

Notifier::~Notifier()
{
    if (pPluginFactory_)
    {
        delete pPluginFactory_;
        pPluginFactory_ = nullptr;
    }
}

void Notifier::addNotifiers(GMainLoop *loop)
{
    auto lstFeatNames = pPluginFactory_->getFeatureList("NOTIFIER");
    for (auto &f : lstFeatNames)
    {
        PLOGI("client : %s", f.c_str());
        IFeaturePtr pFeature = pPluginFactory_->createFeature(f.c_str());
        if (pFeature)
        {
            INotifier *pNotifier = nullptr;
            if (pFeature->queryInterface(f.c_str(), reinterpret_cast<void **>(&pNotifier)))
            {
                pFeatureList_.push_back(std::move(pFeature));
                notifierMap_[f] = pNotifier;
                pNotifier->setLSHandle(lshandle_);
                registerCallback(pNotifier, updateDeviceListCb, loop);
                PLOGI("Notifier \'%s\' instance created and ready : OK!", f.c_str());
            }
        }
    }
}

void Notifier::registerCallback(INotifier *notifier, INotifier::handlercb updateDeviceList,
                                GMainLoop *loop)
{
    if (nullptr != notifier)
        notifier->subscribeToClient(std::move(updateDeviceList), loop);
}

void Notifier::setLSHandle(LSHandle *handle) { lshandle_ = handle; }
