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

/*-----------------------------------------------------------------------------
 #include
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "device_controller.h"
#include "camera_hal_if.h"
#include "command_manager.h"

#include <poll.h>
#include <string.h>

DeviceControl::DeviceControl()
    : b_iscontinuous_capture_(false), b_isstreamon_(false), b_isshmwritedone_(true),
      cam_handle_(NULL), informat_(), mutex_(PTHREAD_MUTEX_INITIALIZER),
      cond_(PTHREAD_COND_INITIALIZER), h_shm_(NULL)
{
}

void DeviceControl::writeImageToFile(const void *p, int size) const
{
  FILE *fp;
  char image_name[100] = {};

  // create file to save data based on format
  if (CAMERA_PIXEL_FORMAT_YUYV == epixelformat_)
    snprintf(image_name, 100, "/tmp/Picture%d.yuv", rand());
  else if (CAMERA_PIXEL_FORMAT_JPEG == epixelformat_)
    snprintf(image_name, 100, "/tmp/Picture%d.jpeg", rand());
  else if (CAMERA_PIXEL_FORMAT_H264 == epixelformat_)
    snprintf(image_name, 100, "/tmp/Picture%d.h264", rand());

  if (NULL == (fp = fopen(image_name, "a")))
  {
    return;
  }
  fwrite((unsigned char *)p, 1, size, fp);
  fclose(fp);
}

DEVICE_RETURN_CODE_T DeviceControl::checkFormat(void *handle, CAMERA_FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_DC, "checkFormat for device\n");

  // get current saved format for device
  stream_format_t streamformat;
  int retval = camera_hal_if_get_format(handle, &streamformat);
  if (CAMERA_ERROR_NONE != retval)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "checkFormat : camera_hal_if_get_format failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  PMLOG_INFO(CONST_MODULE_DC, "checkFormat stream_format_t pixel_format : %d \n",
             streamformat.pixel_format);

  // save pixel format for saving captured image
  epixelformat_ = streamformat.pixel_format;

  DEVICE_RETURN_CODE_T ret = DEVICE_OK;
  camera_pixel_format_t enewformat = getPixelFormat(sformat.eFormat);

  // check if saved format and format for capture is same or not
  // if not then stop v4l2 device, set format again and start capture
  if ((streamformat.stream_height != sformat.nHeight) ||
      (streamformat.stream_width != sformat.nWidth) || (streamformat.pixel_format != enewformat))
  {
    PMLOG_INFO(CONST_MODULE_DC, "checkFormat : Stored format and new format are different\n");
    // stream off, unmap and destroy previous allocated buffers
    // close and again open device to set format again
    stopPreview(handle);
    close(handle);
    open(handle, strdevicenode_);

    // set format again
    stream_format_t newstreamformat;
    newstreamformat.stream_height = sformat.nHeight;
    newstreamformat.stream_width = sformat.nWidth;
    newstreamformat.pixel_format = getPixelFormat(sformat.eFormat);
    retval = camera_hal_if_set_format(handle, newstreamformat);
    if (CAMERA_ERROR_NONE != retval)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "checkFormat : camera_hal_if_set_format failed \n");
      // if set format fails then reset format to preview format
      camera_hal_if_set_format(handle, streamformat);
      // save pixel format for saving captured image
      epixelformat_ = streamformat.pixel_format;
    }
    else
      // save pixel format for saving captured image
      epixelformat_ = newstreamformat.pixel_format;

    // allocate buffers and stream on again
    int key = 0;
    ret = startPreview(handle, &key);
  }

  return ret;
}

int DeviceControl::pollForCapturedImage(void *handle, int ncount) const
{
  int fd = -1;
  int retval = camera_hal_if_get_fd(handle, &fd);
  PMLOG_INFO(CONST_MODULE_DC, "pollForCapturedImage camera_hal_if_get_fd fd : %d \n", fd);
  struct pollfd poll_set[]{
      {.fd = fd, .events = POLLIN},
  };

  // get current saved format for device
  stream_format_t streamformat;
  camera_hal_if_get_format(handle, &streamformat);
  PMLOG_INFO(CONST_MODULE_DC, "Driver set width : %d height : %d", streamformat.stream_width,
             streamformat.stream_height);
  int framesize =
      streamformat.stream_width * streamformat.stream_height * buffer_count + extra_buffer;

  int timeout = 2000;
  buffer_t frame_buffer;
  for (int i = 1; i <= ncount; i++)
  {
    if ((retval = poll(poll_set, 2, timeout)) > 0)
    {
      frame_buffer.start = malloc(framesize);
      retval = camera_hal_if_get_buffer(handle, &frame_buffer);
      if (CAMERA_ERROR_NONE != retval)
      {
        PMLOG_ERROR(CONST_MODULE_DC, "pollForCapturedImage : camera_hal_if_get_buffer failed \n");
        return DEVICE_ERROR_UNKNOWN;
      }
      PMLOG_INFO(CONST_MODULE_DC, "pollForCapturedImage buffer start : %p \n", frame_buffer.start);
      PMLOG_INFO(CONST_MODULE_DC, "pollForCapturedImage buffer length : %d \n",
                 frame_buffer.length);

      // write captured image to /tmp only if startCapture request is made
      writeImageToFile(frame_buffer.start, frame_buffer.length);

      free(frame_buffer.start);
      frame_buffer.start = nullptr;

      retval = camera_hal_if_release_buffer(handle, frame_buffer);
      if (CAMERA_ERROR_NONE != retval)
      {
        PMLOG_ERROR(CONST_MODULE_DC,
                    "pollForCapturedImage : camera_hal_if_release_buffer failed \n");
        return DEVICE_ERROR_UNKNOWN;
      }
    }
  }
  return CAMERA_ERROR_NONE;
}

camera_pixel_format_t DeviceControl::getPixelFormat(camera_format_t eformat)
{
  // convert CAMERA_FORMAT_T to camera_pixel_format_t
  if (CAMERA_FORMAT_H264ES == eformat)
  {
    return CAMERA_PIXEL_FORMAT_H264;
  }
  else if (CAMERA_FORMAT_YUV == eformat)
  {
    return CAMERA_PIXEL_FORMAT_YUYV;
  }
  else if (CAMERA_FORMAT_JPEG == eformat)
  {
    return CAMERA_PIXEL_FORMAT_JPEG;
  }
}

void DeviceControl::captureThread()
{
  PMLOG_INFO(CONST_MODULE_DC, "captureThread started\n");

  // run capture thread until stopCapture received
  while (b_iscontinuous_capture_)
  {
    DEVICE_RETURN_CODE_T ret = captureImage(cam_handle_, 1, informat_);
    if (DEVICE_OK != ret)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "captureThread : captureImage failed \n");
      break;
    }
  }
  // set continuous capture to false
  b_iscontinuous_capture_ = false;
  pthread_detach(tid_capture_);
  return;
}

void DeviceControl::previewThread()
{
  pthread_cond_init(&cond_, NULL);
  pthread_mutex_init(&mutex_, NULL);

  // poll for data on buffers and save captured image
  // lock so that if stop preview is called, first this cycle should complete
  pthread_mutex_lock(&mutex_);
  b_isshmwritedone_ = false;

  // get current saved format for device
  stream_format_t streamformat;
  camera_hal_if_get_format(cam_handle_, &streamformat);
  PMLOG_INFO(CONST_MODULE_DC, "Driver set width : %d height : %d", streamformat.stream_width,
             streamformat.stream_height);
  int framesize =
      streamformat.stream_width * streamformat.stream_height * buffer_count + extra_buffer;

  while (b_isstreamon_)
  {
    buffer_t frame_buffer;
    frame_buffer.start = malloc(framesize);
    int retval = camera_hal_if_get_buffer(cam_handle_, &frame_buffer);
    if (CAMERA_ERROR_NONE != retval)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "previewThread : camera_hal_if_get_buffer failed \n");
      free(frame_buffer.start);
      frame_buffer.start = nullptr;
      break;
    }

    // keep writing data to shared memory
    unsigned int timestamp = 0;
    WriteShmemEx(h_shm_, (unsigned char *)frame_buffer.start, frame_buffer.length,
                 (unsigned char *)&timestamp, sizeof(timestamp));

    free(frame_buffer.start);
    frame_buffer.start = nullptr;

    retval = camera_hal_if_release_buffer(cam_handle_, frame_buffer);
    if (CAMERA_ERROR_NONE != retval)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "previewThread : camera_hal_if_release_buffer failed \n");
      break;
    }
  }

  b_isshmwritedone_ = true;
  pthread_cond_signal(&cond_);
  pthread_mutex_unlock(&mutex_);
  pthread_detach(tid_preview_);
  return;
}

void *DeviceControl::runCaptureImageThread(void *arg)
{
  DeviceControl *ptr = reinterpret_cast<DeviceControl *>(arg);
  ptr->captureThread();
  return 0;
}

void *DeviceControl::runPreviewThread(void *arg)
{
  DeviceControl *ptr = reinterpret_cast<DeviceControl *>(arg);
  ptr->previewThread();
  return 0;
}

DEVICE_RETURN_CODE_T DeviceControl::open(void *handle, std::string devicenode)
{
  PMLOG_INFO(CONST_MODULE_DC, "open started\n");

  strdevicenode_ = devicenode;

  // open camera device
  int ret = camera_hal_if_open_device(handle, devicenode.c_str());

  if (CAMERA_ERROR_NONE != ret)
    return DEVICE_ERROR_CAN_NOT_OPEN;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::close(void *handle)
{
  PMLOG_INFO(CONST_MODULE_DC, "close started \n");

  int retshmem = CloseShmem(&h_shm_);
  if (SHMEM_COMM_OK != retshmem)
    PMLOG_ERROR(CONST_MODULE_DC, "CloseShmem error %d \n", retshmem);

  h_shm_ = NULL;

  // close device
  int ret = camera_hal_if_close_device(handle);
  if (CAMERA_ERROR_NONE != ret)
    return DEVICE_ERROR_CAN_NOT_CLOSE;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startPreview(void *handle, int *pkey)
{
  PMLOG_INFO(CONST_MODULE_DC, "startPreview started !\n");

  cam_handle_ = handle;

  // get current saved format for device
  stream_format_t streamformat;
  camera_hal_if_get_format(handle, &streamformat);
  PMLOG_INFO(CONST_MODULE_DC, "Driver set width : %d height : %d", streamformat.stream_width,
             streamformat.stream_height);

  int size = streamformat.stream_width * streamformat.stream_height * buffer_count + extra_buffer;

  CreateShmemEx(&h_shm_, pkey, size, frame_count, sizeof(unsigned int));

  int retval = camera_hal_if_set_buffer(handle, 4, IOMODE_MMAP);
  if (CAMERA_ERROR_NONE != retval)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "startPreview : camera_hal_if_set_buffer failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  retval = camera_hal_if_start_capture(handle);
  if (CAMERA_ERROR_NONE != retval)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "startPreview : camera_hal_if_start_capture failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  b_isstreamon_ = true;

  // create thread that will continuously capture images until stopcapture received
  if (0 != (pthread_create(&tid_preview_, NULL, runPreviewThread, this)))
  {
    PMLOG_ERROR(CONST_MODULE_DC, "runPreviewThread failed\n");
    return DEVICE_ERROR_UNKNOWN;
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::stopPreview(void *handle)
{
  PMLOG_INFO(CONST_MODULE_DC, "stopPreview started !\n");

  b_isstreamon_ = false;

  // wait for preview thread to close
  pthread_mutex_lock(&mutex_);
  while (!b_isshmwritedone_)
    pthread_cond_wait(&cond_, &mutex_);
  pthread_mutex_unlock(&mutex_);

  pthread_cond_destroy(&cond_);
  pthread_mutex_destroy(&mutex_);

  int retval = camera_hal_if_stop_capture(handle);
  if (CAMERA_ERROR_NONE != retval)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "stopPreview : camera_hal_if_stop_capture failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  retval = camera_hal_if_destroy_buffer(handle);
  if (CAMERA_ERROR_NONE != retval)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "stopPreview : camera_hal_if_destroy_buffer failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startCapture(void *handle, CAMERA_FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_DC, "startCapture started !\n");

  cam_handle_ = handle;
  informat_.nHeight = sformat.nHeight;
  informat_.nWidth = sformat.nWidth;
  informat_.eFormat = sformat.eFormat;
  b_iscontinuous_capture_ = true;

  // create thread that will continuously capture images until stopcapture received
  if (0 != (pthread_create(&tid_capture_, NULL, runCaptureImageThread, this)))
  {
    PMLOG_ERROR(CONST_MODULE_DC, "runCaptureImageThread failed\n");
    return DEVICE_ERROR_UNKNOWN;
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::stopCapture(void *handle)
{
  PMLOG_INFO(CONST_MODULE_DC, "stopCapture started !\n");

  // if capture thread is running, stop capture
  if (b_iscontinuous_capture_)
    b_iscontinuous_capture_ = false;
  else
    return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::captureImage(void *handle, int ncount, CAMERA_FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_DC, "captureImage started ncount : %d \n", ncount);

  // validate if saved format and capture image format are same or not
  if (DEVICE_OK != checkFormat(handle, sformat))
  {
    PMLOG_ERROR(CONST_MODULE_DC, "captureImage : checkFormat failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  // poll for data on buffers and save captured image
  int retval = pollForCapturedImage(handle, ncount);
  if (CAMERA_ERROR_NONE != retval)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "captureImage : pollForCapturedImage failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::createHandle(void **handle, std::string subsystem)
{
  PMLOG_INFO(CONST_MODULE_DC, "createHandle started \n");

  void *p_cam_handle;
  int ret = camera_hal_if_init(&p_cam_handle, subsystem.c_str());
  if (CAMERA_ERROR_NONE != ret)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "Failed to create handle\n!!");
    *handle = NULL;
    return DEVICE_ERROR_UNKNOWN;
  }
  PMLOG_INFO(CONST_MODULE_DC, "createHandle : p_cam_handle : %p \n", p_cam_handle);
  *handle = p_cam_handle;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::destroyHandle(void *handle)
{
  PMLOG_INFO(CONST_MODULE_DC, "destroyHandle started \n");

  int ret = camera_hal_if_deinit(handle);
  if (CAMERA_ERROR_NONE != ret)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "Failed to destroy handle\n!!");
    return DEVICE_ERROR_UNKNOWN;
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getDeviceInfo(std::string strdevicenode,
                                                  camera_device_info_t *pinfo)
{
  PMLOG_INFO(CONST_MODULE_DC, "getDeviceInfo started \n");

  int ret = camera_hal_if_get_info(strdevicenode.c_str(), pinfo);
  if (CAMERA_ERROR_NONE != ret)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "Failed to get the info\n!!");
    return DEVICE_ERROR_UNKNOWN;
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getDeviceList(DEVICE_LIST_T *plist, int *pcamdev, int *pmicdev,
                                                  int *pcamsupport, int *pmicsupport, int ncount)
{
  PMLOG_INFO(CONST_MODULE_DC, "getDeviceList started count : %d \n", ncount);

  *pcamdev = 0;
  *pmicdev = 0;
  *pcamsupport = 0;
  *pmicsupport = 0;
  for (int i = 0; i < ncount; i++)
  {
    if (strncmp(plist[i].strDeviceType, "CAM", 3) == 0)
    {
      pcamdev[i] = i + 1;
      pcamsupport[i] = 1;
    }
    else if (strncmp(plist[i].strDeviceType, "MIC", 3) == 0)
    {
      *pmicdev += 1;
      pmicsupport[i] = 1;
    }
    else
      PMLOG_INFO(CONST_MODULE_DC, "Unknown device\n");
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getDeviceProperty(void *handle, CAMERA_PROPERTIES_T *oparams)
{
  PMLOG_INFO(CONST_MODULE_DC, "getDeviceProperty started !\n");

  camera_properties_t out_params;
  out_params.nPan = CONST_VARIABLE_INITIALIZE;
  out_params.nTilt = CONST_VARIABLE_INITIALIZE;
  out_params.nContrast = CONST_VARIABLE_INITIALIZE;
  out_params.nBrightness = CONST_VARIABLE_INITIALIZE;
  out_params.nSaturation = CONST_VARIABLE_INITIALIZE;
  out_params.nSharpness = CONST_VARIABLE_INITIALIZE;
  out_params.nHue = CONST_VARIABLE_INITIALIZE;
  out_params.nGain = CONST_VARIABLE_INITIALIZE;
  out_params.nGamma = CONST_VARIABLE_INITIALIZE;
  out_params.nFrequency = CONST_VARIABLE_INITIALIZE;
  out_params.nAutoWhiteBalance = CONST_VARIABLE_INITIALIZE;
  out_params.nBacklightCompensation = CONST_VARIABLE_INITIALIZE;
  out_params.nExposure = CONST_VARIABLE_INITIALIZE;
  out_params.nWhiteBalanceTemperature = CONST_VARIABLE_INITIALIZE;
  camera_hal_if_get_properties(handle, &out_params);

  oparams->nPan = out_params.nPan;
  oparams->nTilt = out_params.nTilt;
  oparams->nContrast = out_params.nContrast;
  oparams->nBrightness = out_params.nBrightness;
  oparams->nSaturation = out_params.nSaturation;
  oparams->nSharpness = out_params.nSharpness;
  oparams->nHue = out_params.nHue;
  oparams->nGain = out_params.nGain;
  oparams->nGamma = out_params.nGamma;
  oparams->nFrequency = out_params.nFrequency;
  oparams->bAutoWhiteBalance = out_params.nAutoWhiteBalance;
  oparams->bBacklightCompensation = out_params.nBacklightCompensation;
  if (oparams->bAutoExposure == 0)
    oparams->nExposure = out_params.nExposure;
  if (oparams->bAutoWhiteBalance == 0)
    oparams->nWhiteBalanceTemperature = out_params.nWhiteBalanceTemperature;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setDeviceProperty(void *handle, CAMERA_PROPERTIES_T *inparams)
{
  PMLOG_INFO(CONST_MODULE_DC, "setDeviceProperty started!\n");

  camera_properties_t in_params;

  if (inparams->nZoom != CONST_VARIABLE_INITIALIZE)
    in_params.nZoomAbsolute = inparams->nZoom;

  if (inparams->nPan != CONST_VARIABLE_INITIALIZE)
    in_params.nPan = inparams->nPan;

  if (inparams->nTilt != CONST_VARIABLE_INITIALIZE)
    in_params.nTilt = inparams->nTilt;

  if (inparams->nContrast != CONST_VARIABLE_INITIALIZE)
    in_params.nContrast = inparams->nContrast;

  if (inparams->nBrightness != CONST_VARIABLE_INITIALIZE)
    in_params.nBrightness = inparams->nBrightness;

  if (inparams->nSaturation != CONST_VARIABLE_INITIALIZE)
    in_params.nSaturation = inparams->nSaturation;

  if (inparams->nSharpness != CONST_VARIABLE_INITIALIZE)
    in_params.nSharpness = inparams->nSharpness;

  if (inparams->nHue != CONST_VARIABLE_INITIALIZE)
    in_params.nHue = inparams->nHue;

  if (inparams->bAutoExposure != CONST_VARIABLE_INITIALIZE)
    in_params.nAutoExposure = inparams->bAutoExposure;

  if (inparams->bAutoWhiteBalance != CONST_VARIABLE_INITIALIZE)
    in_params.nAutoWhiteBalance = inparams->bAutoWhiteBalance;

  if (inparams->nExposure != CONST_VARIABLE_INITIALIZE)
    in_params.nExposure = inparams->nExposure;

  if (inparams->nWhiteBalanceTemperature != CONST_VARIABLE_INITIALIZE)
    in_params.nWhiteBalanceTemperature = inparams->nWhiteBalanceTemperature;

  if (inparams->nGain != CONST_VARIABLE_INITIALIZE)
    in_params.nGain = inparams->nGain;

  if (inparams->nGamma != CONST_VARIABLE_INITIALIZE)
    in_params.nGamma = inparams->nGamma;

  if (inparams->nFrequency != CONST_VARIABLE_INITIALIZE)
    in_params.nFrequency = inparams->nFrequency;

  if (inparams->bBacklightCompensation != CONST_VARIABLE_INITIALIZE)
    in_params.nBacklightCompensation = inparams->bBacklightCompensation;

  camera_hal_if_set_properties(handle, &in_params);

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setFormat(void *handle, CAMERA_FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_DC, "sFormat Height %d Width %d Format %d\n", sformat.nHeight,
             sformat.nWidth, sformat.eFormat);

  stream_format_t in_format;
  in_format.stream_height = sformat.nHeight;
  in_format.stream_width = sformat.nWidth;
  in_format.pixel_format = getPixelFormat(sformat.eFormat);

  int ret = camera_hal_if_set_format(handle, in_format);
  if (CAMERA_ERROR_NONE != ret)
    return DEVICE_ERROR_UNSUPPORTED_FORMAT;

  return DEVICE_OK;
}
