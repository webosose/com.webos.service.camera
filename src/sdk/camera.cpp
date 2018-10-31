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

/******************************************************************************
 File Inclusions
 ******************************************************************************/
#include <poll.h>
#include "camera.h"

int Camera::init(std::string subsystem)
{
    int retval = CAMERA_ERROR_UNKNOWN;

    retval = camera_hal_if_init(&pcam_handle_,subsystem.c_str());
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_init failed" << std::endl;);
        return retval;
    }

    std::lock_guard<std::mutex> guard(cam_mutex_);
    if(CAMERA_STATE_UNKNOWN== cam_state_)
        cam_state_ = CAMERA_STATE_INIT;

    return retval;
}

int Camera::deinit()
{
    int retval = CAMERA_ERROR_UNKNOWN;

    retval = camera_hal_if_deinit(pcam_handle_);
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_deinit failed" << std::endl;);
        return retval;
    }

    std::lock_guard<std::mutex> guard(cam_mutex_);
    if(CAMERA_STATE_INIT == cam_state_)
        cam_state_ = CAMERA_STATE_UNKNOWN;

    return retval;
}

int Camera::open(std::string devicenode)
{
    int retval = CAMERA_ERROR_UNKNOWN;

    retval = camera_hal_if_open_device(pcam_handle_,devicenode.c_str());
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_open_device failed" << std::endl;);
        return retval;
    }

    std::lock_guard<std::mutex> guard(cam_mutex_);
    stcam_info_.cam_status = true;
    if(CAMERA_STATE_INIT == cam_state_)
        cam_state_ = CAMERA_STATE_OPEN;

    //callback function
    Callback function = callback_map_.find(CAMERA_MSG_OPEN)->second;
    if (function)
    {
        (*function)();
    }
    else
    {
       DLOG_SDK(std::cout << "No callback registered for Open" << std::endl;);
    }

    return retval;
}

int Camera::close()
{
    int retval = CAMERA_ERROR_UNKNOWN;

    retval= camera_hal_if_close_device(pcam_handle_);
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_close_device failed" << std::endl;);
        return retval;
    }

    stcam_info_.cam_status = false;
    std::lock_guard<std::mutex> guard(cam_mutex_);
    if(CAMERA_STATE_OPEN == cam_state_)
        cam_state_ = CAMERA_STATE_CLOSE;

    return retval;
}

int Camera::startPreview(stream_format_t stformat,int mode)
{
    int retval = CAMERA_ERROR_NONE;
    int npollstatus = EINVAL;

    retval = camera_hal_if_get_fd(pcam_handle_,&ncam_fd_);
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_get_fd failed" << std::endl;);
        return retval;
    }

    struct pollfd poll_set[]{
        {.fd = ncam_fd_,.events = POLLIN},
    };
    nio_mode_ = mode;

    retval = camera_hal_if_get_format(pcam_handle_,&default_format_);
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_get_format failed" << std::endl;);
        return retval;
    }

    if((default_format_.stream_height != stformat.stream_height) ||
       (default_format_.stream_width!= stformat.stream_width) ||
       (default_format_.pixel_format!= stformat.pixel_format))
    {
        retval = camera_hal_if_set_format(pcam_handle_,stformat);
        if(CAMERA_ERROR_NONE != retval)
        {
            DLOG_SDK(std::cout << "camera_hal_if_set_format failed" << std::endl;);
            return retval;
        }
    }

    retval = camera_hal_if_set_buffer(pcam_handle_,PREVIEW_BUFFER,nio_mode_);
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_set_buffer failed" << std::endl;);
        return retval;
    }

    retval = camera_hal_if_start_capture(pcam_handle_);
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_start_capture failed" << std::endl;);
        return retval;
    }

    std::lock_guard<std::mutex> guard(cam_mutex_);
    if(CAMERA_STATE_OPEN == cam_state_)
        cam_state_ = CAMERA_STATE_PREVIEW;

    while((npollstatus = poll(poll_set, 2, TIMEOUT)) > 0)
    {
        retval = camera_hal_if_get_buffer(pcam_handle_,&frame_buffer_);
        if(CAMERA_ERROR_NONE != retval)
        {
            DLOG_SDK(std::cout << "camera_hal_if_get_buffer failed" << std::endl;);
            return retval;
        }

        retval = camera_hal_if_release_buffer(pcam_handle_,frame_buffer_);
        if(CAMERA_ERROR_NONE != retval)
        {
            DLOG_SDK(std::cout << "camera_hal_if_release_buffer failed" << std::endl;);
            return retval;
        }
    }

    return retval;
}

int Camera::stopPreview()
{
    int retval = CAMERA_ERROR_UNKNOWN;

    retval = camera_hal_if_stop_capture(pcam_handle_);
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_stop_capture failed" << std::endl;);
        return retval;
    }

    retval = camera_hal_if_destroy_buffer(pcam_handle_);
    if(CAMERA_ERROR_NONE != retval)
    {
        DLOG_SDK(std::cout << "camera_hal_if_destroy_buffer failed" << std::endl;);
        return retval;
    }

    std::lock_guard<std::mutex> guard(cam_mutex_);
    if(CAMERA_STATE_PREVIEW == cam_state_)
        cam_state_ = CAMERA_STATE_OPEN;

    return retval;
}

int Camera::addCallbacks(camera_msg_types_t msg,Callback func)
{
    // register for callback function
    callback_map_[msg] =  func;
    return CAMERA_ERROR_NONE;
}

int Camera::removeCallbacks(camera_msg_types_t msg)
{
    callback_map_[msg] = NULL;
    return CAMERA_ERROR_NONE;
}

