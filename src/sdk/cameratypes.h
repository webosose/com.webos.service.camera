// Copyright (c) 2018 LG Electronics, Inc.
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

#ifndef CAMERA_TYPES
#define CAMERA_TYPES

#include <functional>
#include <iostream>

#define PREVIEW_BUFFER 4
#define TIMEOUT -1
#define MAX_DEVICE_COUNT 10
#define CONST_MAX_STRING_LENGTH 256

#ifndef DEBUG_SDK
#define DLOG_SDK(x) x
#else
#define DLOG_SDK(x)
#endif

typedef enum _camera_msg_types
{
  // Add the message type
  CAMERA_MSG_AUTOFOCUS = 1,
  CAMERA_MSG_BRIGHTNESS,
  CAMERA_MSG_OPEN,
  CAMERA_MSG_CLOSE
} camera_msg_types_t;

typedef enum _camera_states
{
  CAMERA_STATE_UNKNOWN = 0,
  CAMERA_STATE_INIT,
  CAMERA_STATE_OPEN,
  CAMERA_STATE_PREVIEW,
  CAMERA_STATE_CAPTURE,
  CAMERA_STATE_RECORD,
  CAMERA_STATE_CLOSE,
} camera_states_t;

typedef enum _dev_event_state
{
  DEV_EVENT_STATE_NONE = 0,
  DEV_EVENT_STATE_PLUGGED = 1,
  DEV_EVENT_STATE_UNPLUGGED,
} dev_event_state_t;

typedef enum _notifier_client
{
  NOTIFIER_CLIENT_PDM = 0,
  NOTIFIER_CLIENT_UDEV
} notifier_client_t;

typedef struct _camera_details
{
  int device_num;
  int port_num;
  std::string device_node;
  std::string vendor_name;
  std::string product_name;
  std::string serial_number;
  std::string device_type;
  std::string device_subtype;
  bool cam_status;
  dev_event_state_t cam_state;
} camera_details_t;

typedef int (*Callback)();

typedef struct _camera_message
{
  camera_msg_types_t msg;
  union {
    int ndata;
    float fdata;
    void *pdata;
    char *strdata;
  };
} camera_message_t;

#endif /*CAMERA_TYPES*/
