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

#ifndef CAMERA_H
#define CAMERA_H

#include <mutex>
#include <map>

#include "camera_types.h"
#include "camera_hal_if.h"

class Camera
{
private:
    void *pcam_handle_;
    int ncam_fd_;
    buffer_t frame_buffer_;
    int nio_mode_;
    camera_info_t stcam_info_;
    camera_states_t cam_state_;
    stream_format_t default_format_;
    std::map<camera_msg_types_t, Callback> callback_map_;
    std::mutex cam_mutex_;

    void getCallback(camera_msg_types_t);

public:
    int init(std::string);
    int deinit();
    int open(std::string);
    int close();
    int setFormat(stream_format_t);
    int startPreview(stream_format_t ,int);
    int stopPreview();
    int addCallbacks(camera_msg_types_t,Callback);
    int removeCallbacks(camera_msg_types_t);

    //getters for gtest
    camera_states_t getCameraState()
    {
        return cam_state_;
    }
};

#endif
