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

#ifndef NOTIFIER_H_
#define NOTIFIER_H_

#include "luna-service2/lunaservice.hpp"
#include <functional>
#include <iostream>
#include <map>
#include <plugin_factory.hpp>
#include <plugin_interface.hpp>

class Notifier
{
private:
    LSHandle *lshandle_;
    PluginFactory *pPluginFactory_{nullptr};
    std::vector<IFeaturePtr> pFeatureList_;

public:
    Notifier()
    {
        lshandle_       = nullptr;
        pPluginFactory_ = new PluginFactory();
    }
    virtual ~Notifier();

    void addNotifiers(GMainLoop *loop);
    void setLSHandle(LSHandle *);

private:
    void registerCallback(INotifier *, INotifier::handlercb, GMainLoop *loop);

private:
    std::map<std::string, INotifier *> notifierMap_;
};

#endif /* NOTIFIER_H_ */
