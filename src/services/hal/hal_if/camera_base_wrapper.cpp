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

#include "camera_base_wrapper.h"
#include "camera_hal_if_types.h"
#include "plugin_interface.hpp"

#ifdef __cplusplus
extern "C"
{
#endif

    int open_device(camera_handle_t *h, const char *subsystem, const char *payload)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->openDevice(subsystem, payload);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int close_device(camera_handle_t *h)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->closeDevice();
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int set_format(camera_handle_t *h, const void *cam_format)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->setFormat(cam_format);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int get_format(camera_handle_t *h, void *cam_format)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->getFormat(cam_format);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int set_buffer(camera_handle_t *h, int num_buffer, int io_mode, void **usrpbufs)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->setBuffer(num_buffer, io_mode, usrpbufs);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int get_buffer(camera_handle_t *h, void *buf)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->getBuffer(buf);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int release_buffer(camera_handle_t *h, const void *buf)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->releaseBuffer(buf);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int destroy_buffer(camera_handle_t *h)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->destroyBuffer();
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int start_capture(camera_handle_t *h)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->startCapture();
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int stop_capture(camera_handle_t *h)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->stopCapture();
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int set_properties(camera_handle_t *h, const void *cam_params)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->setProperties(cam_params);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int get_properties(camera_handle_t *h, void *cam_params)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->getProperties(cam_params);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int get_info(camera_handle_t *h, void *cam_info, const char *devicenode)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->getInfo(cam_info, devicenode);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int get_buffer_fd(camera_handle_t *h, int *buf_fd, int *count)
    {
        IHal *hal = static_cast<IHal *>(h->handle);
        if (NULL != hal)
            return hal->getBufferFd(buf_fd, count);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

#ifdef __cplusplus
}
#endif
