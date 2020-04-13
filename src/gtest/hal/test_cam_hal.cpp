// Copyright (c) 2019-2020 LG Electronics, Inc.
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

#include <gtest/gtest.h>

#include "camera_hal_if.h"

const char *subsystem = "libv4l2-camera-plugin.so";
const char *devname = "/dev/video0";
const char *devname_1 = "/dev/video1";
const char *plugininvalid = "libcamera_v4l2.so";
const char *pluginvalid = "libcamera_hal.so";

const int height_480 = 480;
const int width_640 = 640;
const int fps_30 = 30;
const int framesize = width_640 * height_480 * 2 + 1024;

const int buffers = 4;

TEST(CameraHAL, Init_Validplugin)
{
  void *p_h_camera = nullptr;
  int retval = camera_hal_if_init(&p_h_camera, subsystem);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, Init_Invalidplugin)
{
  void *p_h_camera = nullptr;
  int retval = camera_hal_if_init(&p_h_camera, plugininvalid);
  EXPECT_EQ(CAMERA_ERROR_PLUGIN_NOT_FOUND, retval);
}

TEST(CameraHAL, Init_ValidpluginInvalidHandle)
{
  void *p_h_camera = nullptr;
  int retval = camera_hal_if_init(&p_h_camera, pluginvalid);
  EXPECT_EQ(CAMERA_ERROR_CREATE_HANDLE, retval);
}

TEST(CameraHAL, Deinit_Validplugin)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  int retval = camera_hal_if_deinit(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, Deinit_Multiplerequest)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_deinit(p_h_camera);
  p_h_camera = NULL;
  int retval = camera_hal_if_deinit(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_DESTROY_HANDLE, retval);
}

TEST(CameraHAL, Deinit_Invalidplugin)
{
  void *p_h_camera = nullptr;
  int retval = camera_hal_if_deinit(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_DESTROY_HANDLE, retval);
}

TEST(CameraHAL, OpenDevice_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  int retval = camera_hal_if_open_device(p_h_camera, devname);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, OpenDevice_Multipleopen)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  int retval = camera_hal_if_open_device(p_h_camera, devname);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_OPEN, retval);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, OpenDevice_Invalidhandle)
{
  int retval = camera_hal_if_open_device(NULL, devname);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_OPEN, retval);
}

TEST(CameraHAL, OpenDevice_Invaliddevicenode)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  int retval = camera_hal_if_open_device(p_h_camera, devname_1);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_OPEN, retval);
}

TEST(CameraHAL, CloseDevice_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  int retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, CloseDevice_Multipleclose)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  camera_hal_if_close_device(p_h_camera);
  int retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_CLOSE, retval);
}

TEST(CameraHAL, CloseDevice_Invalidhandle)
{
  int retval = camera_hal_if_close_device(NULL);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_CLOSE, retval);
}

TEST(CameraHAL, SetFormat_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  // yuv format with resolution 640x480
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  // yuv format with resolution 800x600
  streamformat.stream_height = 600;
  streamformat.stream_width = 800;
  streamformat.stream_fps  = fps_30;
  retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  // yuv format with resolution 1920x1080
  streamformat.stream_height = 1080;
  streamformat.stream_width = 1920;
  streamformat.stream_fps  = fps_30;
  retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  // jpeg format with resolution 640x480
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_JPEG;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  // jpeg format with resolution 640x360
  streamformat.stream_height = 360;
  streamformat.stream_width = 640;
  retval = camera_hal_if_set_format(p_h_camera, streamformat);
  // jpeg format with resolution 1920x1080
  streamformat.stream_height = 1080;
  streamformat.stream_width = 1920;
  retval = camera_hal_if_set_format(p_h_camera, streamformat);
  // unsupported yuv format with resolution 640x220
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 220;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, SetFormat_Invalidhandle)
{
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  int retval = camera_hal_if_set_format(NULL, streamformat);
  EXPECT_EQ(CAMERA_ERROR_SET_FORMAT, retval);
}

TEST(CameraHAL, SetFormat_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_SET_FORMAT, retval);
}

TEST(CameraHAL, GetFormat_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_get_format(p_h_camera, &streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetFormat_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_get_format(NULL, &streamformat);
  EXPECT_EQ(CAMERA_ERROR_GET_FORMAT, retval);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetFormat_Invalidstate)
{
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  int retval = camera_hal_if_get_format(NULL, &streamformat);
  EXPECT_EQ(CAMERA_ERROR_GET_FORMAT, retval);
}

TEST(CameraHAL, SetBufferDMA_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, SetBufferDMA_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_set_buffer(NULL, buffers, IOMODE_DMABUF);
  EXPECT_EQ(CAMERA_ERROR_SET_BUFFER, retval);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, SetBufferDMA_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  EXPECT_EQ(CAMERA_ERROR_SET_BUFFER, retval);
}

TEST(CameraHAL, SetBufferMMAP_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, SetBufferMMAP_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_set_buffer(NULL, buffers, IOMODE_MMAP);
  EXPECT_EQ(CAMERA_ERROR_SET_BUFFER, retval);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, SetBufferMMAP_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  EXPECT_EQ(CAMERA_ERROR_SET_BUFFER, retval);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, SetBufferUSERPTR_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, SetBufferUSERPTR_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_set_buffer(NULL, buffers, IOMODE_USERPTR);
  EXPECT_EQ(CAMERA_ERROR_SET_BUFFER, retval);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, SetBufferUSERPTR_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  int retval = camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  EXPECT_EQ(CAMERA_ERROR_SET_BUFFER, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
}

TEST(CameraHAL, StartCaptureDMA_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  int retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureDMA_Multiplerequest)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  camera_hal_if_start_capture(p_h_camera);
  int retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_START_CAPTURE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureDMA_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  int retval = camera_hal_if_start_capture(NULL);
  EXPECT_EQ(CAMERA_ERROR_START_CAPTURE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureDMA_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  int retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_START_CAPTURE, retval);
}

TEST(CameraHAL, StartCaptureMMAP_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  int retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureMMAP_Multiplerequest)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  camera_hal_if_start_capture(p_h_camera);
  int retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_START_CAPTURE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureMMAP_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  int retval = camera_hal_if_start_capture(NULL);
  EXPECT_EQ(CAMERA_ERROR_START_CAPTURE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureMMAP_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  int retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_START_CAPTURE, retval);
}

TEST(CameraHAL, StartCaptureUSERPTR_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  int retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureUSERPTR_Multiplerequest)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  camera_hal_if_start_capture(p_h_camera);
  int retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_START_CAPTURE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureUSERPTR_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  int retval = camera_hal_if_start_capture(NULL);
  EXPECT_EQ(CAMERA_ERROR_START_CAPTURE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureUSERPTR_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  int retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_START_CAPTURE, retval);
}

TEST(CameraHAL, StopCaptureDMA_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  camera_hal_if_start_capture(p_h_camera);
  int retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureDMA_Multiplerequest)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  camera_hal_if_start_capture(p_h_camera);
  camera_hal_if_stop_capture(p_h_camera);
  int retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_STOP_CAPTURE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureDMA_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  camera_hal_if_start_capture(p_h_camera);
  int retval = camera_hal_if_stop_capture(NULL);
  EXPECT_EQ(CAMERA_ERROR_STOP_CAPTURE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureDMA_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  int retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_STOP_CAPTURE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureUSERPTR_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  camera_hal_if_start_capture(p_h_camera);
  int retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureUSERPTR_Multiplerequest)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  camera_hal_if_start_capture(p_h_camera);
  camera_hal_if_stop_capture(p_h_camera);
  int retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_STOP_CAPTURE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureUSERPTR_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  camera_hal_if_start_capture(p_h_camera);
  int retval = camera_hal_if_stop_capture(NULL);
  EXPECT_EQ(CAMERA_ERROR_STOP_CAPTURE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureUSERPTR_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  int retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_STOP_CAPTURE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureMMAP_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  camera_hal_if_start_capture(p_h_camera);
  int retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureMMAP_Multiplerequest)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  camera_hal_if_start_capture(p_h_camera);
  camera_hal_if_stop_capture(p_h_camera);
  int retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_STOP_CAPTURE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureMMAP_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  camera_hal_if_start_capture(p_h_camera);
  int retval = camera_hal_if_stop_capture(NULL);
  EXPECT_EQ(CAMERA_ERROR_STOP_CAPTURE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StopCaptureMMAP_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  int retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_STOP_CAPTURE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetBufferMMAP_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  camera_hal_if_start_capture(p_h_camera);

  buffer_t buf;
  buf.start = malloc(framesize);
  int retval = camera_hal_if_get_buffer(p_h_camera, &buf);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  free(buf.start);
  camera_hal_if_release_buffer(p_h_camera, buf);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetBufferMMAP_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  camera_hal_if_start_capture(p_h_camera);

  buffer_t buf;
  int retval = camera_hal_if_get_buffer(NULL, &buf);
  EXPECT_EQ(CAMERA_ERROR_GET_BUFFER, retval);

  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetBufferMMAP_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);

  buffer_t buf;
  int retval = camera_hal_if_get_buffer(p_h_camera, &buf);
  EXPECT_EQ(CAMERA_ERROR_GET_BUFFER, retval);

  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetBufferDMA_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  camera_hal_if_start_capture(p_h_camera);

  buffer_t buf;
  int retval = camera_hal_if_get_buffer(p_h_camera, &buf);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_release_buffer(p_h_camera, buf);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetBufferDMA_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);
  camera_hal_if_start_capture(p_h_camera);

  buffer_t buf;
  int retval = camera_hal_if_get_buffer(NULL, &buf);
  EXPECT_EQ(CAMERA_ERROR_GET_BUFFER, retval);

  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetBufferDMA_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_DMABUF);

  buffer_t buf;
  int retval = camera_hal_if_get_buffer(p_h_camera, &buf);
  EXPECT_EQ(CAMERA_ERROR_GET_BUFFER, retval);

  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetBufferUSERPTR_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  camera_hal_if_start_capture(p_h_camera);

  buffer_t buf;
  int retval = camera_hal_if_get_buffer(p_h_camera, &buf);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_release_buffer(p_h_camera, buf);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetBufferUSERPTR_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  streamformat.stream_fps  = fps_30;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);
  camera_hal_if_start_capture(p_h_camera);

  buffer_t buf;
  int retval = camera_hal_if_get_buffer(NULL, &buf);
  EXPECT_EQ(CAMERA_ERROR_GET_BUFFER, retval);

  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetBufferUSERPTR_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_USERPTR);

  buffer_t buf;
  int retval = camera_hal_if_get_buffer(p_h_camera, &buf);
  EXPECT_EQ(CAMERA_ERROR_GET_BUFFER, retval);

  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, ReleaseBuffer_Validparameters)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  camera_hal_if_start_capture(p_h_camera);
  buffer_t buf;
  buf.start = malloc(framesize);
  camera_hal_if_get_buffer(p_h_camera, &buf);
  free(buf.start);

  int retval = camera_hal_if_release_buffer(p_h_camera, buf);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, ReleaseBuffer_Invalidhandle)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);
  camera_hal_if_start_capture(p_h_camera);
  buffer_t buf;
  buf.start = malloc(framesize);
  camera_hal_if_get_buffer(p_h_camera, &buf);
  free(buf.start);

  int retval = camera_hal_if_release_buffer(NULL, buf);
  EXPECT_EQ(CAMERA_ERROR_RELEASE_BUFFER, retval);
  camera_hal_if_release_buffer(p_h_camera, buf);
  camera_hal_if_stop_capture(p_h_camera);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, ReleaseBuffer_Invalidstate)
{
  void *p_h_camera = nullptr;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = height_480;
  streamformat.stream_width = width_640;
  camera_hal_if_set_format(p_h_camera, streamformat);
  camera_hal_if_set_buffer(p_h_camera, buffers, IOMODE_MMAP);

  buffer_t buf;
  buf.start = malloc(framesize);
  camera_hal_if_get_buffer(p_h_camera, &buf);
  free(buf.start);

  int retval = camera_hal_if_release_buffer(p_h_camera, buf);
  EXPECT_EQ(CAMERA_ERROR_RELEASE_BUFFER, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, GetInfo_Validparameters)
{
  camera_device_info_t caminfo;
  int retval = camera_hal_if_get_info(devname, &caminfo);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, GetInfo_Invaliddevicenode)
{
  camera_device_info_t caminfo;
  int retval = camera_hal_if_get_info(devname_1, &caminfo);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, StressTest)
{
  for (int i = 0; i < 10; i++)
  {
    void *p_h_camera = nullptr;
    camera_hal_if_init(&p_h_camera, subsystem);
    camera_hal_if_open_device(p_h_camera, devname);
    camera_hal_if_close_device(p_h_camera);
    camera_hal_if_deinit(p_h_camera);
    p_h_camera = NULL;
  }
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
