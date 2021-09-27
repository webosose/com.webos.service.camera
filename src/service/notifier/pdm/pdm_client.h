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

#ifndef PDM_CLIENT
#define PDM_CLIENT

#include "camera_types.h"
#include "device_notifier.h"
#include <functional>
#include <luna-service2/lunaservice.hpp>

using pdmhandlercb = std::function<void(DEVICE_LIST_T *)>;

class PDMClient : public DeviceNotifier
{
private:
  static bool subscribeToPdmService(LSHandle *sh,
                const char *serviceName, bool connected, void *ctx);
  LSHandle *lshandle_;

public:
  PDMClient() { lshandle_ = nullptr; }
  virtual ~PDMClient() {}
  virtual void subscribeToClient(pdmhandlercb, GMainLoop *loop) override;
  void setLSHandle(LSHandle *);
};

#endif
