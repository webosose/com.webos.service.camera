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
#include "camera_hal_if_cpp_types.h"
#include "camera_hal_if_types.h"
#include "camera_hal_types.h"
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

const char *subsystem = "libv4l2-camera-plugin.so";
const char *devname   = "/dev/video0";

static unsigned int random_value = 0;
const int fps_30                 = 30;

// default values
#define DEFAULT_BRIGHTNESS 101
#define DEFAULT_CONTRAST 7
#define DEFAULT_SATURATION 62
#define DEFAULT_AUTOWHITEBALANCE 1

void PrintStreamFormat(stream_format_t streamformat)
{
    HAL_LOG_INFO(CONST_MODULE_HAL, "StreamFormat : \n");
    HAL_LOG_INFO(CONST_MODULE_HAL, "    Pixel Format : %d\n", streamformat.pixel_format);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    Width : %d\n", streamformat.stream_width);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    Height : %d\n", streamformat.stream_height);
}

void PrintCameraProperties(const camera_properties_t &params)
{
    HAL_LOG_INFO(CONST_MODULE_HAL, "CAMERA_PROPERTIES_T : \n");
    HAL_LOG_INFO(CONST_MODULE_HAL, "    brightness : %d\n", params.stGetData.data[PROPERTY_BRIGHTNESS][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    contrast : %d\n", params.stGetData.data[PROPERTY_CONTRAST][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    saturation : %d\n", params.stGetData.data[PROPERTY_SATURATION][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    hue : %d\n", params.stGetData.data[PROPERTY_HUE][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    auto white balance temp : : %d\n",
                 params.stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    gamma : %d\n", params.stGetData.data[PROPERTY_GAMMA][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    gain : %d\n", params.stGetData.data[PROPERTY_GAIN][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    frequency : %d\n", params.stGetData.data[PROPERTY_FREQUENCY][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    white balance temp : %d\n",
                 params.stGetData.data[PROPERTY_WHITEBALANCETEMPERATURE][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    sharpness : %d\n", params.stGetData.data[PROPERTY_SHARPNESS][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    backlight compensation : %d\n",
                 params.stGetData.data[PROPERTY_BACKLIGHTCOMPENSATION][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    auto exposure : %d\n", params.stGetData.data[PROPERTY_AUTOEXPOSURE][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    exposure : %d\n", params.stGetData.data[PROPERTY_EXPOSURE][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    pan : %d\n", params.stGetData.data[PROPERTY_PAN][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    tilt : %d\n", params.stGetData.data[PROPERTY_TILT][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    Absolute focus : %d\n", params.stGetData.data[PROPERTY_FOCUSABSOLUTE][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    auto focus : %d\n", params.stGetData.data[PROPERTY_AUTOFOCUS][QUERY_VALUE]);
    HAL_LOG_INFO(CONST_MODULE_HAL, "    zoom : %d\n", params.stGetData.data[PROPERTY_ZOOMABSOLUTE][QUERY_VALUE]);
}

void writeImageToFile(const void *p, int size)
{
    FILE *fp;
    char image_name[100] = {};

    snprintf(image_name, 100, "/tmp/Picture%d.yuv", random_value++);
    if (nullptr == (fp = fopen(image_name, "wb")))
    {
        HAL_LOG_INFO(CONST_MODULE_HAL, "fopen failed\n");
        return;
    }
    fwrite(p, size, 1, fp);
    fclose(fp);
}

int main(int argc, char const *argv[])
{
    void *p_h_camera;
    int timeout = 2000;

    camera_hal_if_init(&p_h_camera, subsystem);
    camera_hal_if_open_device(p_h_camera, devname);

    stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
    streamformat.pixel_format    = CAMERA_PIXEL_FORMAT_YUYV;
    streamformat.stream_height   = 480;
    streamformat.stream_width    = 640;
    streamformat.stream_fps      = fps_30;
    camera_hal_if_set_format(p_h_camera, streamformat);
    camera_hal_if_get_format(p_h_camera, &streamformat);
    PrintStreamFormat(streamformat);

    camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_MMAP, nullptr);
    camera_hal_if_start_capture(p_h_camera);

    // poll on fd and read data
    int fd = 0;
    camera_hal_if_get_fd(p_h_camera, &fd);
    struct pollfd fds[] = {
        {.fd = fd, .events = POLLIN},
    };
    buffer_t frame_buffer;
    // just to verify deinit, disabled while loop
    int ret = poll(fds, 1, timeout);
    if (ret > 0)
    {
        camera_hal_if_get_buffer(p_h_camera, &frame_buffer);
        writeImageToFile(frame_buffer.start, frame_buffer.length);
        camera_hal_if_release_buffer(p_h_camera, frame_buffer);
    }

    camera_hal_if_stop_capture(p_h_camera);
    camera_hal_if_destroy_buffer(p_h_camera);

    camera_properties_t out_params;
    camera_hal_if_get_properties(p_h_camera, &out_params);
    PrintCameraProperties(out_params);

    camera_properties_t *in_params = &out_params;
    in_params->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_VALUE] = DEFAULT_BRIGHTNESS;
    in_params->stGetData.data[PROPERTY_CONTRAST][QUERY_VALUE] = DEFAULT_CONTRAST;
    in_params->stGetData.data[PROPERTY_SATURATION][QUERY_VALUE] = DEFAULT_SATURATION;
    in_params->stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_VALUE] = DEFAULT_AUTOWHITEBALANCE;

    camera_hal_if_set_properties(p_h_camera, in_params);

    camera_properties_t out_params_n;
    camera_hal_if_get_properties(p_h_camera, &out_params_n);
    PrintCameraProperties(out_params_n);

    camera_hal_if_close_device(p_h_camera);
    camera_hal_if_deinit(p_h_camera);

    return 0;
}
