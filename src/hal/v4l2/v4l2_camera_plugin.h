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

#ifndef V4L2_CAMERA_PLUGIN
#define V4L2_CAMERA_PLUGIN

#include <camera_base.h>
#include <iostream>
#include <linux/videodev2.h>
#include <map>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

  void *create_handle();
  void destroy_handle(void *);

  class V4l2CameraPlugin : public CameraBase
  {
  public:
    V4l2CameraPlugin();

    virtual int openDevice(std::string) override;
    virtual int closeDevice() override;
    virtual int setFormat(stream_format_t) override;
    virtual int getFormat(stream_format_t *) override;
    virtual int setBuffer(int, int) override;
    virtual int getBuffer(buffer_t *) override;
    virtual int releaseBuffer(buffer_t) override;
    virtual int destroyBuffer() override;
    virtual int startCapture() override;
    virtual int stopCapture() override;
    virtual int setProperties(const camera_properties_t *) override;
    virtual int getProperties(camera_properties_t *) override;
    virtual int getInfo(camera_device_info_t *, std::string) override;
    virtual int getBufferFd(int *, int *) override;

  private:
    int setV4l2Property(std::map <int,int>);
    int getV4l2Property(struct v4l2_queryctrl, int *);
    void getCameraFormatProperty(struct v4l2_fmtdesc, camera_properties_t *);
    void getResolutionProperty(camera_properties_t *);

    int requestMmapBuffers(int);
    int requestUserptrBuffers(int);
    int releaseMmapBuffers();
    int releaseUserptrBuffers();
    int captureDataMmapMode();
    int captureDataUserptrMode();

    int requestDmabuffers(int);
    int captureDataDmaMode();
    int releaseDmaBuffersFd();

    void createFourCCPixelFormatMap();
    void createCameraPixelFormatMap();
    unsigned long getFourCCPixelFormat(camera_pixel_format_t);
    camera_pixel_format_t getCameraPixelFormat(int);

    static int xioctl(int, int, void *);

    // member variables
    stream_format_t stream_format_;
    buffer_t *buffers_;
    unsigned int n_buffers_;
    int fd_;
    int dmafd_[CONST_MAX_BUFFER_NUM];
    int io_mode_;
    std::map<camera_pixel_format_t, unsigned long> fourcc_format_;
    std::map<unsigned long, camera_pixel_format_t> camera_format_;
  };

#ifdef __cplusplus
}
#endif

#endif
