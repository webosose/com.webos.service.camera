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

#include "camera_types.h"
#include <functional>
#include <iostream>

#include "device_notifier.h"
#include "luna-service2/lunaservice.hpp"
#include "pdm_client.h"

class Notifier
{
private:
  using handlercb = std::function<void(DEVICE_LIST_T *)>;

  PDMClient pdm_;
  LSHandle *lshandle_;
  DeviceNotifier *p_client_notifier_;

public:
  Notifier()
  {
    lshandle_ = nullptr;
    p_client_notifier_ = nullptr;
  }
  virtual ~Notifier() {}

  void addNotifier(NotifierClient, GMainLoop *loop);
  void registerCallback(handlercb, GMainLoop *loop);
  void setLSHandle(LSHandle *);
};

#endif /* NOTIFIER_H_ */
