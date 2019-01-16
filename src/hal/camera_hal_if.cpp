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

#include "camera_hal_if.h"
#include "camera_base_wrapper.h"
#include <dlfcn.h>
#include <new>

#ifdef __cplusplus
extern "C"
{
#endif

  int camera_hal_if_init(void **h, const char *subsystem)
  {
    camera_handle_t *camera_handle = new (std::nothrow) camera_handle_t();
    if (NULL == camera_handle)
    {
      DLOG(printf("CAMERA_ERROR_CREATE_HANDLE\n"););
      return CAMERA_ERROR_CREATE_HANDLE;
    }
    DLOG(printf("In camera_hal_if_init : camera_handle : %p\n", camera_handle););

    camera_handle->h_library = (void *)subsystem;

    camera_handle->h_plugin = dlopen(subsystem, RTLD_LAZY);
    if (!camera_handle->h_plugin)
    {
      DLOG(printf("CAMERA_ERROR_PLUGIN_NOT_FOUND : %s\n", subsystem););
      return CAMERA_ERROR_PLUGIN_NOT_FOUND;
    }

    typedef void *(*pfn_create_handle)();
    pfn_create_handle pf_create_handle =
        (pfn_create_handle)dlsym(camera_handle->h_plugin, "create_handle");

    if (!pf_create_handle)
    {
      DLOG(printf("CAMERA_ERROR_CREATE_HANDLE\n"););
      return CAMERA_ERROR_CREATE_HANDLE;
    }

    camera_handle->handle = (void *)pf_create_handle();
    DLOG(printf("In camera_hal_if_init : camera_handle->handle : %p\n", camera_handle->handle););
    camera_handle->current_state = CAMERA_HAL_STATE_INIT;
    if (0 != pthread_mutex_init(&camera_handle->lock, NULL))
    {
      DLOG(printf("pthread_mutex_init failed for handle : %p \n", camera_handle->handle););
      int retVal = camera_hal_if_deinit((void *)camera_handle);
      *h = NULL;
      return retVal;
    }

    *h = (void *)camera_handle;

    return CAMERA_ERROR_NONE;
  }

  int camera_hal_if_deinit(void *h)
  {
    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      DLOG(printf("CAMERA_ERROR_DESTROY_HANDLE\n"););
      return CAMERA_ERROR_DESTROY_HANDLE;
    }

    DLOG(printf("In camera_hal_if_deinit : camera_handle : %p\n", camera_handle););

    typedef void (*pfn_destroy_handle)(void *);
    pfn_destroy_handle pf_destroy_handle =
        (pfn_destroy_handle)dlsym(camera_handle->h_plugin, "destroy_handle");
    if (!pf_destroy_handle)
    {
      char *error;
      if ((error = dlerror()) != NULL)
        DLOG(fprintf(stderr, "%s\n", error););
      return CAMERA_ERROR_DESTROY_HANDLE;
    }

    camera_handle->current_state = CAMERA_HAL_STATE_UNKNOWN;
    pthread_mutex_destroy(&camera_handle->lock);

    pf_destroy_handle(camera_handle->handle);
    delete camera_handle;

    return CAMERA_ERROR_NONE;
  }

  int camera_hal_if_open_device(void *h, const char *dev)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_DEVICE_OPEN;
      DLOG(printf("CAMERA_ERROR_DEVICE_OPEN\n"););
      return retVal;
    }

    DLOG(printf("In camera_hal_if_open_device : camera_handle : %p\n", camera_handle););

    pthread_mutex_lock(&camera_handle->lock);

    // check if camera is in INIT state
    if (camera_handle->current_state != CAMERA_HAL_STATE_INIT)
    {
      retVal = CAMERA_ERROR_DEVICE_OPEN;
      DLOG(printf("camera_hal_if_open_device : Camera HAL State not INIT\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    camera_handle->fd = open_device(camera_handle, dev);
    DLOG(printf("camera_hal_if_open_device fd : %d\n", camera_handle->fd););

    if (CAMERA_ERROR_UNKNOWN == camera_handle->fd)
    {
      retVal = CAMERA_ERROR_DEVICE_OPEN;
      DLOG(printf("CAMERA_ERROR_DEVICE_OPEN\n"););
    }
    else
    {
      camera_handle->current_state = CAMERA_HAL_STATE_OPEN;
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_close_device(void *h)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_DEVICE_CLOSE;
      DLOG(printf("CAMERA_ERROR_DEVICE_CLOSE\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if camera is in OPEN state
    if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
    {
      retVal = CAMERA_ERROR_DEVICE_CLOSE;
      DLOG(printf("camera_hal_if_close_device : Camera HAL State not OPEN\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == close_device(camera_handle))
    {
      retVal = CAMERA_ERROR_DEVICE_CLOSE;
      DLOG(printf("CAMERA_ERROR_DEVICE_CLOSE\n"););
    }
    else
    {
      camera_handle->current_state = CAMERA_HAL_STATE_INIT;
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_set_format(void *h, stream_format_t stream_format)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_SET_FORMAT;
      DLOG(printf("CAMERA_ERROR_SET_FORMAT\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if camera is in OPEN state
    if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
    {
      retVal = CAMERA_ERROR_SET_FORMAT;
      DLOG(printf("camera_hal_if_set_format : Camera HAL State not OPEN\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == set_format(camera_handle, stream_format))
    {
      retVal = CAMERA_ERROR_SET_FORMAT;
      DLOG(printf("CAMERA_ERROR_SET_FORMAT\n"););
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_get_format(void *h, stream_format_t *stream_format)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_GET_FORMAT;
      DLOG(printf("CAMERA_ERROR_GET_FORMAT\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if camera is in OPEN or STREAMING state
    if (camera_handle->current_state == CAMERA_HAL_STATE_INIT)
    {
      retVal = CAMERA_ERROR_GET_FORMAT;
      DLOG(printf("camera_hal_if_get_format : Camera HAL State not OPEN or STREAMING\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == get_format(camera_handle, stream_format))
    {
      retVal = CAMERA_ERROR_GET_FORMAT;
      DLOG(printf("CAMERA_ERROR_GET_FORMAT\n"););
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_set_buffer(void *h, int NoBuffer, int IOMode)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_SET_BUFFER;
      DLOG(printf("CAMERA_ERROR_SET_BUFFER\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if camera is in OPEN state
    if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
    {
      retVal = CAMERA_ERROR_SET_BUFFER;
      DLOG(printf("camera_hal_if_set_buffer : Camera HAL State not OPEN\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == set_buffer(camera_handle, NoBuffer, IOMode))
    {
      retVal = CAMERA_ERROR_SET_BUFFER;
      DLOG(printf("CAMERA_ERROR_SET_BUFFER\n"););
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_get_buffer(void *h, buffer_t *buf)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_GET_BUFFER;
      DLOG(printf("CAMERA_ERROR_GET_BUFFER\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if camera is in STREAMING state
    if (camera_handle->current_state != CAMERA_HAL_STATE_STREAMING)
    {
      retVal = CAMERA_ERROR_GET_BUFFER;
      DLOG(printf("camera_hal_if_get_buffer : Camera HAL State not STREAMING\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == get_buffer(camera_handle, buf))
    {
      retVal = CAMERA_ERROR_GET_BUFFER;
      DLOG(printf("CAMERA_ERROR_GET_BUFFER\n"););
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_release_buffer(void *h, buffer_t buf)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_RELEASE_BUFFER;
      DLOG(printf("CAMERA_ERROR_RELEASE_BUFFER\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if camera is in STREAMING state
    if (camera_handle->current_state != CAMERA_HAL_STATE_STREAMING)
    {
      retVal = CAMERA_ERROR_RELEASE_BUFFER;
      DLOG(printf("camera_hal_if_release_buffer : Camera HAL State not STREAMING\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == release_buffer(camera_handle, buf))
    {
      retVal = CAMERA_ERROR_RELEASE_BUFFER;
      DLOG(printf("CAMERA_ERROR_RELEASE_BUFFER\n"););
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_destroy_buffer(void *h)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_DESTROY_BUFFER;
      DLOG(printf("CAMERA_ERROR_DESTROY_BUFFER\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if camera is in OPEN state
    if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
    {
      retVal = CAMERA_ERROR_DESTROY_BUFFER;
      DLOG(printf("camera_hal_if_destroy_buffer : Camera HAL State not OPEN\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == destroy_buffer(camera_handle))
    {
      retVal = CAMERA_ERROR_DESTROY_BUFFER;
      DLOG(printf("CAMERA_ERROR_DESTROY_BUFFER\n"););
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_start_capture(void *h)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_START_CAPTURE;
      DLOG(printf("CAMERA_ERROR_START_CAPTURE\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if HAL state is OPEN
    if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
    {
      retVal = CAMERA_ERROR_START_CAPTURE;
      DLOG(printf("camera_hal_if_start_capture : Camera HAL State not OPEN\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == start_capture(camera_handle))
    {
      retVal = CAMERA_ERROR_START_CAPTURE;
      DLOG(printf("CAMERA_ERROR_START_CAPTURE\n"););
    }
    else
    {
      camera_handle->current_state = CAMERA_HAL_STATE_STREAMING;
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_stop_capture(void *h)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_STOP_CAPTURE;
      DLOG(printf("CAMERA_ERROR_STOP_CAPTURE\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if HAL state is STREAMING
    if (camera_handle->current_state != CAMERA_HAL_STATE_STREAMING)
    {
      retVal = CAMERA_ERROR_STOP_CAPTURE;
      DLOG(printf("camera_hal_if_stop_capture : Camera HAL State not STREAMING\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == stop_capture(camera_handle))
    {
      retVal = CAMERA_ERROR_STOP_CAPTURE;
      DLOG(printf("CAMERA_ERROR_STOP_CAPTURE\n"););
    }
    else
    {
      camera_handle->current_state = CAMERA_HAL_STATE_OPEN;
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_set_properties(void *h, const camera_properties_t *cam_in_params)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_SET_PROPERTIES;
      DLOG(printf("CAMERA_ERROR_SET_PROPERTIES\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if HAL state is OPEN
    if (camera_handle->current_state != CAMERA_HAL_STATE_OPEN)
    {
      retVal = CAMERA_ERROR_SET_PROPERTIES;
      DLOG(printf("camera_hal_if_set_properties : Camera HAL State not OPEN\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == set_properties(camera_handle, cam_in_params))
    {
      retVal = CAMERA_ERROR_SET_PROPERTIES;
      DLOG(printf("CAMERA_ERROR_SET_PROPERTIES\n"););
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_get_properties(void *h, camera_properties_t *cam_out_params)
  {
    int retVal = CAMERA_ERROR_NONE;

    camera_handle_t *camera_handle = (camera_handle_t *)h;
    if (NULL == camera_handle)
    {
      retVal = CAMERA_ERROR_GET_PROPERTIES;
      DLOG(printf("CAMERA_ERROR_GET_PROPERTIES\n"););
      return retVal;
    }

    pthread_mutex_lock(&camera_handle->lock);

    // check if HAL state is OPEN or STREAMING
    if (camera_handle->current_state == CAMERA_HAL_STATE_INIT)
    {
      retVal = CAMERA_ERROR_GET_PROPERTIES;
      DLOG(printf("camera_hal_if_get_properties : Camera HAL State not OPEN or STREAMING\n"););
      pthread_mutex_unlock(&camera_handle->lock);
      return retVal;
    }

    if (CAMERA_ERROR_UNKNOWN == get_properties(camera_handle, cam_out_params))
    {
      retVal = CAMERA_ERROR_GET_PROPERTIES;
      DLOG(printf("CAMERA_ERROR_GET_PROPERTIES\n"););
    }

    pthread_mutex_unlock(&camera_handle->lock);

    return retVal;
  }

  int camera_hal_if_get_fd(void *h, int *fd)
  {
    camera_handle_t *camera_handle = (camera_handle_t *)h;

    if (NULL != camera_handle)
    {
      *fd = camera_handle->fd;
    }
    return CAMERA_ERROR_NONE;
  }

  int camera_hal_if_get_info(const char *devicenode, camera_device_info_t *caminfo)
  {
    void *handle;
    int retVal = camera_hal_if_init(&handle, "libv4l2-camera-plugin.so");
    camera_handle_t *camera_handle = (camera_handle_t *)handle;

    if (CAMERA_ERROR_UNKNOWN == get_info(camera_handle, caminfo, devicenode))
    {
      retVal = CAMERA_ERROR_GET_INFO;
      DLOG(printf("CAMERA_ERROR_GET_INFO\n"););
    }

    retVal = camera_hal_if_deinit(handle);

    return retVal;
  }

#ifdef __cplusplus
}
#endif
