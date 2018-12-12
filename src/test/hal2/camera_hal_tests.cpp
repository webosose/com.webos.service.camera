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

#include <camera_hal_if.h>
#include <stdio.h>
#include <poll.h>
#include <pthread.h>

const char *subsystem = "libv4l2-camera-plugin.so";
const char *devname = "/dev/video0";

//default values
#define DEFAULT_BRIGHTNESS 101
#define DEFAULT_CONTRAST 7
#define DEFAULT_SATURATION 62
#define DEFAULT_AUTOWHITEBALANCE 1

void PrintStreamFormat(stream_format_t streamformat)
{
    DLOG(printf("StreamFormat : \n"););
    DLOG(printf("    Pixel Format : %d\n",streamformat.pixel_format););
    DLOG(printf("    Width : %d\n",streamformat.stream_width););
    DLOG(printf("    Height : %d\n",streamformat.stream_height););
}

void PrintCameraProperties(camera_properties_t params)
{
    DLOG(printf("CAMERA_PROPERTIES_T : \n"););
    DLOG(printf("    brightness : %d\n",params.nBrightness););
    DLOG(printf("    contrast : %d\n",params.nContrast););
    DLOG(printf("    saturation : %d\n",params.nSaturation););
    DLOG(printf("    hue : %d\n",params.nHue););
    DLOG(printf("    auto white balance temp : : %d\n",params.nAutoWhiteBalance););
    DLOG(printf("    gamma : %d\n",params.nGamma););
    DLOG(printf("    gain : %d\n",params.nGain););
    DLOG(printf("    frequency : %d\n",params.nFrequency););
    DLOG(printf("    white balance temp : %d\n",params.nWhiteBalanceTemperature););
    DLOG(printf("    sharpness : %d\n",params.nSharpness););
    DLOG(printf("    backlight compensation : %d\n",params.nSharpness););
    DLOG(printf("    auto exposure : %d\n",params.nAutoExposure););
    DLOG(printf("    exposure : %d\n",params.nExposure););
    DLOG(printf("    pan : %d\n",params.nPan););
    DLOG(printf("    tilt : %d\n",params.nTilt););
    DLOG(printf("    Absolute focus : %d\n",params.nFocusAbsolute););
    DLOG(printf("    auto focus : %d\n",params.nAutoFocus););
    DLOG(printf("    zoom : %d\n",params.nZoomAbsolute););
}

void writeImageToFile(const void *p,int size)
{
    FILE *fp;
    static int num = 0;
    char image_name[20];

    sprintf(image_name, "outimage%d.yuv", num++);
    if((fp = fopen(image_name, "wb")) == NULL)
    {
        DLOG(perror("Fail to fopen"););
        return ;
    }
    fwrite(p, size, 1, fp);
    fclose(fp);
}

void* Camera1(void *arg)
{
    int retval = 0;
    void *p_h_camera;
    int timeout = 2000;

    retval = camera_hal_if_init(&p_h_camera, subsystem);

    retval = camera_hal_if_open_device(p_h_camera, devname);

    stream_format_t streamformat;
    streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
    streamformat.stream_height = 480;
    streamformat.stream_width = 640;
    retval = camera_hal_if_set_format(p_h_camera, streamformat);

    retval = camera_hal_if_get_format(p_h_camera, &streamformat);
    PrintStreamFormat(streamformat);

    retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_DMABUF);

    retval = camera_hal_if_start_capture(p_h_camera);

    //poll on fd and read data
    int ret = 0;
    int fd = 0;
    retval = camera_hal_if_get_fd(p_h_camera,&fd);
    struct pollfd fds[] = {
        { .fd = fd, .events = POLLIN },
    };
    buffer_t frame_buffer;
    //just to verify deinit, disabled while loop
    if((ret = poll(fds, 2, timeout)) > 0)
    {
        retval = camera_hal_if_get_buffer(p_h_camera,&frame_buffer);
        writeImageToFile(frame_buffer.start,frame_buffer.length);
        retval = camera_hal_if_release_buffer(p_h_camera,frame_buffer);
    }

    retval = camera_hal_if_stop_capture(p_h_camera);
    retval = camera_hal_if_destroy_buffer(p_h_camera);

    camera_properties_t out_params;
    retval = camera_hal_if_get_properties(p_h_camera,&out_params);
    PrintCameraProperties(out_params);

    camera_properties_t *in_params = &out_params;
    in_params->nBrightness = DEFAULT_BRIGHTNESS;
    in_params->nContrast = DEFAULT_CONTRAST;
    in_params->nSaturation = DEFAULT_SATURATION;
    in_params->nAutoWhiteBalance = DEFAULT_AUTOWHITEBALANCE;
    retval = camera_hal_if_set_properties(p_h_camera,in_params);

    camera_properties_t out_params_n;
    retval = camera_hal_if_get_properties(p_h_camera,&out_params_n);
    PrintCameraProperties(out_params_n);

    retval = camera_hal_if_close_device(p_h_camera);

    retval = camera_hal_if_deinit(p_h_camera);

}

int main(int argc, char const *argv[])
{
    int error;
    pthread_t tid;
    error = pthread_create(&tid, NULL, &Camera1, NULL);

    pthread_join( tid, NULL);

    return 0;
}
