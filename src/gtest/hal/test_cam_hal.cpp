// Copyright (c) 2014-2018 LG Electronics, Inc.
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

TEST(CameraHAL, Init)
{
  void *p_h_camera;
  int retval = camera_hal_if_init(&p_h_camera, subsystem);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, Deinit)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  int retval = camera_hal_if_deinit(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, OpenDevice)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  int retval = camera_hal_if_open_device(p_h_camera, devname);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_open_device(p_h_camera, devname);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_OPEN, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, CloseDevice)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  int retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_CLOSE, retval);
}

TEST(CameraHAL, SetFormat)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, GetFormat)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_get_format(p_h_camera, &streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, SetBufferDMA)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_DMABUF);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, SetBufferMMAP)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_MMAP);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, SetBufferUSERPTR)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_USERPTR);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  camera_hal_if_destroy_buffer(p_h_camera);
  camera_hal_if_close_device(p_h_camera);
}

TEST(CameraHAL, StartCaptureDMA)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_DMABUF);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_CLOSE, retval);
  retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, StartCaptureUSERPTR)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_USERPTR);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_CLOSE, retval);
  retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, StopCaptureDMA)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_DMABUF);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, StopCaptureUSERPTR)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_USERPTR);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, StartCaptureMMAP)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_MMAP);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_DEVICE_CLOSE, retval);
  retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_destroy_buffer(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, StopCaptureMMAP)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_MMAP);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_destroy_buffer(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, GetReleaseBuffer)
{
  void *p_h_camera;
  camera_hal_if_init(&p_h_camera, subsystem);
  camera_hal_if_open_device(p_h_camera, devname);
  stream_format_t streamformat;
  streamformat.pixel_format = CAMERA_PIXEL_FORMAT_YUYV;
  streamformat.stream_height = 480;
  streamformat.stream_width = 640;
  int retval = camera_hal_if_set_format(p_h_camera, streamformat);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_set_buffer(p_h_camera, 4, IOMODE_DMABUF);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_start_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  buffer_t buf;
  retval = camera_hal_if_get_buffer(p_h_camera, &buf);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_release_buffer(p_h_camera, buf);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_stop_capture(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
  retval = camera_hal_if_close_device(p_h_camera);
  EXPECT_EQ(CAMERA_ERROR_NONE, retval);
}

TEST(CameraHAL, StressTest)
{
  for (int i = 0; i < 10; i++)
  {
    void *p_h_camera;
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
