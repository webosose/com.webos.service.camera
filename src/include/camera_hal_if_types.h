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

//camera_hal_if_types.h is for common use with camsrc of g-camera-pipeline.
//It should be written only in C style.
//If the type is not used in camsrc, write it in camera_hal_types.h. ( ex. C++ style )

#ifndef CAMERA_HAL_IF_TYPES
#define CAMERA_HAL_IF_TYPES
typedef enum
{
  CAMERA_HAL_STATE_UNKNOWN = 0,
  CAMERA_HAL_STATE_INIT,
  CAMERA_HAL_STATE_OPEN,
  CAMERA_HAL_STATE_STREAMING
} camera_hal_state_t;

/*Structures*/
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
    IOMODE_READ    = 0,
    IOMODE_MMAP,
    IOMODE_USERPTR,
    IOMODE_DMABUF
} io_mode_t;
typedef struct
{
    camera_pixel_format_t pixel_format;
    unsigned int stream_width;
    unsigned int stream_height;
    int stream_fps;
    unsigned int buffer_size;
    const char *userdata;
} stream_format_t;

typedef struct
{
    void *start;
    unsigned long length;
    int index;
    int fd;
} buffer_t;

#endif

