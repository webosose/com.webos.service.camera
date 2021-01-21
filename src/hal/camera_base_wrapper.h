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

#ifndef CAMERA_BASE_WRAPPER
#define CAMERA_BASE_WRAPPER

#include "camera_hal_types.h"
#include "camera_hal_if_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

int open_device(camera_handle_t *, const char *);
int close_device(camera_handle_t *);
int set_format(camera_handle_t *, stream_format_t);
int get_format(camera_handle_t *, stream_format_t *);
int set_buffer(camera_handle_t *, int, int);
int get_buffer(camera_handle_t *, buffer_t *);
int release_buffer(camera_handle_t *, buffer_t);
int destroy_buffer(camera_handle_t *);
int start_capture(camera_handle_t *);
int stop_capture(camera_handle_t *);
int set_properties(camera_handle_t *, const camera_properties_t *);
int get_properties(camera_handle_t *, camera_properties_t *);
int get_info(camera_handle_t *, camera_device_info_t *, const char *);
int get_buffer_fd(camera_handle_t *, int *, int *);

#ifdef __cplusplus
}
#endif

#endif
