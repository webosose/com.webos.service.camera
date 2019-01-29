// Copyright (c) 2019 LG Electronics, Inc.
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
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

const char *subsystem = "libv4l2-camera-plugin.so";
const char *devname = "/dev/video0";

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

void PrintCameraProperties(camera_properties_t params)
{
  HAL_LOG_INFO(CONST_MODULE_HAL, "CAMERA_PROPERTIES_T : \n");
  HAL_LOG_INFO(CONST_MODULE_HAL, "    brightness : %d\n", params.nBrightness);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    contrast : %d\n", params.nContrast);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    saturation : %d\n", params.nSaturation);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    hue : %d\n", params.nHue);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    auto white balance temp : : %d\n", params.nAutoWhiteBalance);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    gamma : %d\n", params.nGamma);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    gain : %d\n", params.nGain);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    frequency : %d\n", params.nFrequency);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    white balance temp : %d\n", params.nWhiteBalanceTemperature);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    sharpness : %d\n", params.nSharpness);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    backlight compensation : %d\n", params.nSharpness);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    auto exposure : %d\n", params.nAutoExposure);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    exposure : %d\n", params.nExposure);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    pan : %d\n", params.nPan);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    tilt : %d\n", params.nTilt);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    Absolute focus : %d\n", params.nFocusAbsolute);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    auto focus : %d\n", params.nAutoFocus);
  HAL_LOG_INFO(CONST_MODULE_HAL, "    zoom : %d\n", params.nZoomAbsolute);
}

void writeImageToFile(const void *p, int size)
{
  FILE *fp;
  char image_name[100] = {};

  snprintf(image_name, 100, "/tmp/Picture%d.yuv", rand());
  if ((fp = fopen(image_name, "wb")) == NULL)
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

  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_get_format(p_h_camera, &streamformat);
  PrintStreamFormat(streamformat);

  camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_USERPTR);
  camera_hal_if_start_capture(p_h_camera);

  // poll on fd and read data
  int fd = 0;
  camera_hal_if_get_fd(p_h_camera, &fd);
  struct pollfd fds[] = {
      {.fd = fd, .events = POLLIN},
  };
  buffer_t frame_buffer;
  // just to verify deinit, disabled while loop
  int ret = poll(fds, 2, timeout);
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
  in_params->nBrightness = DEFAULT_BRIGHTNESS;
  in_params->nContrast = DEFAULT_CONTRAST;
  in_params->nSaturation = DEFAULT_SATURATION;
  in_params->nAutoWhiteBalance = DEFAULT_AUTOWHITEBALANCE;
  camera_hal_if_set_properties(p_h_camera, in_params);

  camera_properties_t out_params_n;
  camera_hal_if_get_properties(p_h_camera, &out_params_n);
  PrintCameraProperties(out_params_n);

  camera_hal_if_close_device(p_h_camera);
  camera_hal_if_deinit(p_h_camera);

  return 0;
}
