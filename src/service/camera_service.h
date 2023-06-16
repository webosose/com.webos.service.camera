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

#ifndef CAMERA_SERVICE_H_
#define CAMERA_SERVICE_H_

/*-----------------------------------------------------------------------------
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "camera_types.h"
#include "event_notification.h"
#include "json_parser.h"
#include "luna-service2/lunaservice.hpp"
#include <glib.h>

class CameraService : public LS::Handle {
private:
  using mainloop = std::unique_ptr<GMainLoop, void (*)(GMainLoop *)>;
  mainloop main_loop_ptr_ = {g_main_loop_new(nullptr, false),
                             g_main_loop_unref};

  EventNotification event_obj;

  int getId(std::string);
  void createEventMessage(EventType, void *, int);

  std::map<int, std::string> cameraHandleMap;
  std::map<std::string, void *> cameraHandleInfo;

  void printMap();
  bool addClientWatcher(LSHandle *handle, LSMessage *message,
                        int ndevice_handle);
  bool eraseWatcher(LSMessage *message, int ndevhandle);

public:
  CameraService();

  CameraService(CameraService const &) = delete;
  CameraService(CameraService &&) = delete;
  CameraService &operator=(CameraService const &) = delete;
  CameraService &operator=(CameraService &&) = delete;

  bool open(LSMessage &);
  bool close(LSMessage &);
  bool getInfo(LSMessage &);
  bool getCameraList(LSMessage &);
  bool getProperties(LSMessage &);
  bool setProperties(LSMessage &);
  bool setFormat(LSMessage &);
  bool startPreview(LSMessage &);
  bool stopPreview(LSMessage &);
  bool startCapture(LSMessage &);
  bool stopCapture(LSMessage &);
  bool getEventNotification(LSMessage &);
  bool getFd(LSMessage &);
  bool getSolutions(LSMessage &message);
  bool setSolutions(LSMessage &message);
  bool getFormat(LSMessage &message);
};

#endif /*CAMERA_SERVICE_H_*/
