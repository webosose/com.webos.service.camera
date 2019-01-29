// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef CAMERA_HAL_TYPES
#define CAMERA_HAL_TYPES

#include <pthread.h>
#include <stdio.h>

#include "PmLogLib.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define CONST_MODULE_HAL "HAL"

const int variable_initialize = -999;
#define max_index 30
#define max_format 5
#define max_string_length 256

typedef enum
{
  CAMERA_ERROR_UNKNOWN = -1,
  CAMERA_ERROR_NONE = 0,
  CAMERA_ERROR_INVALID_PARAMETER,
  CAMERA_ERROR_INVALID_STATE,
  CAMERA_ERROR_DEVICE_OPEN,
  CAMERA_ERROR_DEVICE_CLOSE,
  CAMERA_ERROR_DEVICE_NOT_FOUND,
  CAMERA_ERROR_PLUGIN_NOT_FOUND,
  CAMERA_ERROR_CREATE_HANDLE,
  CAMERA_ERROR_DESTROY_HANDLE,
  CAMERA_ERROR_SET_FORMAT,
  CAMERA_ERROR_GET_FORMAT,
  CAMERA_ERROR_SET_BUFFER,
  CAMERA_ERROR_GET_BUFFER,
  CAMERA_ERROR_RELEASE_BUFFER,
  CAMERA_ERROR_DESTROY_BUFFER,
  CAMERA_ERROR_START_CAPTURE,
  CAMERA_ERROR_STOP_CAPTURE,
  CAMERA_ERROR_GET_IMAGE,
  CAMERA_ERROR_SET_PROPERTIES,
  CAMERA_ERROR_GET_PROPERTIES,
  CAMERA_ERROR_GET_INFO,
  CAMERA_ERROR_SET_CALLBACK,
  CAMERA_ERROR_REMOVE_CALLBACK,
  CAMERA_ERROR_NO_DEVICE
} camera_error_t;

typedef enum
{
  /* YUV */
  CAMERA_PIXEL_FORMAT_NV12 = 0x0000,
  CAMERA_PIXEL_FORMAT_NV21,
  CAMERA_PIXEL_FORMAT_I420,
  CAMERA_PIXEL_FORMAT_YV12,
  CAMERA_PIXEL_FORMAT_YUYV,
  CAMERA_PIXEL_FORMAT_UYVY,

  /* RGB */
  CAMERA_PIXEL_FORMAT_BGRA8888,
  CAMERA_PIXEL_FORMAT_ARGB8888,

  /* ENCODED */
  CAMERA_PIXEL_FORMAT_JPEG,
  CAMERA_PIXEL_FORMAT_H264,

  /* MAX */
  CAMERA_PIXEL_FORMAT_MAX
} camera_pixel_format_t;

typedef enum
{
  IOMODE_UNKNOWN = -1,
  IOMODE_READ = 0,
  IOMODE_MMAP,
  IOMODE_USERPTR,
  IOMODE_DMABUF
} io_mode_t;

typedef enum
{
  CAMERA_HAL_STATE_UNKNOWN = 0,
  CAMERA_HAL_STATE_INIT,
  CAMERA_HAL_STATE_OPEN,
  CAMERA_HAL_STATE_STREAMING
} camera_hal_state_t;

typedef enum
{
  DEVICE_TYPE_UNDEFINED = -1,
  DEVICE_TYPE_CAMERA = 1,
  DEVICE_TYPE_MICROPHONE,
  DEVICE_TYPE_OTHER
} device_t;

typedef enum
{
  CAMERA_FORMAT_UNDEFINED = -1,
  CAMERA_FORMAT_YUV = 1,
  CAMERA_FORMAT_H264ES = 2,
  CAMERA_FORMAT_JPEG = 4,
} camera_format_t;

/*Structures*/

typedef struct
{
  void *h_library;
  void *h_plugin;
  void *handle;
  int fd;
  void *cb;
  int current_state;
  pthread_mutex_t lock;
} camera_handle_t;

typedef struct
{
  camera_pixel_format_t pixel_format;
  int stream_width;
  int stream_height;
  int stream_fps;
  int stream_rotation;
  int stream_purpose;
  int capture_quality;
  unsigned int buffer_size;
} stream_format_t;

typedef struct
{
  void *start;
  unsigned long length;
  int index;
  int fd;
} buffer_t;

typedef struct
{
  int nBrightness;
  int nContrast;
  int nSaturation;
  int nHue;
  int nAutoWhiteBalance;
  int nGamma;
  int nGain;
  int nFrequency;
  int nWhiteBalanceTemperature;
  int nSharpness;
  int nBacklightCompensation;
  int nAutoExposure;
  int nExposure;
  int nPan;
  int nTilt;
  int nFocusAbsolute;
  int nAutoFocus;
  int nZoomAbsolute;
} camera_properties_t;

typedef struct
{
  int n_width[max_format][max_index];
  int n_height[max_format][max_index];
  char c_res[max_index][max_string_length];
  camera_format_t e_format[max_format];
  int n_frameindex[max_format];
  int n_formatindex;
} camera_resolution_t;

typedef struct
{
  char str_devicename[max_string_length];
  device_t n_devicetype;
  int b_builtin;
  int n_maxvideowidth;
  int n_maxvideoheight;
  int n_maxpicturewidth;
  int n_maxpictureheight;
  int n_format;
  int n_samplingrate;
  int n_codec;
  camera_resolution_t st_resolution;
} camera_device_info_t;

static PmLogContext getHALLunaPmLogContext()
{
  static PmLogContext usLogContext = 0;
  if (0 == usLogContext)
  {
    PmLogGetContext("HAL", &usLogContext);
  }
  return usLogContext;
}

#define HAL_LOG_INFO(module, args...) PmLogMsg(getHALLunaPmLogContext(), Info, module, 0, ##args)

#endif
