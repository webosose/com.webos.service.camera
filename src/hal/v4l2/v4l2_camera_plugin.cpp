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

#include "v4l2_camera_plugin.h"

#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

void *create_handle() { return new V4l2CameraPlugin; }

void destroy_handle(void *handle)
{
  HAL_LOG_INFO(CONST_MODULE_HAL, "destroy_handle : handle %p \n", handle);
  if (handle)
    delete (static_cast<V4l2CameraPlugin *>(handle));
}

V4l2CameraPlugin::V4l2CameraPlugin()
    : buffers_(nullptr), n_buffers_(0), fd_(CAMERA_ERROR_UNKNOWN), io_mode_(IOMODE_UNKNOWN),
      fourcc_format_(), camera_format_(),stream_format_()
{
}

int V4l2CameraPlugin::openDevice(string devname)
{
  fd_ = open(devname.c_str(), O_RDWR);
  if (CAMERA_ERROR_UNKNOWN == fd_)
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "openDevice : cannot open : %s , %d, %s\n", devname.c_str(),
                 errno, strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }
  createFourCCPixelFormatMap();
  createCameraPixelFormatMap();

  return fd_;
}

int V4l2CameraPlugin::closeDevice()
{
  if (CAMERA_ERROR_UNKNOWN == close(fd_))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "closeDevice : cannot close fd: %d , %d, %s\n", fd_, errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }

  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::setFormat(stream_format_t stream_format)
{
  // first set framerate
  struct v4l2_streamparm parm;
  CLEAR(parm);
  parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  parm.parm.capture.timeperframe.numerator = 1;
  parm.parm.capture.timeperframe.denominator = stream_format.stream_fps;

  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_S_PARM, &parm))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "setFormat : VIDIOC_S_PARM failed\n");
    return CAMERA_ERROR_UNKNOWN;
  }

  // set width, height and pixel format
  struct v4l2_format fmt;
  CLEAR(fmt);

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width = stream_format.stream_width;
  fmt.fmt.pix.height = stream_format.stream_height;
  fmt.fmt.pix.pixelformat = getFourCCPixelFormat(stream_format.pixel_format);
  fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_S_FMT, &fmt))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "setFormat : VIDIOC_S_FMT failed\n");
    return CAMERA_ERROR_UNKNOWN;
  }

  if (fmt.fmt.pix.pixelformat != getFourCCPixelFormat(stream_format.pixel_format))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL,
                 "setFormat : Libv4l didn't accept requested format. Can't proceed.\n");
    return CAMERA_ERROR_UNKNOWN;
  }
  if ((fmt.fmt.pix.width != stream_format.stream_width) ||
      (fmt.fmt.pix.height != stream_format.stream_height))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL,
                 "setFormat : Warning: driver is sending image at : width %d height %d\n",
                 fmt.fmt.pix.width, fmt.fmt.pix.height);
    stream_format.stream_width = fmt.fmt.pix.width;
    stream_format.stream_height = fmt.fmt.pix.height;
  }
  stream_format_.stream_width = stream_format.stream_width;
  stream_format_.stream_height = stream_format.stream_height;
  stream_format_.pixel_format = stream_format.pixel_format;
  stream_format_.buffer_size = fmt.fmt.pix.sizeimage;
  stream_format_.stream_fps = stream_format.stream_fps;

  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::getFormat(stream_format_t *stream_format)
{
  struct v4l2_streamparm streamparm;
  CLEAR(streamparm);
  streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_G_PARM, &streamparm))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "getFormat : VIDIOC_G_PARM failed\n");
    return CAMERA_ERROR_UNKNOWN;
  }
  stream_format->stream_fps = streamparm.parm.capture.timeperframe.denominator;

  struct v4l2_format fmt;
  CLEAR(fmt);
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_G_FMT, &fmt))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "getFormat : VIDIOC_G_FMT failed\n");
    return CAMERA_ERROR_UNKNOWN;
  }
  stream_format->stream_width = fmt.fmt.pix.width;
  stream_format->stream_height = fmt.fmt.pix.height;
  stream_format->pixel_format = getCameraPixelFormat(fmt.fmt.pix.pixelformat);
  stream_format->buffer_size = fmt.fmt.pix.sizeimage;

  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::setBuffer(int num_buffer, int io_mode)
{
  int retVal = CAMERA_ERROR_NONE;
  io_mode_ = io_mode;
  n_buffers_ = num_buffer;

  switch (io_mode)
  {
  case IOMODE_MMAP:
  {
    retVal = requestMmapBuffers(num_buffer);
    break;
  }
  case IOMODE_USERPTR:
  {
    retVal = requestUserptrBuffers(num_buffer);
    break;
  }
  case IOMODE_DMABUF:
  {
    retVal = requestDmabuffers(num_buffer);
    break;
  }
  default:
  {
    break;
  }
  }
  return retVal;
}

int V4l2CameraPlugin::getBuffer(buffer_t *outbuf)
{
  struct v4l2_buffer buf;
  CLEAR(buf);
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;

  int retVal = xioctl(fd_, VIDIOC_DQBUF, &buf);
  if (CAMERA_ERROR_NONE != retVal)
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "getBuffer : VIDIOC_DQBUF failed %d, %s\n", errno,
                 strerror(errno));
    return retVal;
  }

  switch (io_mode_)
  {
  case IOMODE_MMAP:
  {
    memcpy(outbuf->start, buffers_[buf.index].start, buf.length);
    outbuf->length = buf.length;
    outbuf->index = buf.index;
    break;
  }
  case IOMODE_USERPTR:
  {
    outbuf->start = (void *)buf.m.userptr;
    outbuf->length = buf.bytesused;
    outbuf->index = buf.index;
    break;
  }
  case IOMODE_DMABUF:
  {
    outbuf->start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buf.m.offset);
    outbuf->length = buf.length;
    outbuf->index = buf.index;
    outbuf->fd = buf.m.fd;
    break;
  }
  default:
  {
    break;
  }
  }
  return retVal;
}

int V4l2CameraPlugin::releaseBuffer(buffer_t inbuf)
{
  struct v4l2_buffer buf;

  CLEAR(buf);
  buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf.memory = V4L2_MEMORY_MMAP;
  buf.index = inbuf.index;

  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QBUF, &buf))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "releaseBuffer : VIDIOC_QBUF failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }

  if (IOMODE_DMABUF == io_mode_)
  {
    munmap(inbuf.start, inbuf.length);
  }

  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::destroyBuffer()
{
  int retVal = CAMERA_ERROR_NONE;

  switch (io_mode_)
  {
  case IOMODE_MMAP:
  {
    retVal = releaseMmapBuffers();
    break;
  }
  case IOMODE_USERPTR:
  {
    retVal = releaseUserptrBuffers();
    break;
  }
  default:
  {
    break;
  }
  }
  return retVal;
}

int V4l2CameraPlugin::startCapture()
{
  int retVal = CAMERA_ERROR_NONE;

  switch (io_mode_)
  {
  case IOMODE_MMAP:
  {
    retVal = captureDataMmapMode();
    break;
  }
  case IOMODE_USERPTR:
  {
    retVal = captureDataUserptrMode();
    break;
  }
  case IOMODE_DMABUF:
  {
    retVal = captureDataDmaMode();
    break;
  }
  default:
  {
    break;
  }
  }
  return retVal;
}

int V4l2CameraPlugin::stopCapture()
{
  int retVal = CAMERA_ERROR_NONE;

  switch (io_mode_)
  {
  case IOMODE_MMAP:
  {
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_STREAMOFF, &type))
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "stopCapture : VIDIOC_STREAMOFF failed %d, %s\n", errno,
                   strerror(errno));
      retVal = CAMERA_ERROR_UNKNOWN;
    }
    break;
  }
  case IOMODE_DMABUF:
  {
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_STREAMOFF, &type))
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "stopCapture : VIDIOC_STREAMOFF failed %d, %s\n", errno,
                   strerror(errno));
      retVal = CAMERA_ERROR_UNKNOWN;
    }
    break;
  }
  default:
  {
    break;
  }
  }
  return retVal;
}

int V4l2CameraPlugin::setProperties(const camera_properties_t *cam_in_params)
{
  int retVal = CAMERA_ERROR_NONE;
  struct v4l2_queryctrl queryctrl;

  if (cam_in_params->nBrightness != variable_initialize)
  {
    queryctrl.id = V4L2_CID_BRIGHTNESS;
    retVal = setV4l2Property(queryctrl, cam_in_params->nBrightness);
  }
  if (cam_in_params->nContrast != variable_initialize)
  {
    queryctrl.id = V4L2_CID_CONTRAST;
    retVal = setV4l2Property(queryctrl, cam_in_params->nContrast);
  }
  if (cam_in_params->nSaturation != variable_initialize)
  {
    queryctrl.id = V4L2_CID_SATURATION;
    retVal = setV4l2Property(queryctrl, cam_in_params->nSaturation);
  }
  if (cam_in_params->nHue != variable_initialize)
  {
    queryctrl.id = V4L2_CID_HFLIP;
    retVal = setV4l2Property(queryctrl, cam_in_params->nHue);
  }
  if (cam_in_params->nAutoWhiteBalance != variable_initialize)
  {
    queryctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
    retVal = setV4l2Property(queryctrl, cam_in_params->nAutoWhiteBalance);
  }
  if (cam_in_params->nGamma != variable_initialize)
  {
    queryctrl.id = V4L2_CID_GAMMA;
    retVal = setV4l2Property(queryctrl, cam_in_params->nGamma);
  }
  if (cam_in_params->nGain != variable_initialize)
  {
    queryctrl.id = V4L2_CID_GAIN;
    retVal = setV4l2Property(queryctrl, cam_in_params->nGain);
  }
  if (cam_in_params->nFrequency != variable_initialize)
  {
    queryctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
    retVal = setV4l2Property(queryctrl, cam_in_params->nFrequency);
  }
  if (cam_in_params->nWhiteBalanceTemperature != variable_initialize)
  {
    queryctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
    retVal = setV4l2Property(queryctrl, cam_in_params->nWhiteBalanceTemperature);
  }
  if (cam_in_params->nSharpness != variable_initialize)
  {
    queryctrl.id = V4L2_CID_SHARPNESS;
    retVal = setV4l2Property(queryctrl, cam_in_params->nSharpness);
  }
  if (cam_in_params->nBacklightCompensation != variable_initialize)
  {
    queryctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
    retVal = setV4l2Property(queryctrl, cam_in_params->nBacklightCompensation);
  }
  if (cam_in_params->nAutoExposure != variable_initialize)
  {
    queryctrl.id = V4L2_CID_EXPOSURE_AUTO;
    retVal = setV4l2Property(queryctrl, cam_in_params->nAutoExposure);
  }
  if (cam_in_params->nExposure != variable_initialize)
  {
    queryctrl.id = V4L2_CID_EXPOSURE;
    retVal = setV4l2Property(queryctrl, cam_in_params->nExposure);
  }
  if (cam_in_params->nPan != variable_initialize)
  {
    queryctrl.id = V4L2_CID_PAN_ABSOLUTE;
    retVal = setV4l2Property(queryctrl, cam_in_params->nPan);
  }
  if (cam_in_params->nTilt != variable_initialize)
  {
    queryctrl.id = V4L2_CID_TILT_ABSOLUTE;
    retVal = setV4l2Property(queryctrl, cam_in_params->nTilt);
  }
  if (cam_in_params->nFocusAbsolute != variable_initialize)
  {
    queryctrl.id = V4L2_CID_FOCUS_ABSOLUTE;
    retVal = setV4l2Property(queryctrl, cam_in_params->nFocusAbsolute);
  }
  if (cam_in_params->nAutoFocus != variable_initialize)
  {
    queryctrl.id = V4L2_CID_FOCUS_AUTO;
    retVal = setV4l2Property(queryctrl, cam_in_params->nAutoFocus);
  }
  if (cam_in_params->nZoomAbsolute != variable_initialize)
  {
    queryctrl.id = V4L2_CID_ZOOM_ABSOLUTE;
    retVal = setV4l2Property(queryctrl, cam_in_params->nZoomAbsolute);
  }

  return retVal;
}

int V4l2CameraPlugin::getProperties(camera_properties_t *cam_out_params)
{
  struct v4l2_queryctrl queryctrl;

  queryctrl.id = V4L2_CID_BRIGHTNESS;
  getV4l2Property(queryctrl, &cam_out_params->nBrightness);

  queryctrl.id = V4L2_CID_CONTRAST;
  getV4l2Property(queryctrl, &cam_out_params->nContrast);

  queryctrl.id = V4L2_CID_SATURATION;
  getV4l2Property(queryctrl, &cam_out_params->nSaturation);

  queryctrl.id = V4L2_CID_HUE;
  getV4l2Property(queryctrl, &cam_out_params->nHue);

  queryctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
  getV4l2Property(queryctrl, &cam_out_params->nAutoWhiteBalance);

  queryctrl.id = V4L2_CID_GAMMA;
  getV4l2Property(queryctrl, &cam_out_params->nGamma);

  queryctrl.id = V4L2_CID_GAIN;
  getV4l2Property(queryctrl, &cam_out_params->nGain);

  queryctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
  getV4l2Property(queryctrl, &cam_out_params->nFrequency);

  queryctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
  getV4l2Property(queryctrl, &cam_out_params->nWhiteBalanceTemperature);

  queryctrl.id = V4L2_CID_SHARPNESS;
  getV4l2Property(queryctrl, &cam_out_params->nSharpness);

  queryctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
  getV4l2Property(queryctrl, &cam_out_params->nBacklightCompensation);

  queryctrl.id = V4L2_CID_EXPOSURE_AUTO;
  getV4l2Property(queryctrl, &cam_out_params->nAutoExposure);

  queryctrl.id = V4L2_CID_EXPOSURE;
  getV4l2Property(queryctrl, &cam_out_params->nExposure);

  queryctrl.id = V4L2_CID_PAN_ABSOLUTE;
  getV4l2Property(queryctrl, &cam_out_params->nPan);

  queryctrl.id = V4L2_CID_TILT_ABSOLUTE;
  getV4l2Property(queryctrl, &cam_out_params->nTilt);

  queryctrl.id = V4L2_CID_FOCUS_ABSOLUTE;
  getV4l2Property(queryctrl, &cam_out_params->nFocusAbsolute);

  queryctrl.id = V4L2_CID_FOCUS_AUTO;
  getV4l2Property(queryctrl, &cam_out_params->nAutoFocus);

  queryctrl.id = V4L2_CID_ZOOM_ABSOLUTE;
  getV4l2Property(queryctrl, &cam_out_params->nZoomAbsolute);

  getResolutionProperty(cam_out_params);

  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::getInfo(camera_device_info_t *cam_info, std::string devicenode)
{
  struct v4l2_format fmt;
  CLEAR(fmt);

  int ret = CAMERA_ERROR_NONE;

  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  struct v4l2_capability cap;
  int fd = open(devicenode.c_str(), O_RDWR | O_NONBLOCK, 0);
  if (CAMERA_ERROR_UNKNOWN == fd)
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "getInfo : cannot open : %s , %d, %s\n", devicenode.c_str(),
                 errno, strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }

  HAL_LOG_INFO(CONST_MODULE_HAL, "getInfo : fd : %d \n", fd);

  if (CAMERA_ERROR_NONE != xioctl(fd, VIDIOC_QUERYCAP, &cap))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "getInfo : VIDIOC_QUERYCAP failed %d, %s\n", errno,
                 strerror(errno));
    ret = CAMERA_ERROR_UNKNOWN;
  }
  if (CAMERA_ERROR_NONE != xioctl(fd, VIDIOC_G_FMT, &fmt))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "getInfo : VIDIOC_G_FMT failed %d, %s\n", errno,
                 strerror(errno));
    ret = CAMERA_ERROR_UNKNOWN;
  }
  if (CAMERA_ERROR_NONE == ret)
  {
    unsigned long width = fmt.fmt.pix.width;   // Image width
    unsigned long height = fmt.fmt.pix.height; // Image height
    int pixelfmt = 0;                          // Pixel format
    struct v4l2_fmtdesc format;
    format.index = 0;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    while ((-1 != xioctl(fd, VIDIOC_ENUM_FMT, &format)))
    {
      format.index++;
      switch (format.pixelformat)
      {
      case V4L2_PIX_FMT_YUYV:
        pixelfmt = pixelfmt | 1;
        break;
      case V4L2_PIX_FMT_MJPEG:
        pixelfmt = pixelfmt | 4;
        break;
      case V4L2_PIX_FMT_H264:
        pixelfmt = pixelfmt | 2;
        break;
      default:
        HAL_LOG_INFO(CONST_MODULE_HAL, "getInfo : pixelfmt : %d \n", pixelfmt);
      }

      cam_info->n_format = pixelfmt;
      cam_info->n_devicetype = DEVICE_TYPE_CAMERA;
      cam_info->b_builtin = false;
      cam_info->n_maxpictureheight = height;
      cam_info->n_maxpicturewidth = width;
      cam_info->n_maxvideoheight = height;
      cam_info->n_maxvideowidth = width;
    }
  }
  close(fd);
  return ret;
}

int V4l2CameraPlugin::setV4l2Property(struct v4l2_queryctrl queryctrl, int value)
{
  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QUERYCTRL, &queryctrl))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "setV4l2Property : VIDIOC_QUERYCTRL failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }
  else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
  {
    HAL_LOG_INFO(CONST_MODULE_HAL,
                 "setV4l2Property : Requested VIDIOC_QUERYCTRL flags is not supported\n");
    return CAMERA_ERROR_UNKNOWN;
  }

  struct v4l2_control control;
  CLEAR(control);
  control.id = queryctrl.id;
  control.value = value;
  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_S_CTRL, &control))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "setV4l2Property : VIDIOC_S_CTRL failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }
  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::getV4l2Property(struct v4l2_queryctrl queryctrl, int *value)
{
  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QUERYCTRL, &queryctrl))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "getV4l2Property : VIDIOC_QUERYCTRL failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }
  else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
  {
    HAL_LOG_INFO(CONST_MODULE_HAL,
                 "getV4l2Property : Requested VIDIOC_QUERYCTRL flags is not supported\n");
    return CAMERA_ERROR_UNKNOWN;
  }

  struct v4l2_control control;
  CLEAR(control);
  control.id = queryctrl.id;
  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_G_CTRL, &control))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "getV4l2Property : VIDIOC_G_CTRL failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }
  *value = control.value;
  return CAMERA_ERROR_NONE;
}

void V4l2CameraPlugin::getResolutionProperty(camera_properties_t *cam_out_params)
{
  int pixelfmt = 0;
  struct v4l2_fmtdesc format;
  CLEAR(format);

  format.index = 0;
  format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  int nformatindex = 0;
  struct v4l2_frmsizeenum frmsize;
  CLEAR(frmsize);
  int ncount = 0;
  while ((-1 != xioctl(fd_, VIDIOC_ENUM_FMT, &format)))
  {
    format.index++;
    switch (format.pixelformat)
    {
    case V4L2_PIX_FMT_YUYV:
      pixelfmt = pixelfmt | 1;
      cam_out_params->st_resolution.e_format[nformatindex] = CAMERA_FORMAT_YUV;
      break;
    case V4L2_PIX_FMT_MJPEG:
      pixelfmt = pixelfmt | 4;
      cam_out_params->st_resolution.e_format[nformatindex] = CAMERA_FORMAT_JPEG;
      break;
    case V4L2_PIX_FMT_H264:
      pixelfmt = pixelfmt | 2;
      cam_out_params->st_resolution.e_format[nformatindex] = CAMERA_FORMAT_H264ES;
      break;
    default:
      HAL_LOG_INFO(CONST_MODULE_HAL, "getResolutionProperty : pixelfmt : %d \n", pixelfmt);
    }
    nformatindex++;
    frmsize.pixel_format = format.pixelformat;
    frmsize.index = 0;
    struct v4l2_frmivalenum fival;
    CLEAR(fival);

    while ((-1 != xioctl(fd_, VIDIOC_ENUM_FRAMESIZES, &frmsize)))
    {
      if (V4L2_FRMSIZE_TYPE_DISCRETE == frmsize.type)
      {
        cam_out_params->st_resolution.n_height[ncount][frmsize.index] = frmsize.discrete.height;
        cam_out_params->st_resolution.n_width[ncount][frmsize.index] = frmsize.discrete.width;
        fival.index = 0;
        fival.pixel_format = frmsize.pixel_format;
        fival.width = frmsize.discrete.width;
        fival.height = frmsize.discrete.height;
        while ((-1 != xioctl(fd_, VIDIOC_ENUM_FRAMEINTERVALS, &fival)))
        {
          snprintf(cam_out_params->st_resolution.c_res[frmsize.index], 20, "%d,%d,%d",
                   frmsize.discrete.width, frmsize.discrete.height, fival.discrete.denominator);
          HAL_LOG_INFO(CONST_MODULE_HAL, "getResolutionProperty c_res %s \n",
                       cam_out_params->st_resolution.c_res[frmsize.index]);
          cam_out_params->st_resolution.n_frameindex[ncount] = frmsize.index;
          break;
        }
      }
      else if (V4L2_FRMSIZE_TYPE_STEPWISE == frmsize.type)
      {
        snprintf(cam_out_params->st_resolution.c_res[frmsize.index], 10, "%d,%d",
                 frmsize.stepwise.max_width, frmsize.stepwise.max_height);
      }
      frmsize.index++;
    }
    ncount++;
    cam_out_params->st_resolution.n_formatindex = nformatindex;
  }
}

int V4l2CameraPlugin::requestMmapBuffers(int num_buffer)
{
  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count = num_buffer;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_REQBUFS, &req))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "requestMmapBuffers : VIDIOC_REQBUFS failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }

  buffers_ = (buffer_t *)calloc(req.count, sizeof(*buffers_));
  if (!buffers_)
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "requestMmapBuffers : Out of memory\n");
    return CAMERA_ERROR_UNKNOWN;
  }

  for (n_buffers_ = 0; n_buffers_ < req.count; ++n_buffers_)
  {
    struct v4l2_buffer buf;
    CLEAR(buf);

    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = n_buffers_;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QUERYBUF, &buf))
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "requestMmapBuffers : VIDIOC_QUERYBUF failed %d, %s\n", errno,
                   strerror(errno));
      return CAMERA_ERROR_UNKNOWN;
    }

    buffers_[n_buffers_].length = buf.length;
    buffers_[n_buffers_].start =
        mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buf.m.offset);

    if (MAP_FAILED == buffers_[n_buffers_].start)
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "requestMmapBuffers : mmap failed %d, %s\n", errno,
                   strerror(errno));
      return CAMERA_ERROR_UNKNOWN;
    }
  }
  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::requestUserptrBuffers(int num_buffer)
{
  struct v4l2_requestbuffers req;
  stream_format_t stream_format;
  int retVal = CAMERA_ERROR_NONE;

  CLEAR(req);

  req.count = num_buffer;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_USERPTR;

  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_REQBUFS, &req))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "requestUserptrBuffers : VIDIOC_REQBUFS failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }

  buffers_ = (buffer_t *)calloc(req.count, sizeof(*buffers_));
  if (!buffers_)
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "requestUserptrBuffers : out of memory\n");
    return CAMERA_ERROR_UNKNOWN;
  }
  retVal = getFormat(&stream_format);
  if (CAMERA_ERROR_NONE != retVal)
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "requestUserptrBuffers : getFormat failed %d, %s\n", errno,
                 strerror(errno));
    return retVal;
  }

  for (n_buffers_ = 0; n_buffers_ < req.count; ++n_buffers_)
  {
    buffers_[n_buffers_].length = stream_format.buffer_size;
    buffers_[n_buffers_].start = malloc(stream_format.buffer_size);

    if (NULL == buffers_[n_buffers_].start)
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "requestUserptrBuffers : malloc failed %d, %s\n", errno,
                   strerror(errno));
      return CAMERA_ERROR_UNKNOWN;
    }
  }
  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::releaseMmapBuffers()
{
  for (int i = 0; i < n_buffers_; ++i)
  {
    if (CAMERA_ERROR_UNKNOWN == munmap(buffers_[i].start, buffers_[i].length))
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "releaseMmapBuffers : munmap failed %d, %s\n", errno,
                   strerror(errno));
      return CAMERA_ERROR_UNKNOWN;
    }
    buffers_[i].start = NULL;
  }
  free(buffers_);
  buffers_ = NULL;
  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::releaseUserptrBuffers()
{
  for (int i = 0; i < n_buffers_; ++i)
  {
    free(buffers_[i].start);
    buffers_[i].start = NULL;
  }
  free(buffers_);
  buffers_ = NULL;
  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::captureDataMmapMode()
{
  struct v4l2_buffer buf;

  for (int i = 0; i < n_buffers_; ++i)
  {
    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = i;
    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QBUF, &buf))
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "captureDataMmapMode : VIDIOC_QBUF failed %d, %s\n", errno,
                   strerror(errno));
      return CAMERA_ERROR_UNKNOWN;
    }
  }

  enum v4l2_buf_type type;
  type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_STREAMON, &type))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "captureDataMmapMode : VIDIOC_STREAMON failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }

  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::captureDataUserptrMode()
{
  for (int i = 0; i < n_buffers_; ++i)
  {
    struct v4l2_buffer buf;

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_USERPTR;
    buf.index = i;
    buf.m.userptr = (unsigned long)buffers_[i].start;
    buf.length = buffers_[i].length;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QBUF, &buf))
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "captureDataUserptrMode : VIDIOC_QBUF failed %d, %s\n", errno,
                   strerror(errno));
      return CAMERA_ERROR_UNKNOWN;
    }
  }
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_STREAMON, &type))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "captureDataUserptrMode : VIDIOC_STREAMON failed %d, %s\n",
                 errno, strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }
  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::requestDmabuffers(int num_buffer)
{
  struct v4l2_requestbuffers req;

  CLEAR(req);

  req.count = num_buffer;
  req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  req.memory = V4L2_MEMORY_MMAP;

  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_REQBUFS, &req))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "requestDmabuffers : VIDIOC_REQBUFS failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }
  return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::captureDataDmaMode()
{
  struct v4l2_exportbuffer expbuf;
  struct v4l2_buffer buf;

  int dmafd[n_buffers_];

  for (int i = 0; i < n_buffers_; ++i)
  {
    CLEAR(expbuf);

    expbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    expbuf.index = i;
    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_EXPBUF, &expbuf))
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "captureDataDmaMode : VIDIOC_EXPBUF failed %d, %s\n", errno,
                   strerror(errno));
      return CAMERA_ERROR_UNKNOWN;
    }
    dmafd[i] = expbuf.fd;

    CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.m.fd = dmafd[i];
    buf.index = i;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QBUF, &buf))
    {
      HAL_LOG_INFO(CONST_MODULE_HAL, "captureDataDmaMode : VIDIOC_QBUF failed %d, %s\n", errno,
                   strerror(errno));
      return CAMERA_ERROR_UNKNOWN;
    }
  }

  int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

  if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_STREAMON, &type))
  {
    HAL_LOG_INFO(CONST_MODULE_HAL, "captureDataDmaMode : VIDIOC_STREAMON failed %d, %s\n", errno,
                 strerror(errno));
    return CAMERA_ERROR_UNKNOWN;
  }
  return CAMERA_ERROR_NONE;
}

void V4l2CameraPlugin::createFourCCPixelFormatMap()
{
  fourcc_format_.insert(std::make_pair(CAMERA_PIXEL_FORMAT_NV12, V4L2_PIX_FMT_NV12));
  fourcc_format_.insert(std::make_pair(CAMERA_PIXEL_FORMAT_NV21, V4L2_PIX_FMT_NV21));
  fourcc_format_.insert(std::make_pair(CAMERA_PIXEL_FORMAT_I420, V4L2_PIX_FMT_SN9C20X_I420));
  fourcc_format_.insert(std::make_pair(CAMERA_PIXEL_FORMAT_YUYV, V4L2_PIX_FMT_YUYV));
  fourcc_format_.insert(std::make_pair(CAMERA_PIXEL_FORMAT_UYVY, V4L2_PIX_FMT_UYVY));
  fourcc_format_.insert(std::make_pair(CAMERA_PIXEL_FORMAT_BGRA8888, V4L2_PIX_FMT_ABGR32));
  fourcc_format_.insert(std::make_pair(CAMERA_PIXEL_FORMAT_ARGB8888, V4L2_PIX_FMT_ARGB32));
  fourcc_format_.insert(std::make_pair(CAMERA_PIXEL_FORMAT_JPEG, V4L2_PIX_FMT_MJPEG));
  fourcc_format_.insert(std::make_pair(CAMERA_PIXEL_FORMAT_H264, V4L2_PIX_FMT_H264));
}

void V4l2CameraPlugin::createCameraPixelFormatMap()
{
  camera_format_.insert(std::make_pair(V4L2_PIX_FMT_NV12, CAMERA_PIXEL_FORMAT_NV12));
  camera_format_.insert(std::make_pair(V4L2_PIX_FMT_NV21, CAMERA_PIXEL_FORMAT_NV21));
  camera_format_.insert(std::make_pair(V4L2_PIX_FMT_SN9C20X_I420, CAMERA_PIXEL_FORMAT_I420));
  camera_format_.insert(std::make_pair(V4L2_PIX_FMT_YUYV, CAMERA_PIXEL_FORMAT_YUYV));
  camera_format_.insert(std::make_pair(V4L2_PIX_FMT_UYVY, CAMERA_PIXEL_FORMAT_UYVY));
  camera_format_.insert(std::make_pair(V4L2_PIX_FMT_ABGR32, CAMERA_PIXEL_FORMAT_BGRA8888));
  camera_format_.insert(std::make_pair(V4L2_PIX_FMT_ARGB32, CAMERA_PIXEL_FORMAT_ARGB8888));
  camera_format_.insert(std::make_pair(V4L2_PIX_FMT_MJPEG, CAMERA_PIXEL_FORMAT_JPEG));
  camera_format_.insert(std::make_pair(V4L2_PIX_FMT_H264, CAMERA_PIXEL_FORMAT_H264));
}

unsigned long V4l2CameraPlugin::getFourCCPixelFormat(camera_pixel_format_t camera_format)
{
  unsigned long pixel_format = V4L2_PIX_FMT_YUYV;
  std::map<camera_pixel_format_t, unsigned long>::iterator it = fourcc_format_.begin();
  while (it != fourcc_format_.end())
  {
    if (it->first == camera_format)
    {
      pixel_format = it->second;
      break;
    }
    ++it;
  }
  return pixel_format;
}

camera_pixel_format_t V4l2CameraPlugin::getCameraPixelFormat(int fourcc_format)
{
  camera_pixel_format_t camera_format = CAMERA_PIXEL_FORMAT_YUYV;

  std::map<unsigned long, camera_pixel_format_t>::iterator it = camera_format_.begin();
  while (it != camera_format_.end())
  {
    if (it->first == fourcc_format)
    {
      camera_format = it->second;
      break;
    }
    ++it;
  }
  return camera_format;
}

int V4l2CameraPlugin::xioctl(int fh, int request, void *arg)
{
  int ret = CAMERA_ERROR_NONE;

  do
  {
    ret = ioctl(fh, request, arg);
  } while (ret == CAMERA_ERROR_UNKNOWN && ((errno == EINTR) || (errno == EAGAIN)));

  return ret;
}
