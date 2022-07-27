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

#include "camera_types.h"
#include <pbnjson.hpp>

class EventNotification
{
public:
    EventNotification() { strcamid_ = ""; }

    ~EventNotification() {}

    bool addSubscription(LSHandle *lsHandle, const char *key, LSMessage &message);
    void eventReply(LSHandle *lsHandle, const char *key, void *p_cur_data, void *p_old_data,
                    EventType etype);
    std::string subscriptionJsonString(bool issubscribed);
    void setCameraId(const std::string &camid) { strcamid_ = camid; }
    std::string getCameraId() { return strcamid_; }

private:
    std::string strcamid_;
    bool getJsonString(jvalue_ref &json_outobj, void *p_cur_data, void *p_old_data,
                       EventType etype);
    int getSubscripeCount(LSHandle *lsHandle, const char *key);
    void subscriptionReply(LSHandle *lsHandle, const char *key, jvalue_ref output_reply);
};
