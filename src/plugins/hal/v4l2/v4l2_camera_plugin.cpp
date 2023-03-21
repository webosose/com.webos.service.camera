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

#define LOG_CONTEXT "hal"
#define LOG_TAG "V4l2CameraPlugin"
#include "v4l2_camera_plugin.h"
#include "camera_log.h"
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define CLEAR(x) memset(&(x), 0, sizeof(x))
#define CONST_PARAM_DEFAULT_VALUE -999

using namespace std;

void *create_handle() { return new V4l2CameraPlugin; }

void destroy_handle(void *handle)
{
    PLOGI("handle %p ", handle);
    if (handle)
        delete (static_cast<V4l2CameraPlugin *>(handle));
}

V4l2CameraPlugin::V4l2CameraPlugin()
    : stream_format_(), buffers_(nullptr), n_buffers_(0), fd_(CAMERA_ERROR_UNKNOWN), dmafd_(),
      io_mode_(IOMODE_UNKNOWN), fourcc_format_(), camera_format_()
{
    PLOGI("");
}

V4l2CameraPlugin::~V4l2CameraPlugin() { PLOGI(""); }

int V4l2CameraPlugin::openDevice(string devname, string payload)
{
    PLOGI("devname : %s, payload :  %s", devname.c_str(), payload.c_str());
    fd_ = open(devname.c_str(), O_RDWR | O_NONBLOCK);
    if (CAMERA_ERROR_UNKNOWN == fd_)
    {
        PLOGE("cannot open : %s , %d, %s", devname.c_str(), errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }
    createFourCCPixelFormatMap();
    createCameraPixelFormatMap();
    createCameraParamMap();

    PLOGI("fd : %d", fd_);
    return fd_;
}

int V4l2CameraPlugin::closeDevice()
{
    PLOGI("");

    if (CAMERA_ERROR_UNKNOWN == close(fd_))
    {
        PLOGE("cannot close fd: %d , %d, %s", fd_, errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::setFormat(const void *stream_format)
{
    PLOGI("");
    // set width, height and pixel format
    const stream_format_t *in_format = static_cast<const stream_format_t *>(stream_format);
    struct v4l2_format fmt;
    CLEAR(fmt);

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = in_format->stream_width;
    fmt.fmt.pix.height      = in_format->stream_height;
    fmt.fmt.pix.pixelformat = getFourCCPixelFormat(in_format->pixel_format);
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_S_FMT, &fmt))
    {
        PLOGE("VIDIOC_S_FMT failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    if (fmt.fmt.pix.pixelformat != getFourCCPixelFormat(in_format->pixel_format))
    {
        PLOGE("Libv4l didn't accept requested format. Can't proceed.");
        return CAMERA_ERROR_UNKNOWN;
    }

    unsigned int s_width  = in_format->stream_width;
    unsigned int s_height = in_format->stream_height;
    if ((fmt.fmt.pix.width != in_format->stream_width) ||
        (fmt.fmt.pix.height != in_format->stream_height))
    {
        PLOGI("Warning: driver is sending image at : width %d height %d", fmt.fmt.pix.width,
              fmt.fmt.pix.height);
        s_width  = fmt.fmt.pix.width;
        s_height = fmt.fmt.pix.height;
    }

    // set framerate
    struct v4l2_streamparm parm;
    CLEAR(parm);
    parm.type                                  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.timeperframe.numerator   = 1;
    parm.parm.capture.timeperframe.denominator = in_format->stream_fps;
    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_S_PARM, &parm))
    {
        PLOGE("VIDIOC_S_PARM failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    stream_format_.stream_width  = s_width;
    stream_format_.stream_height = s_height;
    stream_format_.pixel_format  = in_format->pixel_format;
    stream_format_.buffer_size   = fmt.fmt.pix.sizeimage;
    stream_format_.stream_fps    = in_format->stream_fps;

    PLOGI("");
    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::getFormat(void *stream_format)
{
    PLOGI("");
    stream_format_t *out_format = static_cast<stream_format_t *>(stream_format);
    struct v4l2_streamparm streamparm;
    CLEAR(streamparm);
    streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_G_PARM, &streamparm))
    {
        PLOGE("VIDIOC_G_PARM failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }
    out_format->stream_fps = streamparm.parm.capture.timeperframe.denominator /
                             streamparm.parm.capture.timeperframe.numerator;

    struct v4l2_format fmt;
    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_G_FMT, &fmt))
    {
        PLOGE("VIDIOC_G_FMT failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }
    out_format->stream_width  = fmt.fmt.pix.width;
    out_format->stream_height = fmt.fmt.pix.height;
    out_format->pixel_format  = getCameraPixelFormat(fmt.fmt.pix.pixelformat);
    out_format->buffer_size   = fmt.fmt.pix.sizeimage;

    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::setBuffer(int num_buffer, int io_mode, void **usrbufs)
{
    PLOGI("num_buffer %d, io_mode %d", num_buffer, io_mode);
    int retVal = CAMERA_ERROR_NONE;
    io_mode_   = io_mode;
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
        retVal = requestUserptrBuffers(num_buffer, (buffer_t **)usrbufs);
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
    PLOGI("retVal : %d", retVal);
    return retVal;
}

int V4l2CameraPlugin::getBuffer(void *outbuf)
{
    buffer_t *out_buf = static_cast<buffer_t *>(outbuf);
    int retVal        = -1;
    struct pollfd fds;

    fds.fd     = fd_;
    fds.events = POLLIN;
    retVal     = poll(&fds, 1, 10000);
    if (0 == retVal)
    {
        PLOGE("POLL timeout!");
        return CAMERA_ERROR_UNKNOWN;
    }
    else if (0 > retVal)
    {
        PLOGE("POLL failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    switch (io_mode_)
    {
    case IOMODE_MMAP:
    {
        struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        retVal = xioctl(fd_, VIDIOC_DQBUF, &buf);
        if (CAMERA_ERROR_NONE != retVal)
        {
            PLOGE("VIDIOC_DQBUF failed %d, %s", errno, strerror(errno));
        }
        out_buf->start  = buffers_[buf.index].start;
        out_buf->length = buf.bytesused;
        out_buf->index  = buf.index;
        break;
    }
    case IOMODE_USERPTR:
    {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        retVal = xioctl(fd_, VIDIOC_DQBUF, &buf);
        if (CAMERA_ERROR_NONE != retVal)
        {
            PLOGE("VIDIOC_DQBUF failed %d, %s", errno, strerror(errno));
        }
        out_buf->start  = buffers_[buf.index].start;
        out_buf->length = buf.bytesused;
        out_buf->index  = buf.index;
        break;
    }
    case IOMODE_DMABUF:
    {
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_DMABUF;

        retVal = xioctl(fd_, VIDIOC_DQBUF, &buf);
        if (CAMERA_ERROR_NONE != retVal)
        {
            PLOGE("VIDIOC_DQBUF failed %d, %s", errno, strerror(errno));
        }
        out_buf->length = buf.length;
        out_buf->index  = buf.index;
        break;
    }
    default:
    {
        break;
    }
    }
    return retVal;
}

int V4l2CameraPlugin::releaseBuffer(const void *inbuf)
{
    const buffer_t *in_buf = static_cast<const buffer_t *>(inbuf);
    switch (io_mode_)
    {
    case IOMODE_USERPTR:
    {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory    = V4L2_MEMORY_USERPTR;
        buf.index     = in_buf->index;
        buf.m.userptr = (unsigned long)buffers_[in_buf->index].start;
        buf.length    = buffers_[in_buf->index].length;

        if (-1 == xioctl(fd_, VIDIOC_QBUF, &buf))
        {
            PLOGE("VIDIOC_QBUF error %d, %s", errno, strerror(errno));
            return CAMERA_ERROR_UNKNOWN;
        }

        break;
    }
    case IOMODE_MMAP:
    case IOMODE_DMABUF:
    {
        // moved the whole original code blocks to this place (definately NOT
        // applicable to user pointers!)
        struct v4l2_buffer buf;
        CLEAR(buf);
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = in_buf->index;

        if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QBUF, &buf))
        {
            PLOGE("VIDIOC_QBUF failed %d, %s", errno, strerror(errno));
            return CAMERA_ERROR_UNKNOWN;
        }

        break;
    }
    default:
        break;
    }

    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::destroyBuffer()
{
    PLOGI("");
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
    case IOMODE_DMABUF:
    {
        retVal = releaseDmaBuffersFd();
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
    PLOGI("");
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
    PLOGI("");
    int retVal = CAMERA_ERROR_NONE;

    switch (io_mode_)
    {
    case IOMODE_USERPTR:
    case IOMODE_MMAP:
    {
        enum v4l2_buf_type type;
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_STREAMOFF, &type))
        {
            PLOGE("VIDIOC_STREAMOFF failed %d, %s", errno, strerror(errno));
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
            PLOGE("VIDIOC_STREAMOFF failed %d, %s", errno, strerror(errno));
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

int V4l2CameraPlugin::setProperties(const void *cam_in_params)
{
    PLOGI("");
    const camera_properties_t *in_params = static_cast<const camera_properties_t *>(cam_in_params);
    int retVal                           = CAMERA_ERROR_NONE;

    std::map<int, int> gIdWithPropertyValue;

    for (int i = 0; i < PROPERTY_END; i++)
    {
        gIdWithPropertyValue[i] = in_params->stGetData.data[i][QUERY_VALUE];
    }

    retVal = setV4l2Property(gIdWithPropertyValue);

    return retVal;
}

int V4l2CameraPlugin::getProperties(void *cam_out_params)
{
    PLOGI("");
    camera_properties_t *out_params = static_cast<camera_properties_t *>(cam_out_params);
    struct v4l2_queryctrl queryctrl;
    int ret = CAMERA_ERROR_UNKNOWN;

    for (int i = 0; i < PROPERTY_END; i++)
    {
        queryctrl.id = camera_param_map_[i];
        if (CAMERA_ERROR_NONE == getV4l2Property(queryctrl, out_params->stGetData.data[i]))
            ret = CAMERA_ERROR_NONE;
    }

    if (errno == ENODEV) // No such device
    {
        return CAMERA_ERROR_UNKNOWN;
    }

    return ret;
}

int V4l2CameraPlugin::getInfo(void *cam_info, std::string devicenode)
{
    PLOGI("");
    camera_device_info_t *out_info = static_cast<camera_device_info_t *>(cam_info);
    int ret                        = CAMERA_ERROR_NONE;
    int fd                         = open(devicenode.c_str(), O_RDWR | O_NONBLOCK, 0);

    if (CAMERA_ERROR_UNKNOWN == fd)
    {
        PLOGE("cannot open : %s , %d, %s", devicenode.c_str(), errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    PLOGI("fd : %d ", fd);

    struct v4l2_fmtdesc format;
    CLEAR(format);

    format.index = 0;
    format.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    struct v4l2_frmsizeenum frmsize;
    CLEAR(frmsize);

    out_info->stResolution.clear();
    while ((-1 != xioctl(fd, VIDIOC_ENUM_FMT, &format)))
    {
        format.index++;
        frmsize.pixel_format = format.pixelformat;
        frmsize.index        = 0;
        struct v4l2_frmivalenum fival;
        CLEAR(fival);
        int res_index = 0;

        std::vector<std::string> v_res;
        while ((-1 != xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize)))
        {
            if (V4L2_FRMSIZE_TYPE_DISCRETE == frmsize.type)
            {
                fival.index        = 0;
                fival.pixel_format = frmsize.pixel_format;
                fival.width        = frmsize.discrete.width;
                fival.height       = frmsize.discrete.height;
                while ((-1 != xioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &fival)))
                {
                    std::string res =
                        std::to_string(frmsize.discrete.width) + "," +
                        std::to_string(frmsize.discrete.height) + "," +
                        std::to_string(fival.discrete.denominator / fival.discrete.numerator);
                    PLOGI("c_res %s ", res.c_str());
                    fival.index++;
                    res_index++;
                    v_res.emplace_back(res);
                    if (res_index >= CONST_MAX_INDEX)
                    {
                        PLOGI("WARN : Exceeded resolution table size!");
                        break;
                    }
                }
            }
            else if (V4L2_FRMSIZE_TYPE_STEPWISE == frmsize.type)
            {
                std::string res = std::to_string(frmsize.discrete.width) + "," +
                                  std::to_string(frmsize.discrete.height);
                res_index++;
                v_res.emplace_back(res);
                PLOGI("framesize type : V4L2_FRMSIZE_TYPE_STEPWISE");
            }
            else if (V4L2_FRMSIZE_TYPE_CONTINUOUS == frmsize.type)
            {
                PLOGI("framesize type : V4L2_FRMSIZE_TYPE_CONTINUOUS");
            }

            if (res_index >= CONST_MAX_INDEX)
                break;
            frmsize.index++;
        }
        out_info->stResolution.emplace_back(v_res, getCameraFormatProperty(format));
    }

    out_info->n_devicetype = DEVICE_TYPE_CAMERA;
    out_info->b_builtin    = false;

    close(fd);

    return ret;
}

int V4l2CameraPlugin::setV4l2Property(std::map<int, int> &gIdWithPropertyValue)
{
    struct v4l2_queryctrl queryctrl;
    struct v4l2_control control;

    for (const auto &it : gIdWithPropertyValue)
    {
        if (CONST_PARAM_DEFAULT_VALUE != it.second)
        {
            queryctrl.id = camera_param_map_[it.first];
            if (xioctl(fd_, VIDIOC_QUERYCTRL, &queryctrl) != CAMERA_ERROR_NONE)
            {
                PLOGI("VIDIOC_QUERYCTRL[%d] failed %d, %s", (int)(queryctrl.id), errno,
                      strerror(errno));
                if (errno == EINVAL)
                {
                    continue;
                }
                else
                {
                    return CAMERA_ERROR_UNKNOWN;
                }
            }
            else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
            {
                PLOGE("Requested VIDIOC_QUERYCTRL flags is not supported");
                return CAMERA_ERROR_UNKNOWN;
            }

            CLEAR(control);
            control.id    = queryctrl.id;
            control.value = it.second;
            PLOGI("VIDIOC_S_CTRL[%s] set value:%d ", queryctrl.name, control.value);

            if (xioctl(fd_, VIDIOC_S_CTRL, &control) != CAMERA_ERROR_NONE)
            {
                PLOGE("VIDIOC_S_CTRL[%s] set value:%d, failed %d, %s", queryctrl.name,
                      control.value, errno, strerror(errno));
                if (errno == EINVAL)
                {
                    continue;
                }
                else
                {
                    return CAMERA_ERROR_UNKNOWN;
                }
            }
        }
    }
    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::getV4l2Property(struct v4l2_queryctrl queryctrl, int *getData)
{
    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QUERYCTRL, &queryctrl))
    {
        PLOGW("VIDIOC_QUERYCTRL[%d] failed %d, %s", queryctrl.id, errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }
    else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
    {
        PLOGE("Requested VIDIOC_QUERYCTRL flags is not supported");
        return CAMERA_ERROR_UNKNOWN;
    }

    struct v4l2_control control;
    CLEAR(control);
    control.id = queryctrl.id;
    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_G_CTRL, &control))
    {
        PLOGE("VIDIOC_G_CTRL failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    PLOGI("name=%s min=%d max=%d step=%d default=%d value=%d", queryctrl.name, queryctrl.minimum,
          queryctrl.maximum, queryctrl.step, queryctrl.default_value, control.value);

    getData[0] = queryctrl.minimum;
    getData[1] = queryctrl.maximum;
    getData[2] = queryctrl.step;
    getData[3] = queryctrl.default_value;
    getData[4] = control.value;

    return CAMERA_ERROR_NONE;
}

camera_format_t V4l2CameraPlugin::getCameraFormatProperty(struct v4l2_fmtdesc format)
{
    camera_format_t format_ = CAMERA_FORMAT_UNDEFINED;
    switch (format.pixelformat)
    {
    case V4L2_PIX_FMT_YUYV:
        format_ = CAMERA_FORMAT_YUV;
        break;
    case V4L2_PIX_FMT_MJPEG:
        format_ = CAMERA_FORMAT_JPEG;
        break;
    case V4L2_PIX_FMT_H264:
        format_ = CAMERA_FORMAT_H264ES;
        break;
    }
    return format_;
}

int V4l2CameraPlugin::requestMmapBuffers(int num_buffer)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count  = num_buffer;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_REQBUFS, &req))
    {
        PLOGE("VIDIOC_REQBUFS failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    buffers_ = (buffer_t *)calloc(req.count, sizeof(*buffers_));
    if (!buffers_)
    {
        PLOGE("Out of memory");
        return CAMERA_ERROR_UNKNOWN;
    }

    for (n_buffers_ = 0; n_buffers_ < req.count; ++n_buffers_)
    {
        struct v4l2_buffer buf;
        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = n_buffers_;

        if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QUERYBUF, &buf))
        {
            PLOGE("VIDIOC_QUERYBUF failed %d, %s", errno, strerror(errno));
            return CAMERA_ERROR_UNKNOWN;
        }

        buffers_[n_buffers_].length = buf.length;
        buffers_[n_buffers_].start =
            mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, buf.m.offset);

        if (MAP_FAILED == buffers_[n_buffers_].start)
        {
            PLOGE("mmap failed %d, %s", errno, strerror(errno));
            return CAMERA_ERROR_UNKNOWN;
        }
    }
    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::requestUserptrBuffers(int num_buffer, buffer_t **usrbufs)
{
    struct v4l2_requestbuffers req;
    stream_format_t stream_format;
    int retVal = CAMERA_ERROR_NONE;

    CLEAR(req);

    req.count  = num_buffer;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_USERPTR;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_REQBUFS, &req))
    {
        PLOGE("VIDIOC_REQBUFS failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    buffers_ = (buffer_t *)calloc(req.count, sizeof(*buffers_));
    if (!buffers_)
    {
        PLOGE("out of memory");
        return CAMERA_ERROR_UNKNOWN;
    }
    retVal = getFormat(&stream_format);
    if (CAMERA_ERROR_NONE != retVal)
    {
        PLOGE("getFormat failed %d, %s", errno, strerror(errno));
        return retVal;
    }

    for (n_buffers_ = 0; n_buffers_ < req.count; ++n_buffers_)
    {
        // assign buffers pushed by the user to user pointer buffers
        buffers_[n_buffers_].length = (*usrbufs)[n_buffers_].length;
        buffers_[n_buffers_].start  = (*usrbufs)[n_buffers_].start;
    }

    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::releaseMmapBuffers()
{
    for (unsigned int i = 0; i < n_buffers_; ++i)
    {
        if (CAMERA_ERROR_UNKNOWN == munmap(buffers_[i].start, buffers_[i].length))
        {
            PLOGE("munmap failed %d, %s", errno, strerror(errno));
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
    if (buffers_)
    {
        free(buffers_);
        buffers_ = nullptr;
    }

    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::releaseDmaBuffersFd()
{
    for (unsigned int i = 0; i < n_buffers_; ++i)
    {
        close(dmafd_[i]);
        dmafd_[i] = -1;
    }
    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::captureDataMmapMode()
{
    struct v4l2_buffer buf;

    for (unsigned int i = 0; i < n_buffers_; ++i)
    {
        CLEAR(buf);
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;
        if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QBUF, &buf))
        {
            PLOGE("VIDIOC_QBUF failed %d, %s", errno, strerror(errno));
            return CAMERA_ERROR_UNKNOWN;
        }
    }

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_STREAMON, &type))
    {
        PLOGE("VIDIOC_STREAMON failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::captureDataUserptrMode()
{
    for (unsigned int i = 0; i < n_buffers_; ++i)
    {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory    = V4L2_MEMORY_USERPTR;
        buf.index     = i;
        buf.m.userptr = (unsigned long)buffers_[i].start;
        buf.length    = buffers_[i].length;

        if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QBUF, &buf))
        {
            PLOGE("VIDIOC_QBUF failed %d, %s", errno, strerror(errno));
            return CAMERA_ERROR_UNKNOWN;
        }
    }
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_STREAMON, &type))
    {
        PLOGE("VIDIOC_STREAMON failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }
    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::requestDmabuffers(int num_buffer)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count  = num_buffer;
    req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_REQBUFS, &req))
    {
        PLOGE("VIDIOC_REQBUFS failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }

    // Create buffer now
    struct v4l2_create_buffers bcreate = {0};
    struct v4l2_format fmt             = {0};

    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = stream_format_.stream_width;
    fmt.fmt.pix.height      = stream_format_.stream_height;
    fmt.fmt.pix.pixelformat = getFourCCPixelFormat(stream_format_.pixel_format);
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix.sizeimage   = stream_format_.buffer_size;

    bcreate.memory = V4L2_MEMORY_MMAP;
    bcreate.format = fmt;
    bcreate.count  = req.count;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_CREATE_BUFS, &bcreate))
    {
        PLOGE("VIDIOC_CREATE_BUFS failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }
    for (unsigned int i = 0; i < req.count; i++)
    {
        struct v4l2_buffer qrybuf = {0};

        qrybuf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        qrybuf.memory = V4L2_MEMORY_DMABUF;
        qrybuf.index  = i;

        if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QUERYBUF, &qrybuf))
        {
            PLOGE("VIDIOC_QUERYBUF failed %d, %s", errno, strerror(errno));
            return CAMERA_ERROR_UNKNOWN;
        }
    }
    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::captureDataDmaMode()
{
    struct v4l2_buffer buf;
    for (unsigned int i = 0; i < n_buffers_; ++i)
    {
        CLEAR(buf);
        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = i;

        if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_QBUF, &buf))
        {
            PLOGE("VIDIOC_QBUF failed %d, %s", errno, strerror(errno));
            return CAMERA_ERROR_UNKNOWN;
        }
    }

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_STREAMON, &type))
    {
        PLOGE("VIDIOC_STREAMON failed %d, %s", errno, strerror(errno));
        return CAMERA_ERROR_UNKNOWN;
    }
    return CAMERA_ERROR_NONE;
}

int V4l2CameraPlugin::getBufferFd(int *bufFd, int *count)
{
    struct v4l2_exportbuffer expbuf;
    *count = 0;

    for (unsigned int i = 0; i < n_buffers_; ++i)
    {
        CLEAR(expbuf);
        expbuf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        expbuf.index = i;
        expbuf.plane = 0;
        expbuf.flags = O_CLOEXEC | O_RDWR;

        if (CAMERA_ERROR_NONE != xioctl(fd_, VIDIOC_EXPBUF, &expbuf))
        {
            PLOGE("VIDIOC_EXPBUF failed %d, %s", errno, strerror(errno));
            return CAMERA_ERROR_UNKNOWN;
        }
        dmafd_[i] = expbuf.fd;
        *bufFd    = expbuf.fd;
        bufFd++;
        *count = *count + 1;
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

void V4l2CameraPlugin::createCameraParamMap()
{
    camera_param_map_.insert(std::make_pair(PROPERTY_BRIGHTNESS, V4L2_CID_BRIGHTNESS));
    camera_param_map_.insert(std::make_pair(PROPERTY_CONTRAST, V4L2_CID_CONTRAST));
    camera_param_map_.insert(std::make_pair(PROPERTY_SATURATION, V4L2_CID_SATURATION));
    camera_param_map_.insert(std::make_pair(PROPERTY_HUE, V4L2_CID_HUE));
    camera_param_map_.insert(
        std::make_pair(PROPERTY_AUTOWHITEBALANCE, V4L2_CID_AUTO_WHITE_BALANCE));
    camera_param_map_.insert(std::make_pair(PROPERTY_GAMMA, V4L2_CID_GAMMA));
    camera_param_map_.insert(std::make_pair(PROPERTY_GAIN, V4L2_CID_GAIN));
    camera_param_map_.insert(std::make_pair(PROPERTY_FREQUENCY, V4L2_CID_POWER_LINE_FREQUENCY));
    camera_param_map_.insert(std::make_pair(PROPERTY_SHARPNESS, V4L2_CID_SHARPNESS));
    camera_param_map_.insert(
        std::make_pair(PROPERTY_BACKLIGHTCOMPENSATION, V4L2_CID_BACKLIGHT_COMPENSATION));
    camera_param_map_.insert(std::make_pair(PROPERTY_AUTOEXPOSURE, V4L2_CID_EXPOSURE_AUTO));
    camera_param_map_.insert(std::make_pair(PROPERTY_PAN, V4L2_CID_PAN_ABSOLUTE));
    camera_param_map_.insert(std::make_pair(PROPERTY_TILT, V4L2_CID_TILT_ABSOLUTE));
    camera_param_map_.insert(std::make_pair(PROPERTY_AUTOFOCUS, V4L2_CID_FOCUS_AUTO));
    camera_param_map_.insert(std::make_pair(PROPERTY_ZOOMABSOLUTE, V4L2_CID_ZOOM_ABSOLUTE));
    camera_param_map_.insert(
        std::make_pair(PROPERTY_WHITEBALANCETEMPERATURE, V4L2_CID_WHITE_BALANCE_TEMPERATURE));
    camera_param_map_.insert(std::make_pair(PROPERTY_EXPOSURE, V4L2_CID_EXPOSURE_ABSOLUTE));
    camera_param_map_.insert(std::make_pair(PROPERTY_FOCUSABSOLUTE, V4L2_CID_FOCUS_ABSOLUTE));
}

unsigned long V4l2CameraPlugin::getFourCCPixelFormat(camera_pixel_format_t camera_format)
{
    unsigned long pixel_format                                  = V4L2_PIX_FMT_YUYV;
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

camera_pixel_format_t V4l2CameraPlugin::getCameraPixelFormat(unsigned long fourcc_format)
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

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
extern "C"
{
    IPlugin *plugin_init(void)
    {
        Plugin *plg = new Plugin();
        plg->setName("V4L2 Hal");
        plg->setDescription("V4L2 Camera HAL");
        plg->setCategory("HAL");
        plg->setVersion("1.0.0");
        plg->setOrganization("LG Electronics.");
        plg->registerFeature<V4l2CameraPlugin>("v4l2");

        return plg;
    }

    void __attribute__((constructor)) plugin_load(void)
    {
        printf("%s:%s\n", __FILENAME__, __PRETTY_FUNCTION__);
    }

    void __attribute__((destructor)) plugin_unload(void)
    {
        printf("%s:%s\n", __FILENAME__, __PRETTY_FUNCTION__);
    }
}
