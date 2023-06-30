// Copyright (c) 2019-2023 LG Electronics, Inc.
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

#include "camera_types.h"
#include <nlohmann/json.hpp>
#include <pbnjson.hpp>

using namespace nlohmann;

class EventNotification
{
public:
    EventNotification(){};
    ~EventNotification() {}

    bool addSubscription(LSHandle *lsHandle, std::string key, LSMessage &message);
    void eventReply(LSHandle *lsHandle, std::string key, EventType etype,
                    void *p_cur_data = nullptr, void *p_old_data = nullptr);
    std::string getEventKeyWithId(int dev_handle, std::string key);
    int getSubscribeCount(LSHandle *lsHandle, std::string key);
    void removeSubscription(LSHandle *lsHandle, int camera_id);

private:
    bool getJsonString(json &json_outobj, std::string key, EventType etype, void *p_cur_data,
                       void *p_old_data);
    void subscriptionReply(LSHandle *lsHandle, std::string key, std::string output_reply);
    std::string getCameraIdFromKey(std::string key);
};
