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

#include "camera_base.h"
#include "camera_base_wrapper.h"

#ifdef __cplusplus
extern "C" {
#endif

    int open_device(camera_handle_t *h, const char *subsystem)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->openDevice(subsystem);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int close_device(camera_handle_t *h)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->closeDevice();
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int set_format(camera_handle_t *h, stream_format_t cam_format)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->setFormat(cam_format);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int get_format(camera_handle_t *h, stream_format_t *cam_format)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->getFormat(cam_format);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int set_buffer(camera_handle_t *h, int num_buffer, int io_mode)
    {
            CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->setBuffer(num_buffer,io_mode);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int get_buffer(camera_handle_t *h, buffer_t *buf)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->getBuffer(buf);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int release_buffer(camera_handle_t *h, buffer_t buf)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->releaseBuffer(buf);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int destroy_buffer(camera_handle_t *h)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->destroyBuffer();
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int start_capture(camera_handle_t *h)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->startCapture();
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int stop_capture(camera_handle_t *h)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->stopCapture();
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int set_properties(camera_handle_t *h, const camera_properties_t *cam_params)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->setProperties(cam_params);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

    int get_properties(camera_handle_t *h, camera_properties_t *cam_params)
    {
        CameraBase *camera_base = (CameraBase *)h->handle;
        if(NULL != camera_base)
            return camera_base->getProperties(cam_params);
        else
            return CAMERA_ERROR_UNKNOWN;
    }

#ifdef __cplusplus
    }
#endif



