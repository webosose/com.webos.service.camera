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

/** @file ddi_cam.c
 *
 *  Device Driver Interface functions for Camera
 *
 */

/******************************************************************************
 File Inclusions
 ******************************************************************************/
#include "hal.h"
#include "hal_cam_internal.h"
#include "hal_v4l2.h"

#ifdef __cplusplus
extern "C"
{
#endif

/******************************************************************************
 Static Variables & Function Prototypes Declarations
 ******************************************************************************/

CAM_DEVICE_T gCameraDeviceList[10];

#ifdef STREAM_DUMP
static FILE *g_StreamDump[CONST_MAX_DEVICE_COUNT];
#endif

static int xioctl(int fd, int request, void *arg);

DEVICE_RETURN_CODE_T _v4l2_ConnectMemoryMapping(CAM_DEVICE_T *pDevice);
DEVICE_RETURN_CODE_T _v4l2_ConnectCapCheck(CAM_DEVICE_T *pDevice);
DEVICE_RETURN_CODE_T _v4l2_ConnectRequestBuffers(CAM_DEVICE_T *pDevice);
DEVICE_RETURN_CODE_T _v4l2_connect(CAM_DEVICE_T *pDevice);
DEVICE_RETURN_CODE_T _v4l2_disconnect(CAM_DEVICE_T *pDevice);
DEVICE_RETURN_CODE_T _v4l2_streamon(CAM_DEVICE_T *pDevice);
DEVICE_RETURN_CODE_T _v4l2_memoryunmapping(CAM_DEVICE_T *pDevice);
DEVICE_RETURN_CODE_T _lockers_init(CAM_DEVICE_T* pDevice);
int _camera_init(char * strDeviceName);
static int gCamCount;
/******************************************************************************
 Function Definitions
 ******************************************************************************/
DEVICE_RETURN_CODE_T _v4l2_dequebuffer(CAM_DEVICE_T *pDevice, struct v4l2_buffer *pBuf)
{
    memset(pBuf, 0, sizeof(struct v4l2_buffer));
    pBuf->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    pBuf->memory = V4L2_MEMORY_MMAP;
    if (-1 == xioctl(pDevice->hCamfd, VIDIOC_DQBUF, pBuf))
    {
        switch (errno)
        {
        case EAGAIN: // no data in the buffer
            PMLOG_INFO(CONST_MODULE_HAL, "%d: EAGAIN : no data in the buffer (can not recover)!!",
                    __LINE__);
            return DEVICE_ERROR_UNKNOWN;
            //case EIO:
        default:
            PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_DQBUF error (%d) %s !!", __LINE__, errno,
                    strerror(errno));
            return DEVICE_ERROR_UNKNOWN;
        }
    }

    if (pBuf->index >= pDevice->nMMbuffers)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "buf.index >= n_buffers !!", __LINE__);
        return DEVICE_ERROR_UNKNOWN;
    }

    return DEVICE_OK;

}

DEVICE_RETURN_CODE_T _v4l2_QueBuffer(CAM_DEVICE_T *pDevice, struct v4l2_buffer *pBuf)
{
    if (-1 == xioctl(pDevice->hCamfd, VIDIOC_QBUF, pBuf))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_QBUF error (%d) %s !!", __LINE__, errno,
                strerror(errno));
        return DEVICE_ERROR_UNKNOWN;
    }

    return DEVICE_OK;
}
void *CaptureThread(void *arg)
{
    CAM_DEVICE_T *pDevice = (CAM_DEVICE_T *) arg;

    DEVICE_RETURN_CODE_T val = DEVICE_OK;
    struct v4l2_buffer vidbuf;

    while (pDevice->isDeviceOpen)
    {
        while (pDevice->isStreamOn)
        {
            val = _v4l2_dequebuffer(pDevice, &vidbuf);
            if (val == DEVICE_OK)
            {
                // Deque buffer
                if(pDevice->isCapturing != true)
                {
                    pDevice->pDataCB(pDevice->cameraNum, ST_YUY2, vidbuf.bytesused,
                            (unsigned char *) (pDevice->hMMbuffers[vidbuf.index].start), 0);
                }
                val = _v4l2_QueBuffer(pDevice, &vidbuf);
            }
            if ((val == DEVICE_ERROR_UNKNOWN))
                break;
        }

    }

    pDevice->isStreamOn = 0;
    pDevice->isDeviceOpen = 0;

    pthread_mutex_lock(&pDevice->hCaptureThread);
    pthread_cond_signal(&pDevice->hCaptureThreadCond);
    pthread_mutex_unlock(&pDevice->hCaptureThread);

    if (pDevice->CameraStatus == NO_DEVICE)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "Device is unplugged. callback don't work.");
    }
    else
    {
        if (val == DEVICE_ERROR_UNKNOWN)
        {
            PMLOG_INFO(CONST_MODULE_HAL, "%d: error to get camera data, destroy capture thread!",
                    __LINE__);
        }
    }
    pthread_detach(pDevice->loop_thread);

    return NULL;
}

DEVICE_RETURN_CODE_T _v4l2_streamoff(CAM_DEVICE_T *pDevice)
{
    PMLOG_INFO(CONST_MODULE_HAL, "%d: Stop.\n", __LINE__);

    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(pDevice->hCamfd, VIDIOC_STREAMOFF, &type))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_STREAMOFF error (%d) %s !!", __LINE__, errno,
                strerror(errno));
        return DEVICE_ERROR_CAN_NOT_STOP;
    }
    pDevice->isStreamOn = 0;

    PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_STREAMOFF OK.", __LINE__);

    return DEVICE_OK;

}
DEVICE_RETURN_CODE_T _v4l2_streamon(CAM_DEVICE_T *pDevice)
{
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;
    unsigned int i;
    for (i = 0; i < pDevice->nMMbuffers; i++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (-1 == xioctl(pDevice->hCamfd, VIDIOC_QUERYBUF, &buf))
        {
            PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_QUERYBUF error (%d) %s !!", __LINE__, errno,
                    strerror(errno));
            return DEVICE_ERROR_CAN_NOT_START;
        }
        if (-1 == xioctl(pDevice->hCamfd, VIDIOC_QBUF, &buf))
        {
            PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_QBUF error (%d) %s !!", __LINE__, errno,
                    strerror(errno));
            return DEVICE_ERROR_CAN_NOT_START;
        }
        PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_QBUF OK.", __LINE__);
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (-1 == xioctl(pDevice->hCamfd, VIDIOC_STREAMON, &type))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_STREAMON error (%d) %s !!", __LINE__, errno,
                strerror(errno));
        return DEVICE_ERROR_CAN_NOT_START;
    }
    pDevice->isStreamOn = 1;
    pDevice->bCameraStreamDebugOn[0] = pDevice->bCameraStreamDebugOn[1] =
            pDevice->bCameraStreamDebugOn[2] = 1;

    PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_STREAMON OK.\n", __LINE__);
    PMLOG_INFO(CONST_MODULE_HAL, "%d: pDevice->isStreamOn:%d\n", __LINE__, pDevice->isStreamOn);

    pDevice->CameraStatus = STREAM_ON;
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T v4l2_cam_stop_capture(char *strDeviceName)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int cameraNum = 0;
    cameraNum = _camera_init(strDeviceName);

    if (gCameraDeviceList[cameraNum].isCapturing)
    {
        gCameraDeviceList[cameraNum].isCapturing = false;
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Capture isn't happening currently for the device\n",
                __LINE__);
        ret = DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;
    }
    return ret;
}
void *capturing_thread(void *arg)
{
    CAM_DEVICE_T *pDevice = (CAM_DEVICE_T *) arg;
    DEVICE_RETURN_CODE_T ret;
    CAMERA_FORMAT sCurrentFormat;

    sCurrentFormat.nWidth = pDevice->nVideoWidth;
    sCurrentFormat.nHeight = pDevice->nVideoHeight;
    sCurrentFormat.eFormat = pDevice->nVideoMode;
    while (pDevice->isCapturing)
    {
        ret = v4l2_cam_capture_image(pDevice->strDeviceName, 1,sCurrentFormat);
        if (ret != DEVICE_OK)
        {
            PMLOG_INFO(CONST_MODULE_HAL, "%d: start capture failed\n", __LINE__);
            break;
        }
    }
    pDevice->isCapturing = false;
    pthread_detach(pDevice->capture_thread);

    return NULL;
}
DEVICE_RETURN_CODE_T v4l2_cam_start_capture(char *strDeviceName, CAMERA_FORMAT sFormat)
{
    DEVICE_RETURN_CODE_T ret;
    int nThreadErr;
    int cameraNum = 0;

    cameraNum = _camera_init(strDeviceName);
    if (gCameraDeviceList[cameraNum].isCapturing != true)
    {
        gCameraDeviceList[cameraNum].isCapturing = true;
        if ((nThreadErr = pthread_create(&gCameraDeviceList[cameraNum].capture_thread, NULL,
                capturing_thread, &gCameraDeviceList[cameraNum])) != 0)
        {
            PMLOG_INFO(CONST_MODULE_HAL, "%d create thread failed\n", __LINE__);
            ret = DEVICE_ERROR_UNKNOWN;
        }
    }
    else
    {
        ret = DEVICE_ERROR_DEVICE_IS_ALREADY_STARTED;
    }
    return ret;
}
DEVICE_RETURN_CODE_T v4l2_cam_get_format(char *strDeviceName, CAMERA_FORMAT *sFormat)
{
    struct v4l2_format fmt;
    int cameraNum = 0;

    cameraNum = _camera_init(strDeviceName);
    CAM_DEVICE_T *pDevice = &gCameraDeviceList[cameraNum];

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_FMT, &fmt))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_S_FMT error again.(%d) %s !!", __LINE__, errno,
                strerror(errno));
        return DEVICE_ERROR_WRONG_PARAM;
    }
    if (fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV)
        sFormat->eFormat == CAMERA_FORMAT_YUV;
    else if (fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG)
        sFormat->eFormat == CAMERA_FORMAT_JPEG;
    if (fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264)
        sFormat->eFormat == CAMERA_FORMAT_H264ES;
    else
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: invalid format\n", __LINE__);
        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
    }
    sFormat->nWidth = fmt.fmt.pix.width;
    sFormat->nHeight = fmt.fmt.pix.height;
    pDevice->nVideoWidth = sFormat->nWidth;
    pDevice->nVideoHeight = sFormat->nHeight;
    return DEVICE_OK;
}
DEVICE_RETURN_CODE_T v4l2_cam_set_format(char *strDeviceName, CAMERA_FORMAT sFormat)
{
    struct v4l2_format fmt;
    int cameraNum = 0;

    cameraNum = _camera_init(strDeviceName);
    CAM_DEVICE_T *pDevice = &gCameraDeviceList[cameraNum];

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    fmt.fmt.pix.width = sFormat.nWidth;
    fmt.fmt.pix.height = sFormat.nHeight;
    if (sFormat.eFormat == CAMERA_FORMAT_YUV)
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV; //0; //v4l2_fourcc('H','2','6','4'); //V4L2_PIX_FMT_H264;
    else if (sFormat.eFormat == CAMERA_FORMAT_JPEG)
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG; //0; //v4l2_fourcc('H','2','6','4'); //V4L2_PIX_FMT_H264;
    else if (sFormat.eFormat == CAMERA_FORMAT_H264ES)
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264; //0; //v4l2_fourcc('H','2','6','4'); //V4L2_PIX_FMT_H264;
    else
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: invalid format\n", __LINE__);
        return DEVICE_ERROR_WRONG_PARAM;
    }
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_FMT, &fmt))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_S_FMT error again.(%d) %s !!", __LINE__, errno,
                strerror(errno));
        return DEVICE_ERROR_WRONG_PARAM;
    }
    pDevice->nVideoMode = sFormat.eFormat;
    pDevice->nVideoWidth = sFormat.nWidth;
    pDevice->nVideoHeight = sFormat.nHeight;
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T v4l2_cam_capture_image(char *strDeviceName, int nCount, CAMERA_FORMAT sFormat)
{
    FILE *fp;
    char filename[100];
    int i;
    struct v4l2_buffer vidbuf;
    int cameraNum = 0;
    CAMERA_FORMAT sCurrentFormat;
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    bool bFormatChanged = false;

    cameraNum = _camera_init(strDeviceName);
    PMLOG_DEBUG(CONST_MODULE_HAL, "%s:%d : cameraNum:%d\n", __FUNCTION__, __LINE__, cameraNum);
    CAM_DEVICE_T *pDevice = &gCameraDeviceList[cameraNum];
    pDevice->isCapturing = true;

    sCurrentFormat.nWidth = pDevice->nVideoWidth;
    sCurrentFormat.nHeight = pDevice->nVideoHeight;
    sCurrentFormat.eFormat = pDevice->nVideoMode;

    if((sFormat.nWidth != sCurrentFormat.nWidth) ||
            (sFormat.nHeight != sCurrentFormat.nHeight) ||
            (sFormat.eFormat != sCurrentFormat.eFormat))
    {
        PMLOG_DEBUG(CONST_MODULE_HAL, "%d:Format change\n",__LINE__);
        PMLOG_DEBUG(CONST_MODULE_HAL, "%d:strDeviceName:%s\n",__LINE__,strDeviceName);
        ret = v4l2_cam_stop(strDeviceName);
        if (ret == DEVICE_OK)
        {
            PMLOG_DEBUG(CONST_MODULE_HAL, "v4l2_cam_stop success\n");
            ret = v4l2_cam_set_format(strDeviceName, sFormat);
            if (ret == DEVICE_OK)
            {
                bFormatChanged = true;
                PMLOG_DEBUG(CONST_MODULE_HAL, "v4l2_cam_set_format success\n");
            }
            else
            {
                /*if set format for new resoultion fails then we reset the
                 * format to the preview format */
                ret = v4l2_cam_set_format(strDeviceName, sCurrentFormat);
                if (ret != DEVICE_OK)
                {
                    PMLOG_INFO(CONST_MODULE_HAL,"set format failed hence starting with default parameters\n");
                }
                bFormatChanged = false;
            }
        }
        ret = v4l2_cam_start(strDeviceName);
        if (ret == DEVICE_OK)
        {
            PMLOG_INFO(CONST_MODULE_HAL, "v4l2_cam_start success\n");
        }
        else
        {
            pDevice->isCapturing = false;
            return ret;
        }
    }

    for (i = 1; i <= nCount; i++)
    {
        _v4l2_dequebuffer(pDevice, &vidbuf);

        if (gCameraDeviceList[cameraNum].nVideoMode == CAMERA_FORMAT_YUV)
        {
            sprintf(filename, "/tmp/Picture%d.yuv", rand());
        }
        else if (gCameraDeviceList[cameraNum].nVideoMode == CAMERA_FORMAT_JPEG)
        {
            sprintf(filename, "/tmp/Picture%d.jpeg", rand());
        }
        else if (gCameraDeviceList[cameraNum].nVideoMode == CAMERA_FORMAT_H264ES)
        {
            sprintf(filename, "/tmp/Picture%d.h264", rand());
        }
        PMLOG_INFO(CONST_MODULE_HAL, "picture will save at this file path:%s\n", filename);
        fp = fopen(filename, "a");

        fwrite((unsigned char *) (pDevice->hMMbuffers[vidbuf.index].start), 1, vidbuf.bytesused,
                fp);
        fclose(fp);

        _v4l2_QueBuffer(pDevice, &vidbuf);
    }

    if(bFormatChanged == true)
    {
        PMLOG_DEBUG(CONST_MODULE_HAL, "%dFormat change\n",__LINE__);
        ret = v4l2_cam_stop(strDeviceName);
        if (ret == DEVICE_OK)
        {
            PMLOG_DEBUG(CONST_MODULE_HAL, "streamoff success\n");
            ret = v4l2_cam_set_format(strDeviceName, sCurrentFormat);
            if (ret == DEVICE_OK)
            {
                PMLOG_INFO(CONST_MODULE_HAL, "setFormat success\n");
            }
            else
            {
                PMLOG_INFO(CONST_MODULE_HAL,"set format failed,starting the camera with default format\n");
            }
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_HAL,"camera stop failed,starting the camera with default format\n");
        }
        ret = v4l2_cam_start(strDeviceName);
        if (ret == DEVICE_OK)
        {
            PMLOG_DEBUG(CONST_MODULE_HAL, "streamon success\n");
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_HAL,"camera start failed\n");
            pDevice->isCapturing = false;
            return ret;
        }
        bFormatChanged = false;
    }
    pDevice->isCapturing = false;
    return ret;
}

DEVICE_RETURN_CODE_T v4l2_cam_get_list(int *cameraCount, int cameraType[])
{
    char buffer[256];
    struct stat buf;
    int i = 0;
    *cameraCount = 0;

    while (stat(buffer, &buf) == 0)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "buffer:%s\n\n", buffer);
        *cameraCount = *cameraCount + 1;
        cameraType[i] = CAMERA_TYPE_V4L2;
        i = i + 1;
        snprintf(buffer, 32, "/dev/video%d", i);
    }
    return DEVICE_OK;

}

DEVICE_RETURN_CODE_T v4l2_cam_get_property(char *strDeviceName, CAMERA_PROPERTIES_INDEX_T nProperty,
        int *value)
{
    struct v4l2_control control;
    int errno;
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    struct v4l2_queryctrl queryctrl;
    int cameraNum = 0;

    cameraNum = _camera_init(strDeviceName);
    CAM_DEVICE_T *pDevice = &gCameraDeviceList[cameraNum];
    switch (nProperty)
    {
        case CAMERA_PROPERTIES_AUTOWHITEBALANCE:
            {
                queryctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_AUTO_WHITE_BALANCE;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_AUTOEXPOSURE:
            {
                queryctrl.id = V4L2_CID_AUTOGAIN;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_AUTOGAIN;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_BACKLIGHT_COMPENSATION:
            {
                queryctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_BACKLIGHT_COMPENSATION;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_EXPOSURE:
            {
                queryctrl.id = V4L2_CID_EXPOSURE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_EXPOSURE;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_MIRROR:
            {
                queryctrl.id = V4L2_CID_HFLIP;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_HFLIP;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_HUE:
            {
                queryctrl.id = V4L2_CID_HUE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_HUE;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_SATURATION:
            {
                queryctrl.id = V4L2_CID_SATURATION;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_SATURATION;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_PAN:
            {
                queryctrl.id = V4L2_CID_PAN_ABSOLUTE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_PAN_ABSOLUTE;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_GAMMA:
            {
                queryctrl.id = V4L2_CID_GAMMA;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_GAMMA;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_GAIN:
            {
                queryctrl.id = V4L2_CID_GAIN;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_GAIN;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_FREQUENCY:
            {
                queryctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_POWER_LINE_FREQUENCY;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_WHITEBALANCETEMPERATURE:
            {
                queryctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_SHARPNESS:
            {
                queryctrl.id = V4L2_CID_SHARPNESS;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_SHARPNESS;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_TILT:
            {
                queryctrl.id = V4L2_CID_TILT_ABSOLUTE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_TILT_ABSOLUTE;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_CONTRAST:
            {
                queryctrl.id = V4L2_CID_CONTRAST;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_CONTRAST;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        case CAMERA_PROPERTIES_BRIGHTNESS:
            {
                queryctrl.id = V4L2_CID_BRIGHTNESS;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_BRIGHTNESS;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_G_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                *value = control.value;
                break;
            }

        default:
            {
                PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Invalid Property\n", __LINE__, __FUNCTION__);
                ret = DEVICE_ERROR_UNSUPPORTED_FORMAT;
                break;
            }
    }
    return ret;
}

DEVICE_RETURN_CODE_T v4l2_cam_set_property(char *strDeviceName, CAMERA_PROPERTIES_INDEX_T nProperty,
        int value)
{
    struct v4l2_control control;
    int errno;
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int cameraNum = 0;

    cameraNum = _camera_init(strDeviceName);
    CAM_DEVICE_T *pDevice = &gCameraDeviceList[cameraNum];
    struct v4l2_queryctrl queryctrl;

    switch (nProperty)
    {
        case CAMERA_PROPERTIES_AUTOWHITEBALANCE:
            {
                queryctrl.id = V4L2_CID_AUTO_WHITE_BALANCE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_AUTO_WHITE_BALANCE;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_AUTOEXPOSURE:
            {
                queryctrl.id = V4L2_CID_AUTOGAIN;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_AUTOGAIN;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_BACKLIGHT_COMPENSATION:
            {
                queryctrl.id = V4L2_CID_BACKLIGHT_COMPENSATION;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_BACKLIGHT_COMPENSATION;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_EXPOSURE:
            {
                queryctrl.id = V4L2_CID_EXPOSURE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_EXPOSURE;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_MIRROR:
            {
                queryctrl.id = V4L2_CID_HFLIP;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_HFLIP;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_HUE:
            {
                queryctrl.id = V4L2_CID_HUE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_HUE;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_SATURATION:
            {
                queryctrl.id = V4L2_CID_SATURATION;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_SATURATION;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_PAN:
            {
                queryctrl.id = V4L2_CID_PAN_ABSOLUTE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_PAN_ABSOLUTE;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_GAMMA:
            {
                queryctrl.id = V4L2_CID_GAMMA;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_GAMMA;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_GAIN:
            {
                queryctrl.id = V4L2_CID_GAIN;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_GAIN;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_FREQUENCY:
            {
                queryctrl.id = V4L2_CID_POWER_LINE_FREQUENCY;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_POWER_LINE_FREQUENCY;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_WHITEBALANCETEMPERATURE:
            {
                queryctrl.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_WHITE_BALANCE_TEMPERATURE;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_SHARPNESS:
            {
                queryctrl.id = V4L2_CID_SHARPNESS;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_SHARPNESS;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_TILT:
            {
                queryctrl.id = V4L2_CID_TILT_ABSOLUTE;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_TILT_ABSOLUTE;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_CONTRAST:
            {
                queryctrl.id = V4L2_CID_CONTRAST;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported \n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }
                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_CONTRAST;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        case CAMERA_PROPERTIES_BRIGHTNESS:
            {
                queryctrl.id = V4L2_CID_BRIGHTNESS;
                if (-1 == ioctl(pDevice->hCamfd, VIDIOC_QUERYCTRL, &queryctrl))
                {
                    if (errno != EINVAL)
                    {
                        PMLOG_INFO(CONST_MODULE_HAL,
                                "%d:%s : Requested property is not supported is not supported\n", __LINE__,
                                __FUNCTION__);
                        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                    }
                }
                else if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Requested property is not supported\n", __LINE__,
                            __FUNCTION__);
                    return DEVICE_ERROR_UNSUPPORTED_FORMAT;
                }

                memset(&control, 0, sizeof(control));
                control.id = V4L2_CID_BRIGHTNESS;
                control.value = value;
                if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_CTRL, &control))
                {
                    PMLOG_INFO(CONST_MODULE_HAL, "%d:%s set format error\n", __LINE__, __FUNCTION__);
                    ret = DEVICE_ERROR_UNKNOWN;
                }
                break;
            }

        default:
            {
                PMLOG_INFO(CONST_MODULE_HAL, "%d:%s Invalid Property\n", __LINE__, __FUNCTION__);
                ret = DEVICE_ERROR_UNSUPPORTED_FORMAT;
                break;
            }
    }
    return ret;
}

DEVICE_RETURN_CODE_T v4l2_cam_get_info(char *strDeviceName, CAMERA_INFO_T *pInfo)
{
    char buf[256] =
    { 0 };
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int fd = 0;
    struct v4l2_format fmt;
    struct v4l2_fmtdesc format;
    struct v4l2_capability cap;
    size_t width = 0; //Image width
    size_t height = 0; //Image height
    size_t imageSize = 0; // Total image size in bytes
    int pixelFmt = 0; // Pixel format
    int cameraNum = 0;
    int i;
    struct v4l2_frmsizeenum frmsize;
    struct v4l2_frmivalenum frmival;

    cameraNum = _camera_init(strDeviceName);
    i = cameraNum;
    strcpy(buf, gCameraDeviceList[i].strDeviceName);

    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    //snprintf (buf,32,"/dev/video%d",i);

    //CAM_DEVICE_INFO_T *CInfo = &g_CameraInfoList[0];
    fd = open(buf, O_RDWR | O_NONBLOCK, 0);
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Querycap failed\n", __LINE__);
        ret = DEVICE_ERROR_UNKNOWN;
    }
    if (-1 == xioctl(fd, VIDIOC_G_FMT, &fmt))
    {
        ret = DEVICE_ERROR_UNKNOWN;
        PMLOG_INFO(CONST_MODULE_HAL, "%d:VIDIOC_G_FMT failed\n", __LINE__);
    }
    if (DEVICE_OK == ret)
    {
        width = fmt.fmt.pix.width; //Image width
        height = fmt.fmt.pix.height; //Image height
        imageSize = fmt.fmt.pix.sizeimage; // Total image size in bytes
        format.index = 0;
        format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

        while ((-1 != xioctl(fd, VIDIOC_ENUM_FMT, &format)))
        {

            format.index++;

            switch (format.pixelformat)
            {
                case V4L2_PIX_FMT_YUYV:
                    pixelFmt = pixelFmt | CAMERA_FORMAT_YUV;
                    PMLOG_INFO(CONST_MODULE_HAL, " & format: YuYv\n");
                    break;
                case V4L2_PIX_FMT_MJPEG:
                    pixelFmt = pixelFmt | CAMERA_FORMAT_JPEG;
                    PMLOG_INFO(CONST_MODULE_HAL, " & format: MJPEG\n");
                    break;
                case V4L2_PIX_FMT_H264:
                    pixelFmt = pixelFmt | CAMERA_FORMAT_H264ES;
                    PMLOG_INFO(CONST_MODULE_HAL, " & format: H264\n");
                    break;
                case V4L2_PIX_FMT_RGB24:
                    PMLOG_INFO(CONST_MODULE_HAL, " & format: RGB24\n");
                    break;
                default:
                    PMLOG_INFO(CONST_MODULE_HAL, " & format: %u\n", pixelFmt);
            }
            PMLOG_INFO(CONST_MODULE_HAL, "{ pixelformat =''%c%c%c%c'', description = ''%s'' }/n",

                    format.pixelformat & 0xFF, (format.pixelformat >> 8) & 0xFF,

                    (format.pixelformat >> 16) & 0xFF, (format.pixelformat >> 24) & 0xFF,

                    format.description);


            frmsize.pixel_format = format.pixelformat;
            frmsize.index = 0;
            while (ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) >= 0)
            {
                if (frmsize.type == V4L2_FRMSIZE_TYPE_DISCRETE)
                {
                    PMLOG_INFO(CONST_MODULE_HAL,"%dx%d\n",
                            frmsize.discrete.width,
                            frmsize.discrete.height);
                }
                else if (frmsize.type == V4L2_FRMSIZE_TYPE_STEPWISE)
                {
                    PMLOG_INFO(CONST_MODULE_HAL,"%dx%d\n",
                            frmsize.stepwise.max_width,
                            frmsize.stepwise.max_height);
                }
                frmsize.index++;
            }
            PMLOG_INFO(CONST_MODULE_HAL, "Camera name:%s\n\n", cap.card);
            PMLOG_INFO(CONST_MODULE_HAL, "pixelFmt value:%d\n\n", pixelFmt);
            pInfo->nFormat = pixelFmt;
            pInfo->nType = DEVICE_CAMERA;
            pInfo->bBuiltin = false;
            strncpy(pInfo->strName, (char *) cap.card, 32);
            pInfo->nMaxPictureHeight = height;
            pInfo->nMaxPictureWidth = width;
            pInfo->nMaxVideoHeight = height;
            pInfo->nMaxVideoWidth = width;
        }
    }

    close(fd);
    return ret;

}

DEVICE_RETURN_CODE_T v4l2_cam_stop(char *strDeviceName)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int camCnt = 0;

    camCnt = _camera_init(strDeviceName);

    if (gCameraDeviceList[camCnt].isStreamOn == 0)
    {
        //DDI_CAM_ERR_PRINT("Already started.");
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Already started.", __LINE__);
    }
    if ((ret = _v4l2_streamoff(&gCameraDeviceList[camCnt])) != DEVICE_OK)
    {
        // start thread
        PMLOG_INFO(CONST_MODULE_HAL, "%d: StreamOn failed: error (%d) %s !!", __LINE__, errno,
                strerror(errno));
    }
    ret = _v4l2_memoryunmapping(&gCameraDeviceList[camCnt]);
    ret = _v4l2_disconnect(&gCameraDeviceList[camCnt]);
    ret = _v4l2_connect(&gCameraDeviceList[camCnt]);
    if ((ret != DEVICE_OK))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Camera connect FAILED !!\n", __LINE__);
        return ret;
    }
    PMLOG_INFO(CONST_MODULE_HAL, "%d: status at the end of start:%d\n\n",
            gCameraDeviceList[camCnt].isStreamOn);
    return ret;
}

DEVICE_RETURN_CODE_T v4l2_cam_start(char *strDeviceName)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int camCnt = 0;

    camCnt = _camera_init(strDeviceName);

    if (gCameraDeviceList[camCnt].isStreamOn == 1)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Already started.", __LINE__);
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STARTED;
    }
    ret = _v4l2_ConnectCapCheck(&gCameraDeviceList[camCnt]);
    if ((ret != DEVICE_OK))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Connect cap check failed\n", __LINE__);
        return ret;
    }
    if (ret == DEVICE_OK
            && ((ret = _v4l2_ConnectRequestBuffers(&gCameraDeviceList[camCnt])) != DEVICE_OK))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Connect request buffers failed\n", __LINE__);
        return ret;
    }
    if (ret == DEVICE_OK
            && ((ret = _v4l2_ConnectMemoryMapping(&gCameraDeviceList[camCnt])) != DEVICE_OK))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Connect memory mapping failed\n", __LINE__);
        return ret;
    }
    if ((ret = _v4l2_streamon(&gCameraDeviceList[camCnt])) != DEVICE_OK)
    {
        // start thread
        PMLOG_INFO(CONST_MODULE_HAL, "%d: StreamOn failed: error (%d) %s !!", __LINE__, errno,
                strerror(errno));
    }
    PMLOG_INFO(CONST_MODULE_HAL, "%d: status at the end of start:%d\n\n",
            gCameraDeviceList[camCnt].isStreamOn);
    return ret;
}

DEVICE_RETURN_CODE_T v4l2_cam_close(char *strDeviceName)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int camCnt = 0;

    camCnt = _camera_init(strDeviceName);
    /*Exiting the thread*/
    gCameraDeviceList[camCnt].isDeviceOpen = false;
    gCameraDeviceList[camCnt].cameraNum = camCnt;
    gCameraDeviceList[camCnt].nVideoWidth = 0;
    gCameraDeviceList[camCnt].nVideoHeight = 0;
    gCameraDeviceList[camCnt].nVideoMode = CAMERA_FORMAT_YUV;
    if (gCameraDeviceList[camCnt].isStreamOn = true)
    {
        gCameraDeviceList[camCnt].isStreamOn = false;

    }
    ret = _v4l2_disconnect(&gCameraDeviceList[camCnt]);

    return ret;
}

int _camera_init(char * strDeviceName)
{
    int i = 0;

    for (i = 0; i < gCamCount; i++)
    {
        if (strcmp(strDeviceName, gCameraDeviceList[i].strDeviceName) == 0)
        {
            return i;
        }
    }
    gCamCount++;

    gCameraDeviceList[i].cameraNum = i;
    gCameraDeviceList[i].nVideoWidth = 640;
    gCameraDeviceList[i].nVideoHeight = 480;
    gCameraDeviceList[i].nBitrate = DEFAULT_BITRATE;
    gCameraDeviceList[i].nFramerate = DEFAULT_FRAMERATE;
    gCameraDeviceList[i].nVideoMode = CAMERA_FORMAT_YUV;
    gCameraDeviceList[i].nGOPLength = DEFAULT_GOPLENGTH;
    strcpy(gCameraDeviceList[i].strDeviceName, strDeviceName);
    //sprintf (gCameraDeviceList[camCnt].strDevice, "/dev/video%d",cameraNum);
    _lockers_init(&gCameraDeviceList[i]);
    PMLOG_INFO(CONST_MODULE_HAL, "\nExisting inititate gracefully\n\n");

    return i;
}
DEVICE_RETURN_CODE_T v4l2_cam_open(char *strDeviceName)
{
    PMLOG_INFO(CONST_MODULE_HAL, "%d: Start.\n", __LINE__);

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int camCnt = 0;
    int nThreadErr;

    camCnt = _camera_init(strDeviceName);
    PMLOG_INFO(CONST_MODULE_HAL, "\n@@@@@@@@@@@@@@camCnt:%d\n", camCnt);

    ret = _v4l2_connect(&gCameraDeviceList[camCnt]);
    if ((ret != DEVICE_OK))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Camera connect FAILED !!\n", __LINE__);
        return ret;
    }
    PMLOG_INFO(CONST_MODULE_HAL, "%d: CreateTask : CaptureThread\n", __LINE__);
    if ((nThreadErr = pthread_create(&gCameraDeviceList[camCnt].loop_thread, NULL, CaptureThread,
            &gCameraDeviceList[camCnt])) != 0)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d create thread failed\n", __LINE__);
        ret = DEVICE_ERROR_UNKNOWN;
    }
    gCameraDeviceList[camCnt].isDeviceOpen = true;
    return ret;
}

DEVICE_RETURN_CODE_T _lockers_init(CAM_DEVICE_T* pDevice)
{
    pthread_mutex_init(&pDevice->hCaptureThread, NULL);
    pthread_cond_init(&pDevice->hCaptureThreadCond, NULL);

    return DEVICE_OK;
}

void v4l2_cam_registerCallback(char *strDeviceName, pfpDataCB func)
{

    int camCnt = 0;

    camCnt = _camera_init(strDeviceName);

    gCameraDeviceList[camCnt].pDataCB = func;
}

/******************************************************************************
 Local Function Definitions
 ******************************************************************************/
DEVICE_RETURN_CODE_T _v4l2_disconnect(CAM_DEVICE_T *pDevice)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    int retVal;
    if (pDevice->hCamfd != 0 && pDevice->hCamfd != -1)
    {
        retVal = close(pDevice->hCamfd);
        pDevice->hCamfd = -1;
        if (-1 == retVal)
            PMLOG_INFO(CONST_MODULE_HAL, "Close device failed\n");
        else
            PMLOG_INFO(CONST_MODULE_HAL, "%d: close device success\n");

    }
    //ret = _v4l2_memoryunmapping(pDevice);
    return ret;
}

DEVICE_RETURN_CODE_T _v4l2_memoryunmapping(CAM_DEVICE_T *pDevice)
{
    PMLOG_INFO(CONST_MODULE_HAL, "%d: Memory UnMapping started\n", __LINE__);
    unsigned int i;

    for (i = 0; i < pDevice->nMMbuffers; i++)
    {
        if (pDevice->hMMbuffers[i].start != NULL)
        {
            if (-1 == munmap(pDevice->hMMbuffers[i].start, pDevice->hMMbuffers[i].length))
            {
                PMLOG_INFO(CONST_MODULE_HAL, "%d: munmap error (%d) %s !!", __LINE__, errno,
                        strerror(errno));
                return DEVICE_ERROR_UNKNOWN;
            }
            pDevice->hMMbuffers[i].start = NULL;
        }
        PMLOG_INFO(CONST_MODULE_HAL, "%d:Munmap buffers[%d].\n", __LINE__, i);
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T _v4l2_connect(CAM_DEVICE_T *pDevice)
{
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    pDevice->hCamfd = open(pDevice->strDeviceName, O_RDWR, 0);
    if (-1 == pDevice->hCamfd)
    {
        PMLOG_INFO(CONST_MODULE_HAL, ":%d: v4l2_connect failed\n\n");
        ret = DEVICE_ERROR_NODEVICE;
    }
    else
        PMLOG_INFO(CONST_MODULE_HAL, "%d: device open success\n");
    return ret;
}

static int xioctl(int fd, int request, void *arg)
{
    int r;
    do
    {
        r = ioctl(fd, request, arg);
    } while (-1 == r && EINTR == errno);
    return r;
}

DEVICE_RETURN_CODE_T _v4l2_ConnectCapCheck(CAM_DEVICE_T *pDevice)
{
    struct v4l2_capability cap;
    struct v4l2_format fmt;

    if (-1 == xioctl(pDevice->hCamfd, VIDIOC_QUERYCAP, &cap))
    {
        if (EINVAL == errno)
        {
            PMLOG_INFO(CONST_MODULE_HAL, "%d: %s is no V4L2 device", __LINE__,
                    pDevice->strDeviceName);
            return DEVICE_ERROR_UNSUPPORTED_DEVICE;
        }
        else
            PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_QUERYCAP error (%d) %s !!", __LINE__, errno,
                    strerror(errno));
    }
    else
        PMLOG_INFO(CONST_MODULE_HAL, "VIDIOC_QUERYCAP OK.");

    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%s is no video capture device !!", pDevice->strDeviceName);
        return DEVICE_ERROR_UNSUPPORTED_DEVICE;
    }

    if (!(cap.capabilities & V4L2_CAP_STREAMING))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%s does not support streaming i/o !!", pDevice->strDeviceName);
        return DEVICE_ERROR_UNSUPPORTED_DEVICE;
    }

    PMLOG_INFO(CONST_MODULE_HAL, "%s's capabilities are OK.", pDevice->strDeviceName);

    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = pDevice->nVideoWidth;
    fmt.fmt.pix.height = pDevice->nVideoHeight;

    if (pDevice->nVideoMode == CAMERA_FORMAT_H264ES)
    {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_H264; //0; //v4l2_fourcc('H','2','6','4'); //V4L2_PIX_FMT_H264;

    }
    else if (pDevice->nVideoMode == CAMERA_FORMAT_YUV)
    {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
    }
    else if (pDevice->nVideoMode == CAMERA_FORMAT_JPEG)
    {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
    }

    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

    if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_FMT, &fmt))
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_S_FMT error (%d) %s !!", __LINE__, errno,
                strerror(errno));
        if ((fmt.fmt.pix.pixelformat == V4L2_PIX_FMT_H264))
        {
            fmt.fmt.pix.pixelformat = 0;
            if (-1 == xioctl(pDevice->hCamfd, VIDIOC_S_FMT, &fmt))
            {
                PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_S_FMT error again.(%d) %s !!", __LINE__,
                        errno, strerror(errno));
                return DEVICE_ERROR_WRONG_PARAM;
            }
        }
        else
            return DEVICE_ERROR_WRONG_PARAM;
    }
    PMLOG_INFO(CONST_MODULE_HAL, "VIDIOC_S_FMT OK.(format = %d)", pDevice->nVideoMode);

    if (fmt.fmt.pix.width != (unsigned int) pDevice->nVideoWidth
            || fmt.fmt.pix.height != (unsigned int) pDevice->nVideoHeight)
    {
        PMLOG_INFO(CONST_MODULE_HAL,
                "%d: VIDIOC_S_FMT changed width or height: wanted(%d %d), got(%d %d) !!", __LINE__,
                pDevice->nVideoWidth, pDevice->nVideoHeight, fmt.fmt.pix.width, fmt.fmt.pix.height);
        return DEVICE_ERROR_WRONG_PARAM;
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T _v4l2_ConnectRequestBuffers(CAM_DEVICE_T *pDevice)
{
    struct v4l2_requestbuffers req;
    req.count = MAX_MEMORY_MAP;     // fixed the number of mmap 2012.01.17
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (-1 == xioctl(pDevice->hCamfd, VIDIOC_REQBUFS, &req))
    {
        //if(EINVAL == errno)
        //  AIT_PRINT("%s does not support memory mapping\r\n", pDevice->strDevice);
        //else

        PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_REQBUFS error (%d) %s !!", __LINE__, errno,
                strerror(errno));
        return DEVICE_ERROR_UNKNOWN;
    }
    else
        PMLOG_INFO(CONST_MODULE_HAL, "VIDIOC_REQBUFS OK.");

    if (req.count < 2)
    {
        PMLOG_INFO(CONST_MODULE_HAL, "%d: Insufficient buffer memory on %s.", __LINE__,
                pDevice->strDeviceName);
        return DEVICE_ERROR_OUT_OF_MEMORY;
    }

    pDevice->nMMbuffers = req.count;

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T _v4l2_ConnectMemoryMapping(CAM_DEVICE_T *pDevice)
{
    unsigned int i;

    struct v4l2_buffer buf;

    PMLOG_INFO(CONST_MODULE_HAL, "%d: number of buffers requested:%d\n\n", __LINE__,
            pDevice->nMMbuffers);
    for (i = 0; i < pDevice->nMMbuffers; i++)
    {
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (-1 == xioctl(pDevice->hCamfd, VIDIOC_QUERYBUF, &buf))
        {
            PMLOG_INFO(CONST_MODULE_HAL, "%d: VIDIOC_QUERYBUF error (%d) %s !!", __LINE__, errno,
                    strerror(errno));
            return DEVICE_ERROR_UNKNOWN;
        }
        pDevice->hMMbuffers[i].length = buf.length;
        pDevice->hMMbuffers[i].start = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                pDevice->hCamfd, buf.m.offset);
        if (MAP_FAILED == pDevice->hMMbuffers[i].start)
        {
            PMLOG_INFO(CONST_MODULE_HAL, "%d: mmap failed (%d) %s !!", __LINE__, errno,
                    strerror(errno));
            return DEVICE_ERROR_OUT_OF_MEMORY;
        }
        PMLOG_INFO(CONST_MODULE_HAL, "Mmap buffers[%d] : %d.", i, buf.length);
    }

    return DEVICE_OK;
}

#ifdef __cplusplus
}
#endif
