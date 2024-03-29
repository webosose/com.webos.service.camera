// Copyright (c) 2019-2023 LG Electronics, Inc.
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

#include "camera_hal_types.h"
#include "camera_hal_if_types.h"
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

    virtual int openDevice(std::string devname) override;
    virtual int closeDevice() override;
    virtual int setFormat(const void *stream_format) override;
    virtual int getFormat(void *stream_format) override;
    virtual int setBuffer(int num_buffer, int io_mode, void **usrbufs) override;
    virtual int getBuffer(void *outbuf) override;
    virtual int releaseBuffer(const void *inbuf) override;
    virtual int destroyBuffer() override;
    virtual int startCapture() override;
    virtual int stopCapture() override;
    virtual int setProperties(const void *cam_in_param) override;
    virtual int getProperties(void *cam_out_param) override;
    virtual int getInfo(void *cam_info, std::string devicenode) override;
    virtual int getBufferFd(int *bufFd, int *count) override;

  private:
    int setV4l2Property(std::map <int,int> &);
    int getV4l2Property(struct v4l2_queryctrl, int *);
    camera_format_t getCameraFormatProperty(struct v4l2_fmtdesc);

    int requestMmapBuffers(unsigned int);
    int requestUserptrBuffers(unsigned int, buffer_t **);
    int releaseMmapBuffers();
    int releaseUserptrBuffers();
    int captureDataMmapMode();
    int captureDataUserptrMode();

    int requestDmabuffers(unsigned int);
    int captureDataDmaMode();
    int releaseDmaBuffersFd();
    int requestBuffersToV4l2(unsigned int count, unsigned int type, unsigned int memory);

    void createFourCCPixelFormatMap();
    void createCameraPixelFormatMap();
    void createCameraParamMap();
    unsigned long getFourCCPixelFormat(camera_pixel_format_t);
    camera_pixel_format_t getCameraPixelFormat(unsigned long);

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
    std::map<int, unsigned int> camera_param_map_;
  };

#ifdef __cplusplus
}
#endif

#endif
