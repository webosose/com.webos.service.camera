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

#include "camera_hal_if.h"
#include "camera_base_wrapper.h"
#include "camera_hal_types.h"
#include <dlfcn.h>
#include <new>
#include <unistd.h>
#include <cstring>

#ifdef __cplusplus
extern "C"
{
#endif

    int camera_hal_if_init(void **h, const char *subsystem)
    {
        camera_handle_t *camera_handle = new (std::nothrow) camera_handle_t();
        if (!camera_handle)
        {
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle is NULL");
            return CAMERA_ERROR_CREATE_HANDLE;
        }
        HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle : %p", camera_handle);

        camera_handle->h_plugin = dlopen(subsystem, RTLD_LAZY);
        if (!camera_handle->h_plugin)
        {
            HAL_LOG_INFO(CONST_MODULE_HAL, "dlopen failed for : %s", subsystem);
            delete camera_handle;
            return CAMERA_ERROR_PLUGIN_NOT_FOUND;
        }

        typedef void *(*pfn_create_handle)();
        pfn_create_handle pf_create_handle =
            (pfn_create_handle)dlsym(camera_handle->h_plugin, "create_handle");

        if (!pf_create_handle)
        {
            HAL_LOG_INFO(CONST_MODULE_HAL, "dlsym failed ");
            delete camera_handle;
            return CAMERA_ERROR_CREATE_HANDLE;
        }

        camera_handle->handle = (void *)pf_create_handle();
        HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle->handle : %p ", camera_handle->handle);
        camera_handle->current_state = CAMERA_HAL_STATE_INIT;

        *h = (void *)camera_handle;

        return CAMERA_ERROR_NONE;
    }

    int camera_hal_if_deinit(void *h)
    {
        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle is NULL");
            return CAMERA_ERROR_DESTROY_HANDLE;
        }

        HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle : %p ", camera_handle);

        typedef void (*pfn_destroy_handle)(void *);
        pfn_destroy_handle pf_destroy_handle =
            (pfn_destroy_handle)dlsym(camera_handle->h_plugin, "destroy_handle");
        if (!pf_destroy_handle)
        {
            HAL_LOG_INFO(CONST_MODULE_HAL, "dlsym failed ");
            return CAMERA_ERROR_DESTROY_HANDLE;
        }

        camera_handle->current_state = CAMERA_HAL_STATE_UNKNOWN;

        pf_destroy_handle(camera_handle->handle);
        delete camera_handle;

        return CAMERA_ERROR_NONE;
    }

    int camera_hal_if_open_device(void *h, const char *dev)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_DEVICE_OPEN;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        if (!*dev)
        {
            retVal = CAMERA_ERROR_DEVICE_OPEN;
            HAL_LOG_INFO(CONST_MODULE_HAL, "device node is empty ");
            return retVal;
        }

        HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle : %p , device : %s", camera_handle, dev);

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if camera is in INIT state
        if (camera_handle->current_state != CAMERA_HAL_STATE_INIT)
        {
            retVal = CAMERA_ERROR_DEVICE_OPEN;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not INIT ");
            return retVal;
        }

        camera_handle->fd = open_device(camera_handle, dev);
        HAL_LOG_INFO(CONST_MODULE_HAL, "fd : %d ", camera_handle->fd);

        if (camera_handle->fd == CAMERA_ERROR_UNKNOWN)
        {
            retVal = CAMERA_ERROR_DEVICE_OPEN;
            HAL_LOG_INFO(CONST_MODULE_HAL, "fd invalid ");
        }
        else
        {
            camera_handle->current_state = CAMERA_HAL_STATE_OPEN;
        }
        return retVal;
    }

    int camera_hal_if_close_device(void *h)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_DEVICE_CLOSE;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if camera is in OPEN state
        if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
        {
            retVal = CAMERA_ERROR_DEVICE_CLOSE;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not OPEN");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == close_device(camera_handle))
        {
            retVal = CAMERA_ERROR_DEVICE_CLOSE;
            HAL_LOG_INFO(CONST_MODULE_HAL, "close_device failed");
        }
        else
        {
            camera_handle->current_state = CAMERA_HAL_STATE_INIT;
        }
        return retVal;
    }

    int camera_hal_if_set_format(void *h, stream_format_t stream_format)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_SET_FORMAT;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if camera is in OPEN state
        if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
        {
            retVal = CAMERA_ERROR_SET_FORMAT;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not OPEN");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == set_format(camera_handle, stream_format))
        {
            retVal = CAMERA_ERROR_SET_FORMAT;
            HAL_LOG_INFO(CONST_MODULE_HAL, "set_format failed");
        }
        return retVal;
    }

    int camera_hal_if_get_format(void *h, stream_format_t *stream_format)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_GET_FORMAT;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if camera is in OPEN or STREAMING state
        if (camera_handle->current_state == CAMERA_HAL_STATE_INIT)
        {
            retVal = CAMERA_ERROR_GET_FORMAT;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not OPEN or STREAMING ");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == get_format(camera_handle, stream_format))
        {
            retVal = CAMERA_ERROR_GET_FORMAT;
            HAL_LOG_INFO(CONST_MODULE_HAL, "get_format failed");
        }
        return retVal;
    }

    int camera_hal_if_set_buffer(void *h, int NoBuffer, int IOMode, buffer_t **usrpbufs)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_SET_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if camera is in OPEN state
        if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
        {
            retVal = CAMERA_ERROR_SET_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not OPEN ");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == set_buffer(camera_handle, NoBuffer, IOMode, usrpbufs))
        {
            retVal = CAMERA_ERROR_SET_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "set_buffer failed ");
        }
        return retVal;
    }

    int camera_hal_if_get_buffer(void *h, buffer_t *buf)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_GET_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if camera is in STREAMING state
        if (camera_handle->current_state != CAMERA_HAL_STATE_STREAMING)
        {
            retVal = CAMERA_ERROR_GET_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not STREAMING ");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == get_buffer(camera_handle, buf))
        {
            retVal = CAMERA_ERROR_GET_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "get_buffer failed");
        }
        return retVal;
    }

    int camera_hal_if_release_buffer(void *h, buffer_t buf)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_RELEASE_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if camera is in STREAMING state
        if (camera_handle->current_state != CAMERA_HAL_STATE_STREAMING)
        {
            retVal = CAMERA_ERROR_RELEASE_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not STREAMING ");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == release_buffer(camera_handle, buf))
        {
            retVal = CAMERA_ERROR_RELEASE_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "release_buffer failed");
        }
        return retVal;
    }

    int camera_hal_if_destroy_buffer(void *h)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_DESTROY_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if camera is in OPEN state
        if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
        {
            retVal = CAMERA_ERROR_DESTROY_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not OPEN ");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == destroy_buffer(camera_handle))
        {
            retVal = CAMERA_ERROR_DESTROY_BUFFER;
            HAL_LOG_INFO(CONST_MODULE_HAL, "destroy_buffer failed");
        }
        return retVal;
    }

    int camera_hal_if_start_capture(void *h)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_START_CAPTURE;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if HAL state is OPEN
        if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
        {
            retVal = CAMERA_ERROR_START_CAPTURE;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not OPEN ");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == start_capture(camera_handle))
        {
            retVal = CAMERA_ERROR_START_CAPTURE;
            HAL_LOG_INFO(CONST_MODULE_HAL, "start_capture failed");
        }
        else
        {
            camera_handle->current_state = CAMERA_HAL_STATE_STREAMING;
        }
        return retVal;
    }

    int camera_hal_if_stop_capture(void *h)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_STOP_CAPTURE;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if HAL state is STREAMING
        if (camera_handle->current_state != CAMERA_HAL_STATE_STREAMING)
        {
            retVal = CAMERA_ERROR_STOP_CAPTURE;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not STREAMING ");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == stop_capture(camera_handle))
        {
            retVal = CAMERA_ERROR_STOP_CAPTURE;
            HAL_LOG_INFO(CONST_MODULE_HAL, "stop_capture failed");
        }
        else
        {
            camera_handle->current_state = CAMERA_HAL_STATE_OPEN;
        }
        return retVal;
    }

    int camera_hal_if_set_properties(void *h, const camera_properties_t *cam_in_params)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_SET_PROPERTIES;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);

        // check if HAL state is OPEN or STREAMING
        if (camera_handle->current_state < CAMERA_HAL_STATE_OPEN)
        {
            retVal = CAMERA_ERROR_SET_PROPERTIES;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not OPEN ");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == set_properties(camera_handle, cam_in_params))
        {
            retVal = CAMERA_ERROR_SET_PROPERTIES;
            HAL_LOG_INFO(CONST_MODULE_HAL, "set_properties failed");
        }
        return retVal;
    }

    int camera_hal_if_get_properties(void *h, camera_properties_t *cam_out_params)
    {
        int retVal = CAMERA_ERROR_NONE;

        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (!camera_handle)
        {
            retVal = CAMERA_ERROR_GET_PROPERTIES;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }

        const std::lock_guard<std::mutex> lock(camera_handle->lock);
        // check if HAL state is OPEN or STREAMING
        if (camera_handle->current_state == CAMERA_HAL_STATE_INIT)
        {
            retVal = CAMERA_ERROR_GET_PROPERTIES;
            HAL_LOG_INFO(CONST_MODULE_HAL, "Camera HAL State not OPEN or STREAMING");
            return retVal;
        }

        if (CAMERA_ERROR_UNKNOWN == get_properties(camera_handle, cam_out_params))
        {
            retVal = CAMERA_ERROR_GET_PROPERTIES;
            HAL_LOG_INFO(CONST_MODULE_HAL, "get_properties failed");
        }
        return retVal;
    }

    int camera_hal_if_get_fd(void *h, int *fd)
    {
        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (camera_handle)
        {
            *fd = camera_handle->fd;
        }
        return CAMERA_ERROR_NONE;
    }

    int camera_hal_if_get_info(const char *devicenode, camera_device_info_t *caminfo)
    {
        void *handle;
        int retVal;

        if (strstr(devicenode, "udpsrc"))
            retVal = camera_hal_if_init(&handle, "libremote-camera-plugin.so");
        else
            retVal = camera_hal_if_init(&handle, "libv4l2-camera-plugin.so");

        if (CAMERA_ERROR_NONE != retVal)
        {
            retVal = CAMERA_ERROR_GET_INFO;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera_handle NULL ");
            return retVal;
        }
        camera_handle_t *camera_handle = (camera_handle_t *)handle;

        retVal = get_info(camera_handle, caminfo, devicenode);
        if (CAMERA_ERROR_UNKNOWN == retVal)
        {
            retVal = CAMERA_ERROR_GET_INFO;
            HAL_LOG_INFO(CONST_MODULE_HAL, "get_info failed");
        }

        camera_hal_if_deinit(handle);
        return retVal;
    }

    int camera_hal_if_get_buffer_fd(void *h, int *bufFd, int *count)
    {
        int retVal                     = -1;
        camera_handle_t *camera_handle = (camera_handle_t *)h;
        if (camera_handle)
        {
            const std::lock_guard<std::mutex> lock(camera_handle->lock);

            retVal = get_buffer_fd(camera_handle, bufFd, count);
            if (retVal == CAMERA_ERROR_UNKNOWN)
            {
                retVal = CAMERA_ERROR_GET_BUFFER_FD;
                HAL_LOG_INFO(CONST_MODULE_HAL, "CAMERA_ERROR_UNKNOWN failed ");
            }
        }
        else
        {
            retVal = CAMERA_ERROR_GET_BUFFER_FD;
            HAL_LOG_INFO(CONST_MODULE_HAL, "camera handle NULL  ");
        }
        return retVal;
    }

#ifdef __cplusplus
}
#endif
