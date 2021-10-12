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

#include <signal.h>
#include <errno.h>
#include <algorithm>

/**
 * need to call directly camera base methods in order to cancel preview when the camera is disconnected.
 */
#include "camera_base_wrapper.h"

/**
 * added for shared memory clean-up when the service crashes
 */
#include <sys/ipc.h>

typedef enum
{
  SHMEM_COMM_MARK_NORMAL = 0x0,
  SHMEM_COMM_MARK_RESET = 0x1,
  SHMEM_COMM_MARK_TERMINATE = 0x2
} SHMEM_MARK_T;

typedef struct
{
  int shmem_id;
  int sema_id;

  /*shared memory overhead*/
  int *write_index;
  int *read_index;
  int *unit_size;
  int *unit_num;
  SHMEM_MARK_T *mark;

  unsigned int *length_buf;
  unsigned char *data_buf;

  int *extra_size;
  unsigned char *extra_buf;
} SHMEM_COMM_T;
/**
 * end
 */


int DeviceControl::n_imagecount_ = 0;

DeviceControl::DeviceControl()
    : b_iscontinuous_capture_(false), b_isstreamon_(false), b_isposixruning(false),
      b_issystemvruning(false), b_isshmwritedone_(true), b_issyshmwritedone_(true),
      b_isposhmwritedone_(true), cam_handle_(NULL), shmemfd_(-1), informat_(), epixelformat_(CAMERA_PIXEL_FORMAT_JPEG),
      tMutex(), tCondVar(), h_shmsystem_(NULL), h_shmposix_(NULL),
      str_imagepath_(cstr_empty), str_capturemode_(cstr_oneshot), str_memtype_(""),
      str_shmemname_(""), cancel_preview_(false), buf_size_(0)
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

  PMLOG_INFO(CONST_MODULE_DC, "path : %s\n", path.c_str());

  if (NULL == (fp = fopen(path.c_str(), "w")))
  {
    PMLOG_INFO(CONST_MODULE_DC, "path : fopen failed\n");
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
    PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_get_format failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  PMLOG_INFO(CONST_MODULE_DC, "stream_format_t pixel_format : %d \n",
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
    PMLOG_INFO(CONST_MODULE_DC, "Stored format and new format are different\n");
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
      PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_set_format failed \n");
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
  PMLOG_INFO(CONST_MODULE_DC, "camera_hal_if_get_fd fd : %d \n", fd);
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

  int timeout = 10000;
  buffer_t frame_buffer;
  for (int i = 1; i <= ncount; i++)
  {
    if ((retval = poll(poll_set, 1, timeout)) > 0)
    {
      frame_buffer.start = malloc(framesize);
      retval = camera_hal_if_get_buffer(handle, &frame_buffer);
      if (CAMERA_ERROR_NONE != retval)
      {
        PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_get_buffer failed \n");
        return DEVICE_ERROR_UNKNOWN;
      }
      PMLOG_INFO(CONST_MODULE_DC, "buffer start : %p \n", frame_buffer.start);
      PMLOG_INFO(CONST_MODULE_DC, "buffer length : %lu \n",
                 frame_buffer.length);

      // write captured image to /tmp only if startCapture request is made
      if (DEVICE_ERROR_CANNOT_WRITE == writeImageToFile(frame_buffer.start, frame_buffer.length))
        return DEVICE_ERROR_CANNOT_WRITE;

      free(frame_buffer.start);
      frame_buffer.start = nullptr;

      retval = camera_hal_if_release_buffer(handle, frame_buffer);
      if (retval != CAMERA_ERROR_NONE)
      {
        PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_release_buffer failed \n");
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
  PMLOG_INFO(CONST_MODULE_DC, "started\n");

  // run capture thread until stopCapture received
  while (b_iscontinuous_capture_)
  {
    auto ret =
        captureImage(cam_handle_, 1, informat_, str_imagepath_, cstr_continuous);
    if (ret != DEVICE_OK)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "captureImage failed \n");
      break;
    }
  }
  // set continuous capture to false
  b_iscontinuous_capture_ = false;
  tidCapture.detach();
  n_imagecount_ = 0;
  PMLOG_INFO(CONST_MODULE_DC, "ended\n");
  return;
}

void DeviceControl::previewThread()
{
  PMLOG_INFO(CONST_MODULE_DC, "cam_handle(%p) start!", cam_handle_);
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

  int debug_counter = 0;
  int debug_interval = 100; // frames
  auto tic = std::chrono::steady_clock::now();

  while (b_isstreamon_)
  {
    buffer_t frame_buffer;
    frame_buffer.start = malloc(framesize);
    auto retval = camera_hal_if_get_buffer(cam_handle_, &frame_buffer);
    if (retval != CAMERA_ERROR_NONE)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_get_buffer failed \n");
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
       broadcast_();
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
      PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_release_buffer failed \n");
      break;
    }

    if (++debug_counter >= debug_interval)
    {
      auto toc = std::chrono::steady_clock::now();
      auto us = std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count();
      PMLOG_INFO(CONST_MODULE_DC, "previewThread cam_handle_(%p) : fps(%3.2f), clients(%u)",
                 cam_handle_, debug_interval * 1000000.0f / us, client_pool_.size());
      tic = toc;
      debug_counter = 0;
    }
  }

  b_isshmwritedone_ = true;
  b_issyshmwritedone_ = true;
  b_isposhmwritedone_ = true;
  tCondVar.notify_one();
  tidPreview.detach();
  PMLOG_INFO(CONST_MODULE_DC, "cam_handle(%p) end!", cam_handle_);
  return;
}

DEVICE_RETURN_CODE_T DeviceControl::open(void *handle, std::string devicenode)
{
  PMLOG_INFO(CONST_MODULE_DC, "started\n");

  strdevicenode_ = devicenode;

  // open camera device
  auto ret = camera_hal_if_open_device(handle, devicenode.c_str());

  if (ret != CAMERA_ERROR_NONE)
    return DEVICE_ERROR_CAN_NOT_OPEN;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::close(void *handle)
{
    PMLOG_INFO(CONST_MODULE_DC, "started \n");

    if (cancel_preview_ == true)
    {
        if (-1 == close_device((camera_handle_t*)handle))
        {
            return DEVICE_ERROR_CAN_NOT_CLOSE;
        }
        return DEVICE_OK;
    }
    else
    {
        // close device
        auto ret = camera_hal_if_close_device(handle);
        if (ret != CAMERA_ERROR_NONE)
            return DEVICE_ERROR_CAN_NOT_CLOSE;
        return DEVICE_OK;
    }
}

DEVICE_RETURN_CODE_T DeviceControl::startPreview(void *handle, std::string memtype,
                                                 int *pkey)
{
  PMLOG_INFO(CONST_MODULE_DC, "started !\n");

  cam_handle_  = handle;
  str_memtype_ = memtype;

  // get current saved format for device
  stream_format_t streamformat;
  camera_hal_if_get_format(handle, &streamformat);
  PMLOG_INFO(CONST_MODULE_DC, "Driver set width : %d height : %d", streamformat.stream_width,
             streamformat.stream_height);

  buf_size_ = streamformat.stream_width * streamformat.stream_height * buffer_count + extra_buffer;

  if(memtype == kMemtypeShmem)
  {
    auto retshmem = IPCSharedMemory::getInstance().CreateShmemory(&h_shmsystem_,
                  pkey, buf_size_, frame_count, sizeof(unsigned int));
    if (retshmem != SHMEM_COMM_OK)
       PMLOG_ERROR(CONST_MODULE_DC, "CreateShmemory error %d \n", retshmem);
  }
  else
  {
    std::string shmname = "";
    auto retshmem = IPCPosixSharedMemory::getInstance().CreatePosixShmemory(&h_shmposix_,
                             buf_size_, frame_count, sizeof(unsigned int), pkey, &shmname);
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
      PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_set_buffer failed \n");
      return DEVICE_ERROR_UNKNOWN;
    }

    retval = camera_hal_if_start_capture(handle);
    if (retval != CAMERA_ERROR_NONE)
    {
      PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_start_capture failed \n");
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
    PMLOG_INFO(CONST_MODULE_DC, "started !\n");

    b_isstreamon_ = false;
    if (tidPreview.joinable())
    {
        tidPreview.join();
    }    

    if (cancel_preview_ == true)
    {
        stop_capture((camera_handle_t*)cam_handle_);
        destroy_buffer((camera_handle_t*)cam_handle_);
        PMLOG_INFO(CONST_MODULE_DC, "releasing camera resources by directly calling base methods has at least been tried.\n");
    }
    else
    {  
        auto retval = camera_hal_if_stop_capture(handle);
        if (retval != CAMERA_ERROR_NONE)
        {
            PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_stop_capture failed \n");
            return DEVICE_ERROR_UNKNOWN;
        }
        retval = camera_hal_if_destroy_buffer(handle);
        if (retval != CAMERA_ERROR_NONE)
        {
            PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_destroy_buffer failed \n");
            return DEVICE_ERROR_UNKNOWN;
        }
    }

    if(memtype == SHMEM_POSIX)
    {
        b_isposixruning = false;
        if (h_shmposix_)
        {
            auto retshmem = IPCPosixSharedMemory::getInstance().ClosePosixShmemory(&h_shmposix_,
                      frame_count, buf_size_, sizeof(unsigned int), str_shmemname_, shmemfd_);
            if (retshmem != POSHMEM_COMM_OK)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "ClosePosixShmemory error %d \n", retshmem);
            }
            h_shmposix_ = NULL;
        }
    }
    else
    {   
        b_issystemvruning = false;
        if (h_shmsystem_ != nullptr)
        {
            auto retshmem = IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
            if (retshmem != SHMEM_COMM_OK)
                PMLOG_ERROR(CONST_MODULE_DC, "CloseShmemory error %d \n", retshmem);
            h_shmsystem_ = NULL;
        }
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startCapture(void *handle, CAMERA_FORMAT sformat,
                                                 const std::string& imagepath)
{
  PMLOG_INFO(CONST_MODULE_DC, "started !\n");

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
  PMLOG_INFO(CONST_MODULE_DC, "started !\n");

  // if capture thread is running, stop capture
  if (b_iscontinuous_capture_) {
    b_iscontinuous_capture_ = false;
    tidCapture.join();
  }
  else
    return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::captureImage(void *handle, int ncount, CAMERA_FORMAT sformat,
                                                 const std::string& imagepath,
                                                 const std::string& mode)
{
  PMLOG_INFO(CONST_MODULE_DC, "started ncount : %d \n", ncount);

  // update image locstion if there is a change
  if (str_imagepath_ != imagepath)
    str_imagepath_ = imagepath;

  if (str_capturemode_ != mode)
    str_capturemode_ = mode;

  // validate if saved format and capture image format are same or not
  if (DEVICE_OK != checkFormat(handle, sformat))
  {
    PMLOG_ERROR(CONST_MODULE_DC, "checkFormat failed \n");
    return DEVICE_ERROR_UNKNOWN;
  }

  // poll for data on buffers and save captured image
  auto retval = pollForCapturedImage(handle, ncount);
  if (retval != DEVICE_OK)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "pollForCapturedImage failed \n");
    return retval;
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::createHandle(void **handle, std::string subsystem)
{
  PMLOG_INFO(CONST_MODULE_DC, "started \n");

  void *p_cam_handle;
  auto ret = camera_hal_if_init(&p_cam_handle, subsystem.c_str());
  if (ret != CAMERA_ERROR_NONE)
  {
    PMLOG_ERROR(CONST_MODULE_DC, "Failed to create handle\n!!");
    *handle = NULL;
    return DEVICE_ERROR_UNKNOWN;
  }
  PMLOG_INFO(CONST_MODULE_DC, "p_cam_handle : %p \n", p_cam_handle);
  *handle = p_cam_handle;

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::destroyHandle(void *handle)
{
  PMLOG_INFO(CONST_MODULE_DC, "started \n");

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
  PMLOG_INFO(CONST_MODULE_DC, "started \n");

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
  PMLOG_INFO(CONST_MODULE_DC, "started count : %d \n", ncount);

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
  PMLOG_INFO(CONST_MODULE_DC, "started !\n");

  camera_properties_t out_params;

  out_params.nPan = CONST_PARAM_DEFAULT_VALUE;
  out_params.nTilt = CONST_PARAM_DEFAULT_VALUE;
  out_params.nContrast = CONST_PARAM_DEFAULT_VALUE;
  out_params.nBrightness = CONST_PARAM_DEFAULT_VALUE;
  out_params.nSaturation = CONST_PARAM_DEFAULT_VALUE;
  out_params.nSharpness = CONST_PARAM_DEFAULT_VALUE;
  out_params.nHue = CONST_PARAM_DEFAULT_VALUE;
  out_params.nGain = CONST_PARAM_DEFAULT_VALUE;
  out_params.nGamma = CONST_PARAM_DEFAULT_VALUE;
  out_params.nFrequency = CONST_PARAM_DEFAULT_VALUE;
  out_params.nAutoWhiteBalance = CONST_PARAM_DEFAULT_VALUE;
  out_params.nBacklightCompensation = CONST_PARAM_DEFAULT_VALUE;
  out_params.nExposure = CONST_PARAM_DEFAULT_VALUE;
  out_params.nWhiteBalanceTemperature = CONST_PARAM_DEFAULT_VALUE;
  out_params.nAutoExposure = CONST_PARAM_DEFAULT_VALUE;
  out_params.nZoomAbsolute = CONST_PARAM_DEFAULT_VALUE;
  out_params.nFocusAbsolute =CONST_PARAM_DEFAULT_VALUE;
  out_params.nAutoFocus = CONST_PARAM_DEFAULT_VALUE;


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
  oparams->nAutoWhiteBalance = out_params.nAutoWhiteBalance;
  oparams->nBacklightCompensation = out_params.nBacklightCompensation;
  oparams->nExposure = out_params.nExposure;
  oparams->nWhiteBalanceTemperature = out_params.nWhiteBalanceTemperature;

  oparams->nAutoExposure = out_params.nAutoExposure;
  oparams->nZoomAbsolute = out_params.nZoomAbsolute;
  oparams->nFocusAbsolute = out_params.nFocusAbsolute;
  oparams->nAutoFocus = out_params.nAutoFocus;

  //update stGetData
  for (int i = 0; i < PROPERTY_END; i++)
  {
     for (int j = 0; j < QUERY_END; j++)
	 {
          oparams->stGetData.data[i][j] = out_params.stGetData.data[i][j];

		   PMLOG_INFO(CONST_MODULE_DC, "out_params.stGetData[%d][%d]:%d\n", i, j, out_params.stGetData.data[i][j]);
     }
  }

  // update resolution structure
  oparams->stResolution.n_formatindex = out_params.stResolution.n_formatindex;
  for (int n = 0; n < out_params.stResolution.n_formatindex; n++)
  {
    oparams->stResolution.e_format[n] = out_params.stResolution.e_format[n];
    oparams->stResolution.n_frameindex[n] = out_params.stResolution.n_frameindex[n];
    out_params.stResolution.n_framecount[n] = out_params.stResolution.n_frameindex[n] + 1;
    oparams->stResolution.n_framecount[n] = out_params.stResolution.n_framecount[n];
    for (int count = 0; count < out_params.stResolution.n_framecount[n]; count++)
    {
      oparams->stResolution.n_height[n][count] = out_params.stResolution.n_height[n][count];
      oparams->stResolution.n_width[n][count] = out_params.stResolution.n_width[n][count];
      PMLOG_INFO(CONST_MODULE_DC, "out_params.stResolution.c_res %s\n",
                 out_params.stResolution.c_res[n][count]);
      memset(oparams->stResolution.c_res[n][count], '\0',
             sizeof(oparams->stResolution.c_res[n][count]));
      strncpy(oparams->stResolution.c_res[n][count], out_params.stResolution.c_res[n][count],
              sizeof(oparams->stResolution.c_res[n][count])-1);
    }
  }

  return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setDeviceProperty(void *handle, CAMERA_PROPERTIES_T *inparams)
{
  PMLOG_INFO(CONST_MODULE_DC, "started!\n");

  camera_properties_t in_params;
  in_params.nFocusAbsolute = inparams->nFocusAbsolute;
  in_params.nAutoFocus = inparams->nAutoFocus;
  in_params.nZoomAbsolute = inparams->nZoomAbsolute;
  in_params.nPan = inparams->nPan;
  in_params.nTilt = inparams->nTilt;
  in_params.nContrast = inparams->nContrast;
  in_params.nBrightness = inparams->nBrightness;
  in_params.nSaturation = inparams->nSaturation;
  in_params.nSharpness = inparams->nSharpness;
  in_params.nHue = inparams->nHue;
  in_params.nAutoExposure = inparams->nAutoExposure;
  in_params.nAutoWhiteBalance = inparams->nAutoWhiteBalance;
  in_params.nExposure = inparams->nExposure;
  in_params.nWhiteBalanceTemperature = inparams->nWhiteBalanceTemperature;
  in_params.nGain = inparams->nGain;
  in_params.nGamma = inparams->nGamma;
  in_params.nFrequency = inparams->nFrequency;
  in_params.nBacklightCompensation = inparams->nBacklightCompensation;

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


bool DeviceControl::registerClient(pid_t pid, int sig, int devhandle, std::string& outmsg)
{
  std::lock_guard<std::mutex> mlock(client_pool_mutex_);
  {
    auto it = std::find_if(client_pool_.begin(), client_pool_.end(),
                           [=](const CLIENT_INFO_T& p) {
                             return p.pid == pid;
                           });

    if (it == client_pool_.end())
    {
      CLIENT_INFO_T p = {pid, sig, devhandle};
      client_pool_.push_back(p);
      outmsg = "The client of pid " + std::to_string(pid) + " registered with sig " + std::to_string(sig) + " :: OK";
      return true;
    }
    else
    {
      outmsg = "The client of pid " + std::to_string(pid) + " is already registered :: ignored";
      PMLOG_INFO(CONST_MODULE_DC, "%s", outmsg.c_str());
      return false;
    }
  }
}

bool DeviceControl::unregisterClient(pid_t pid, std::string& outmsg)
{
  std::lock_guard<std::mutex> mlock(client_pool_mutex_);
  {
    auto it = std::find_if(client_pool_.begin(), client_pool_.end(),
                           [=](const CLIENT_INFO_T& p) {
                             return p.pid == pid;
                           });

    if (it != client_pool_.end())
    {
      client_pool_.erase(it);
      outmsg = "The client of pid " + std::to_string(pid) + " unregistered :: OK";
      PMLOG_INFO(CONST_MODULE_DC, "%s", outmsg.c_str());
      return true;
    }
    else
    {
      outmsg = "No client of pid " + std::to_string(pid) + " exists :: ignored";
      PMLOG_INFO(CONST_MODULE_DC, "%s", outmsg.c_str());
      return false;
    }
  }
}

void DeviceControl::broadcast_()
{
  std::lock_guard<std::mutex> mlock(client_pool_mutex_);
  {
    PMLOG_DEBUG("Broadcasting to %u clients\n", client_pool_.size());
    
    auto it = client_pool_.begin();
    while (it != client_pool_.end())
    {

      PMLOG_DEBUG("About to send a signal %d to the client of pid %d ...\n", it->sig, it->pid);
      int errid = kill(it->pid, it->sig);
      if (errid == -1)
      {
        switch (errno)
        {
          case ESRCH:
            PMLOG_ERROR(CONST_MODULE_DC, "The client of pid %d does not exist and will be removed from the pool!!", it->pid);
            // remove this pid from the client pool to make ensure no zombie process exists.
            it = client_pool_.erase(it);
            break;
          case EINVAL:
            PMLOG_ERROR(CONST_MODULE_DC, "The client of pid %d was given an invalid signal %d and will be be removed from the pool!!", it->pid, it->sig);
            it = client_pool_.erase(it);
            break;
          default: // case errno = EPERM
            PMLOG_ERROR(CONST_MODULE_DC, "Unexpected error in sending the signal to the client of pid %d\n", it->pid);
            break;
        }
      }
      else
      {
        ++it;
      }
    }
  }
}

void DeviceControl::requestPreviewCancel()
{
     cancel_preview_ = true;
}