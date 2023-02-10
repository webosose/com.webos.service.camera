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

#ifndef CAMERA_HAL_IF
#define CAMERA_HAL_IF

#ifdef __cplusplus
extern "C"
{
#endif

    int camera_hal_if_init(void **, const char *);
    int camera_hal_if_deinit(void *);
    int camera_hal_if_open_device(void *, const char *, const char *payload = nullptr);
    int camera_hal_if_close_device(void *);
    int camera_hal_if_set_format(void *, const void *);
    int camera_hal_if_get_format(void *, void *);
    int camera_hal_if_set_buffer(void *, int, int, void **);
    int camera_hal_if_get_buffer(void *, void *);
    int camera_hal_if_release_buffer(void *, const void *);
    int camera_hal_if_destroy_buffer(void *);
    int camera_hal_if_start_capture(void *);
    int camera_hal_if_stop_capture(void *);
    int camera_hal_if_set_properties(void *, const void *);
    int camera_hal_if_get_properties(void *, void *);
    int camera_hal_if_get_fd(void *, int *);
    int camera_hal_if_get_info(const char *, const char *, void *);
    int camera_hal_if_get_buffer_fd(void *, int *, int *);

#ifdef __cplusplus
}
#endif

#endif
