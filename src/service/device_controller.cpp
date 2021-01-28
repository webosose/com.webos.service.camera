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

/*-----------------------------------------------------------------------------
 #include
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include "device_controller.h"
#include "camera_hal_if.h"
#include "command_manager.h"

#include <ctime>
#include <poll.h>
#include <sys/time.h>

int DeviceControl::n_imagecount_ = 0;

DeviceControl::DeviceControl()
    : b_iscontinuous_capture_(false), b_isstreamon_(false), b_isposixruning(false),
      b_issystemvruning(false), b_isshmwritedone_(true), b_issyshmwritedone_(true),
      b_isposhmwritedone_(true), cam_handle_(NULL), shmemfd_(-1), informat_(),
      tMutex(), tCondVar(), h_shmsystem_(NULL), h_shmposix_(NULL),
      str_imagepath_(cstr_empty), str_capturemode_(cstr_oneshot), str_memtype_(""),
      str_shmemname_(""), epixelformat_(CAMERA_PIXEL_FORMAT_JPEG)
{
}

DEVICE_RETURN_CODE_T DeviceControl::writeImageToFile(const void *p, int size) const
{
  FILE *fp;
  char image_name[100] = {};

  auto path = str_imagepath_;
  if (path.empty())
    path = "/tmp/";

  // find the file extension to check if file name is provided or path is provided
  std::size_t position = path.find_last_of(".");
  std::string extension = path.substr(position + 1);

  if ((extension == "yuv") || (extension == "jpeg") || (extension == "h264"))
  {
    if (cstr_burst == str_capturemode_ || cstr_continuous == str_capturemode_)
    {
      path.insert(position, std::to_string(n_imagecount_));
      n_imagecount_++;
    }
  }
  else
  {
    // check if specified location ends with '/' else add
    char ch = path.back();
    if (ch != '/')
      path += "/";

    time_t t = time(NULL);
    tm *timePtr = localtime(&t);
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    // create file to save data based on format
    if (epixelformat_ == CAMERA_PIXEL_FORMAT_YUYV)
      snprintf(image_name, 100, "Picture%02d%02d%02d-%02d%02d%02d%02d.yuv", timePtr->tm_mday,
               (timePtr->tm_mon) + 1, (timePtr->tm_year) + 1900, (timePtr->tm_hour),
               (timePtr->tm_min), (timePtr->tm_sec), ((int)tmnow.tv_usec) / 10000);
    else if (epixelformat_ == CAMERA_PIXEL_FORMAT_JPEG)
      snprintf(image_name, 100, "Picture%02d%02d%02d-%02d%02d%02d%02d.jpeg", timePtr->tm_mday,
               (timePtr->tm_mon) + 1, (timePtr->tm_year) + 1900, (timePtr->tm_hour),
               (timePtr->tm_min), (timePtr->tm_sec), ((int)tmnow.tv_usec) / 10000);
    else if (epixelformat_ == CAMERA_PIXEL_FORMAT_H264)
      snprintf(image_name, 100, "Picture%02d%02d%02d-%02d%02d%02d%02d.h264", timePtr->tm_mday,
               (timePtr->tm_mon) + 1, (timePtr->tm_year) + 1900, (timePtr->tm_hour),
               (timePtr->tm_min), (timePtr->tm_sec), ((int)tmnow.tv_usec) / 10000);
    path = path + image_name;
  }

  PMLOG_INFO(CONST_MODULE_DC, "writeImageToFile path : %s\n", path.c_str());

  if (NULL == (fp = fopen(path.c_str(), "w")))
  {
    PMLOG_INFO(CONST_MODULE_DC, "writeImageToFile path : fopen failed\n");
    return DEVICE_ERROR_CANNOT_WRITE;
  }
  fwrite((unsigned char *)p, 1, size, fp);
  fclose(fp);
  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::checkFormat(void *handle, CAMERA_FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_DC, "checkFormat for device\n");

  // get current saved format for device
  stream_format_t streamformat;
  auto retval = camera_hal_if_get_format(handle, &streamformat);
  if (retval != CAMERA_ERROR_NONE)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "checkFormat : camera_hal_if_get_format failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  PMLOG_INFO(CONST_MODULE_DC, "checkFormat stream_format_t pixel_format : %d \n",
             streamformat.pixel_format);

  // save pixel format for saving captured image
  epixelformat_ = streamformat.pixel_format;

  DEVICE_RETURN_CODE_T ret = DEVICE_OK;
  auto enewformat = getPixelFormat(sformat.eFormat);
  //error handling
  if(enewformat == CAMERA_PIXEL_FORMAT_MAX)
    return DEVICE_ERROR_UNSUPPORTED_FORMAT;

  // check if saved format and format for capture is same or not
  // if not then stop v4l2 device, set format again and start capture
  if ((streamformat.stream_height != sformat.nHeight) ||
      (streamformat.stream_width != sformat.nWidth) || (streamformat.pixel_format != enewformat))
  {
    PMLOG_INFO(CONST_MODULE_DC, "checkFormat : Stored format and new format are different\n");
    // stream off, unmap and destroy previous allocated buffers
    // close and again open device to set format again
    int memtype = -1;
    if(str_memtype_ == kMemtypeShmem)
       memtype = SHMEM_SYSTEMV;
    else
       memtype = SHMEM_POSIX;
    stopPreview(handle, memtype);
    close(handle);
    open(handle, strdevicenode_);

    // set format again
    stream_format_t newstreamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
    newstreamformat.stream_height = sformat.nHeight;
    newstreamformat.stream_width = sformat.nWidth;
    newstreamformat.pixel_format = getPixelFormat(sformat.eFormat);
    //error handling
    if(newstreamformat.pixel_format == CAMERA_PIXEL_FORMAT_MAX)
      return DEVICE_ERROR_UNSUPPORTED_FORMAT;
    retval = camera_hal_if_set_format(handle, newstreamformat);
    if (retval != CAMERA_ERROR_NONE)
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
    ret = startPreview(handle, str_memtype_, &key);
  }

  return ret;
}

DEVICE_RETURN_CODE_T DeviceControl::pollForCapturedImage(void *handle, int ncount) const
{
  int fd = -1;
  auto retval = camera_hal_if_get_fd(handle, &fd);
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
    if ((retval = poll(poll_set, 1, timeout)) > 0)
    {
      frame_buffer.start = malloc(framesize);
      retval = camera_hal_if_get_buffer(handle, &frame_buffer);
      if (CAMERA_ERROR_NONE != retval)
      {
        PMLOG_ERROR(CONST_MODULE_DC, "pollForCapturedImage : camera_hal_if_get_buffer failed \n");
        return DEVICE_ERROR_UNKNOWN;
      }
      PMLOG_INFO(CONST_MODULE_DC, "pollForCapturedImage buffer start : %p \n", frame_buffer.start);
      PMLOG_INFO(CONST_MODULE_DC, "pollForCapturedImage buffer length : %lu \n",
                 frame_buffer.length);

      // write captured image to /tmp only if startCapture request is made
      if (DEVICE_ERROR_CANNOT_WRITE == writeImageToFile(frame_buffer.start, frame_buffer.length))
        return DEVICE_ERROR_CANNOT_WRITE;

      free(frame_buffer.start);
      frame_buffer.start = nullptr;

      retval = camera_hal_if_release_buffer(handle, frame_buffer);
      if (retval != CAMERA_ERROR_NONE)
      {
        PMLOG_ERROR(CONST_MODULE_DC,
                    "pollForCapturedImage : camera_hal_if_release_buffer failed \n");
        return DEVICE_ERROR_UNKNOWN;
      }
    }
  }

  if (cstr_burst == str_capturemode_)
    n_imagecount_ = 0;

  return DEVICE_OK;
}

camera_pixel_format_t DeviceControl::getPixelFormat(camera_format_t eformat)
{
  // convert CAMERA_FORMAT_T to camera_pixel_format_t
  if (eformat == CAMERA_FORMAT_H264ES)
  {
    return CAMERA_PIXEL_FORMAT_H264;
  }
  else if (eformat == CAMERA_FORMAT_YUV)
  {
    return CAMERA_PIXEL_FORMAT_YUYV;
  }
  else if (eformat == CAMERA_FORMAT_JPEG)
  {
    return CAMERA_PIXEL_FORMAT_JPEG;
  }
  //error case
  return CAMERA_PIXEL_FORMAT_MAX;
}

void DeviceControl::captureThread()
{
  PMLOG_INFO(CONST_MODULE_DC, "captureThread started\n");

  // run capture thread until stopCapture received
  while (b_iscontinuous_capture_)
  {
    auto ret =
        captureImage(cam_handle_, 1, informat_, str_imagepath_, cstr_continuous);
    if (ret != DEVICE_OK)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "captureThread : captureImage failed \n");
      break;
    }
  }
  // set continuous capture to false
  b_iscontinuous_capture_ = false;
  tidCapture.detach();
  n_imagecount_ = 0;
  return;
}

void DeviceControl::previewThread()
{
  // poll for data on buffers and save captured image
  // lock so that if stop preview is called, first this cycle should complete
  std::lock_guard<std::mutex> guard(tMutex);
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
    auto retval = camera_hal_if_get_buffer(cam_handle_, &frame_buffer);
    if (retval != CAMERA_ERROR_NONE)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "previewThread : camera_hal_if_get_buffer failed \n");
      free(frame_buffer.start);
      frame_buffer.start = nullptr;
      break;
    }

    // keep writing data to shared memory
    unsigned int timestamp = 0;
    if(b_issystemvruning)
    {
       b_issyshmwritedone_ = false;
       auto retshmem = IPCSharedMemory::getInstance().WriteShmemory(h_shmsystem_,
                    (unsigned char *)frame_buffer.start, frame_buffer.length,
                    (unsigned char *)&timestamp, sizeof(timestamp));
       if (retshmem != SHMEM_COMM_OK)
       {
         PMLOG_ERROR(CONST_MODULE_DC, "Write Shared memory error %d \n", retshmem);
       }
       b_issyshmwritedone_ = true;
    }
    if(b_isposixruning)
    {
       b_isposhmwritedone_ = false;
       auto retshmem = IPCPosixSharedMemory::getInstance().WritePosixShmemory(h_shmposix_,
                    (unsigned char *)frame_buffer.start, frame_buffer.length,
                    (unsigned char *)&timestamp, sizeof(timestamp));
       if (retshmem != POSHMEM_COMM_OK)
       {
         PMLOG_ERROR(CONST_MODULE_DC, "Write Posix Shared memory error %d \n", retshmem);
       }
       b_isposhmwritedone_ = true;
    }
    free(frame_buffer.start);
    frame_buffer.start = nullptr;

    retval = camera_hal_if_release_buffer(cam_handle_, frame_buffer);
    if (retval != CAMERA_ERROR_NONE)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "previewThread : camera_hal_if_release_buffer failed \n");
      break;
    }
  }

  b_isshmwritedone_ = true;
  b_issyshmwritedone_ = true;
  b_isposhmwritedone_ = true;
  tCondVar.notify_one();
  tidPreview.detach();
  return;
}

DEVICE_RETURN_CODE_T DeviceControl::open(void *handle, std::string devicenode)
{
  PMLOG_INFO(CONST_MODULE_DC, "open started\n");

  strdevicenode_ = devicenode;

  // open camera device
  auto ret = camera_hal_if_open_device(handle, devicenode.c_str());

  if (ret != CAMERA_ERROR_NONE)
    return DEVICE_ERROR_CAN_NOT_OPEN;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::close(void *handle)
{
  PMLOG_INFO(CONST_MODULE_DC, "close started \n");

  // close device
  auto ret = camera_hal_if_close_device(handle);
  if (ret != CAMERA_ERROR_NONE)
    return DEVICE_ERROR_CAN_NOT_CLOSE;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startPreview(void *handle, std::string memtype,
                                                 int *pkey)
{
  PMLOG_INFO(CONST_MODULE_DC, "startPreview started !\n");

  cam_handle_  = handle;
  str_memtype_ = memtype;

  // get current saved format for device
  stream_format_t streamformat;
  camera_hal_if_get_format(handle, &streamformat);
  PMLOG_INFO(CONST_MODULE_DC, "Driver set width : %d height : %d", streamformat.stream_width,
             streamformat.stream_height);

  int size = streamformat.stream_width * streamformat.stream_height * buffer_count + extra_buffer;

  if(memtype == kMemtypeShmem)
  {
    auto retshmem = IPCSharedMemory::getInstance().CreateShmemory(&h_shmsystem_,
                  pkey, size, frame_count, sizeof(unsigned int));
    if (retshmem != SHMEM_COMM_OK)
       PMLOG_ERROR(CONST_MODULE_DC, "CreateShmemory error %d \n", retshmem);
  }
  else
  {
    std::string shmname = "";
    auto retshmem = IPCPosixSharedMemory::getInstance().CreatePosixShmemory(&h_shmposix_,
                             size, frame_count, sizeof(unsigned int), pkey, &shmname);
    if (retshmem != POSHMEM_COMM_OK)
       PMLOG_ERROR(CONST_MODULE_DC, "CreatePosixShmemory error %d \n", retshmem);

    shmemfd_ = *pkey;
    str_shmemname_ = shmname;
  }

  if(b_isstreamon_ == false)
  {
    auto retval = camera_hal_if_set_buffer(handle, 4, IOMODE_MMAP);
    if (retval != CAMERA_ERROR_NONE)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "startPreview : camera_hal_if_set_buffer failed \n");
      return DEVICE_ERROR_UNKNOWN;
    }

    retval = camera_hal_if_start_capture(handle);
    if (retval != CAMERA_ERROR_NONE)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "startPreview : camera_hal_if_start_capture failed \n");
      return DEVICE_ERROR_UNKNOWN;
    }

    b_isstreamon_ = true;

    // create thread that will continuously capture images until stopcapture received
    tidPreview = std::thread{[this]() { this->previewThread(); }};
  }
  if(memtype == kMemtypePosixshm)
  {
    b_isposixruning = true;
  }
  else
  {
    b_issystemvruning = true;
  }
  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::stopPreview(void *handle, int memtype)
{
  PMLOG_INFO(CONST_MODULE_DC, "stopPreview started !\n");

  // get current saved format for device
  stream_format_t streamformat;
  camera_hal_if_get_format(handle, &streamformat);
  PMLOG_INFO(CONST_MODULE_DC, "Driver set width : %d height : %d", streamformat.stream_width,
           streamformat.stream_height);

  int size = streamformat.stream_width * streamformat.stream_height * buffer_count + extra_buffer;

  if( b_issystemvruning^b_isposixruning )
  {
    b_isstreamon_ = false;

    // wait for preview thread to close
    std::lock_guard<std::mutex> guard(tMutex);

    while (!b_isshmwritedone_)
    {
      std::unique_lock<std::mutex> uniqLock(tMutex);
      tCondVar.wait(uniqLock);
    }
    auto retval = camera_hal_if_stop_capture(handle);
    if (retval != CAMERA_ERROR_NONE)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "stopPreview : camera_hal_if_stop_capture failed \n");
      return DEVICE_ERROR_UNKNOWN;
    }
    retval = camera_hal_if_destroy_buffer(handle);
    if (retval != CAMERA_ERROR_NONE)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "stopPreview : camera_hal_if_destroy_buffer failed \n");
      return DEVICE_ERROR_UNKNOWN;
    }
  }
  if(memtype == SHMEM_POSIX)
  {
    b_isposixruning = false;
    if(b_isstreamon_)
    {
      while (!b_isposhmwritedone_)
      {
        continue;
      }
    }
    auto retshmem = IPCPosixSharedMemory::getInstance().ClosePosixShmemory(&h_shmposix_,
                    frame_count, size, sizeof(unsigned int), str_shmemname_, shmemfd_);
    if (retshmem != POSHMEM_COMM_OK)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "ClosePosixShmemory error %d \n", retshmem);
    }
    h_shmposix_ = NULL;
  }
  else
  {
    b_issystemvruning = false;
    if(b_isstreamon_)
    {
      while (!b_issyshmwritedone_)
      {
        continue;
      }
    }
    auto retshmem = IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
    if (retshmem != SHMEM_COMM_OK)
      PMLOG_ERROR(CONST_MODULE_DC, "CloseShmemory error %d \n", retshmem);
    h_shmsystem_ = NULL;
  }
  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startCapture(void *handle, CAMERA_FORMAT sformat,
                                                 const std::string& imagepath)
{
  PMLOG_INFO(CONST_MODULE_DC, "startCapture started !\n");

  cam_handle_ = handle;
  informat_.nHeight = sformat.nHeight;
  informat_.nWidth = sformat.nWidth;
  informat_.eFormat = sformat.eFormat;
  b_iscontinuous_capture_ = true;
  str_imagepath_ = imagepath;

  // create thread that will continuously capture images until stopcapture received
  tidCapture = std::thread{[this]() { this->captureThread(); }};

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

DEVICE_RETURN_CODE_T DeviceControl::captureImage(void *handle, int ncount, CAMERA_FORMAT sformat,
                                                 const std::string& imagepath,
                                                 const std::string& mode)
{
  PMLOG_INFO(CONST_MODULE_DC, "captureImage started ncount : %d \n", ncount);

  // update image locstion if there is a change
  if (str_imagepath_ != imagepath)
    str_imagepath_ = imagepath;

  if (str_capturemode_ != mode)
    str_capturemode_ = mode;

  // validate if saved format and capture image format are same or not
  if (DEVICE_OK != checkFormat(handle, sformat))
  {
    PMLOG_ERROR(CONST_MODULE_DC, "captureImage : checkFormat failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  // poll for data on buffers and save captured image
  auto retval = pollForCapturedImage(handle, ncount);
  if (retval != DEVICE_OK)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "captureImage : pollForCapturedImage failed \n");
    return retval;
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::createHandle(void **handle, std::string subsystem)
{
  PMLOG_INFO(CONST_MODULE_DC, "createHandle started \n");

  void *p_cam_handle;
  auto ret = camera_hal_if_init(&p_cam_handle, subsystem.c_str());
  if (ret != CAMERA_ERROR_NONE)
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

  auto ret = camera_hal_if_deinit(handle);
  if (ret != CAMERA_ERROR_NONE)
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

  auto ret = camera_hal_if_get_info(strdevicenode.c_str(), pinfo);
  if (ret != CAMERA_ERROR_NONE)
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
  out_params.n_pan = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_tilt = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_contrast = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_brightness = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_saturation = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_sharpness = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_hue = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_gain = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_gamma = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_frequency = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_autowhitebalance = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_backlightcompensation = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_exposure = CONST_PARAM_DEFAULT_VALUE;
  out_params.n_whitebalancetemperature = CONST_PARAM_DEFAULT_VALUE;
  camera_hal_if_get_properties(handle, &out_params);

  oparams->nPan = out_params.n_pan;
  oparams->nTilt = out_params.n_tilt;
  oparams->nContrast = out_params.n_contrast;
  oparams->nBrightness = out_params.n_brightness;
  oparams->nSaturation = out_params.n_saturation;
  oparams->nSharpness = out_params.n_sharpness;
  oparams->nHue = out_params.n_hue;
  oparams->nGain = out_params.n_gain;
  oparams->nGamma = out_params.n_gamma;
  oparams->nFrequency = out_params.n_frequency;
  oparams->nAutoWhiteBalance = out_params.n_autowhitebalance;
  oparams->nBacklightCompensation = out_params.n_backlightcompensation;
  if (oparams->nAutoExposure == 0)
    oparams->nExposure = out_params.n_exposure;
  if (oparams->nAutoWhiteBalance == 0)
    oparams->nWhiteBalanceTemperature = out_params.n_whitebalancetemperature;
  // update resolution structure
  oparams->stResolution.n_formatindex = out_params.st_resolution.n_formatindex;
  for (int n = 0; n < out_params.st_resolution.n_formatindex; n++)
  {
    oparams->stResolution.e_format[n] = out_params.st_resolution.e_format[n];
    oparams->stResolution.n_frameindex[n] = out_params.st_resolution.n_frameindex[n];
    for (int count = 0; count < out_params.st_resolution.n_frameindex[n]; count++)
    {
      oparams->stResolution.n_height[n][count] = out_params.st_resolution.n_height[n][count];
      oparams->stResolution.n_width[n][count] = out_params.st_resolution.n_width[n][count];
      PMLOG_INFO(CONST_MODULE_DC, "out_params.st_resolution.c_res %s\n",
                 out_params.st_resolution.c_res[count]);
      memset(oparams->stResolution.c_res[count], '\0',
             sizeof(oparams->stResolution.c_res[count]));
      strncpy(oparams->stResolution.c_res[count], out_params.st_resolution.c_res[count],
              sizeof(oparams->stResolution.c_res[count])-1);
    }
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setDeviceProperty(void *handle, CAMERA_PROPERTIES_T *inparams)
{
  PMLOG_INFO(CONST_MODULE_DC, "setDeviceProperty started!\n");

  camera_properties_t in_params;

  if (inparams->nZoomAbsolute != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_zoomabsolute = inparams->nZoomAbsolute;

  if (inparams->nPan != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_pan = inparams->nPan;

  if (inparams->nTilt != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_tilt = inparams->nTilt;

  if (inparams->nContrast != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_contrast = inparams->nContrast;

  if (inparams->nBrightness != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_brightness = inparams->nBrightness;

  if (inparams->nSaturation != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_saturation = inparams->nSaturation;

  if (inparams->nSharpness != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_sharpness = inparams->nSharpness;

  if (inparams->nHue != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_hue = inparams->nHue;

  if (inparams->nAutoExposure != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_autoexposure = inparams->nAutoExposure;

  if (inparams->nAutoWhiteBalance != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_autowhitebalance = inparams->nAutoWhiteBalance;

  if (inparams->nExposure != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_exposure = inparams->nExposure;

  if (inparams->nWhiteBalanceTemperature != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_whitebalancetemperature = inparams->nWhiteBalanceTemperature;

  if (inparams->nGain != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_gain = inparams->nGain;

  if (inparams->nGamma != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_gamma = inparams->nGamma;

  if (inparams->nFrequency != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_frequency = inparams->nFrequency;

  if (inparams->nBacklightCompensation != CONST_PARAM_DEFAULT_VALUE)
    in_params.n_backlightcompensation = inparams->nBacklightCompensation;

  camera_hal_if_set_properties(handle, &in_params);

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setFormat(void *handle, CAMERA_FORMAT sformat)
{
  PMLOG_INFO(CONST_MODULE_DC, "sFormat Height %d Width %d Format %d Fps : %d\n", sformat.nHeight,
             sformat.nWidth, sformat.eFormat, sformat.nFps);

  stream_format_t in_format = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
  in_format.stream_height = sformat.nHeight;
  in_format.stream_width = sformat.nWidth;
  in_format.stream_fps = sformat.nFps;
  in_format.pixel_format = getPixelFormat(sformat.eFormat);
  //error handling
  if(in_format.pixel_format == CAMERA_PIXEL_FORMAT_MAX)
    return DEVICE_ERROR_UNSUPPORTED_FORMAT;

  auto ret = camera_hal_if_set_format(handle, in_format);
  if (ret != CAMERA_ERROR_NONE)
    return DEVICE_ERROR_UNSUPPORTED_FORMAT;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getFormat(void *handle, CAMERA_FORMAT *pformat)
{
  // get current saved format for device
  stream_format_t streamformat;
  camera_hal_if_get_format(handle, &streamformat);
  pformat->nHeight = streamformat.stream_height;
  pformat->nWidth = streamformat.stream_width;
  pformat->eFormat = getCameraFormat(streamformat.pixel_format);
  pformat->nFps = streamformat.stream_fps;
  return DEVICE_OK;
}

camera_format_t DeviceControl::getCameraFormat(camera_pixel_format_t eformat)
{
  // convert camera_pixel_format_t to CAMERA_FORMAT_T
  if (eformat == CAMERA_PIXEL_FORMAT_H264)
  {
    return CAMERA_FORMAT_H264ES;
  }
  else if (eformat == CAMERA_PIXEL_FORMAT_YUYV)
  {
    return CAMERA_FORMAT_YUV;
  }
  else if (eformat == CAMERA_PIXEL_FORMAT_JPEG)
  {
    return CAMERA_FORMAT_JPEG;
  }
  //error case
  return CAMERA_FORMAT_UNDEFINED;
}
