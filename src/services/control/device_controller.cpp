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
 (File Inclusions)
 ----------------------------------------------------------------------------*/
#include "device_controller.h"
#include "camera_hal_if.h"
#include "camera_solution_event.h"
#include "camera_solution_manager.h"
#include "command_manager.h"
#include "device_manager.h"
#include <algorithm>
#include <ctime>
#include <errno.h>
#include <pbnjson.h>
#include <poll.h>
#include <signal.h>
#include <sys/time.h>

/**
 * need to call directly camera base methods in order to cancel preview when the camera is
 * disconnected.
 */
#include "camera_base_wrapper.h"

struct MemoryListener : public CameraSolutionEvent
{
    MemoryListener(void) { PMLOG_INFO(CONST_MODULE_DC, ""); }
    virtual ~MemoryListener(void)
    {
        if (jsonResult_ != nullptr)
            j_release(&jsonResult_);
        jsonResult_ = nullptr;
        PMLOG_INFO(CONST_MODULE_DC, "");
    }
    virtual void onInitialized(void) override { PMLOG_INFO(CONST_MODULE_DC, "It's initialized!!"); }
    virtual void onDone(jvalue_ref jsonObj) override
    {
        std::lock_guard<std::mutex> lg(mtxResult_);
        // TODO : We need to composite more than one results.
        //        e.g) {"faces":[{...}, {...}], "poses":[{...}, {...}]}
        j_release(&jsonResult_);
        jsonResult_ = jvalue_copy(jsonObj);
        PMLOG_INFO(CONST_MODULE_DC, "Solution Result: %s", jvalue_stringify(jsonResult_));
    }
    std::string getResult(void)
    {
        std::lock_guard<std::mutex> lg(mtxResult_);
        if (jsonResult_ != nullptr)
        {
            const char *jvalue = jvalue_stringify(jsonResult_);
            if (jvalue)
                return jvalue;
        }
        return "";
    }
    std::mutex mtxResult_;
    jvalue_ref jsonResult_{nullptr};
};

int DeviceControl::n_imagecount_ = 0;

DeviceControl::DeviceControl()
    : b_iscontinuous_capture_(false), b_isstreamon_(false), b_isposixruning(false),
      b_issystemvruning_mmap(false), b_issystemvruning(false), cam_handle_(nullptr), shmemfd_(-1),
      usrpbufs_(nullptr), informat_(), epixelformat_(CAMERA_PIXEL_FORMAT_JPEG), tMutex(),
      tCondVar(), h_shmsystem_(nullptr), h_shmposix_(nullptr), str_imagepath_(cstr_empty),
      str_capturemode_(cstr_oneshot), str_memtype_(""), str_shmemname_(""), cancel_preview_(false),
      buf_size_(0), sh_(nullptr), subskey_(""), camera_id_(-1)
{
    pCameraSolution = std::make_shared<CameraSolutionManager>();
    pMemoryListener = std::make_shared<MemoryListener>();
    if (pCameraSolution != nullptr && pMemoryListener != nullptr)
    {
        pCameraSolution->setEventListener(pMemoryListener.get());
    }
}

DEVICE_RETURN_CODE_T DeviceControl::writeImageToFile(const void *p, int size) const
{
    FILE *fp;
    char image_name[100] = {};

    auto path = str_imagepath_;
    if (path.empty())
        path = "/tmp/";

    // find the file extension to check if file name is provided or path is provided
    std::size_t position  = path.find_last_of(".");
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

        time_t t    = time(NULL);
        tm *timePtr = localtime(&t);
        if (timePtr == nullptr)
        {
            PMLOG_ERROR(CONST_MODULE_DC, "localtime() given null ptr");
            return DEVICE_ERROR_UNKNOWN;
        }
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

    PMLOG_INFO(CONST_MODULE_DC, "stream_format_t pixel_format : %d \n", streamformat.pixel_format);

    // save pixel format for saving captured image
    epixelformat_ = streamformat.pixel_format;

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    auto enewformat          = getPixelFormat(sformat.eFormat);
    // error handling
    if (enewformat == CAMERA_PIXEL_FORMAT_MAX)
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
        if (str_memtype_ == kMemtypeShmem || str_memtype_ == kMemtypeShmemMmap)
            memtype = SHMEM_SYSTEMV;
        else
            memtype = SHMEM_POSIX;
        stopPreview(handle, memtype);
        close(handle);
        open(handle, strdevicenode_, camera_id_, payload_);

        // set format again
        stream_format_t newstreamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
        newstreamformat.stream_height   = sformat.nHeight;
        newstreamformat.stream_width    = sformat.nWidth;
        newstreamformat.pixel_format    = getPixelFormat(sformat.eFormat);
        // error handling
        if (newstreamformat.pixel_format == CAMERA_PIXEL_FORMAT_MAX)
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
        ret     = startPreview(handle, str_memtype_, &key, sh_, subskey_.c_str());
    }

    return ret;
}

DEVICE_RETURN_CODE_T DeviceControl::pollForCapturedImage(void *handle, int ncount) const
{
    int fd      = -1;
    auto retval = camera_hal_if_get_fd(handle, &fd);
    PMLOG_INFO(CONST_MODULE_DC, "camera_hal_if_get_fd fd : %d \n", fd);
    struct pollfd poll_set[]
    {
        {.fd = fd, .events = POLLIN},
    };

    // get current saved format for device
    stream_format_t streamformat;
    camera_hal_if_get_format(handle, &streamformat);
    PMLOG_INFO(CONST_MODULE_DC, "Driver set width : %d height : %d", streamformat.stream_width,
               streamformat.stream_height);
    int framesize =
        streamformat.stream_width * streamformat.stream_height * buffer_count + extra_buffer;

    int timeout           = 10000;
    buffer_t frame_buffer = {0};

    for (int i = 1; i <= ncount; i++)
    {
        if ((retval = poll(poll_set, 1, timeout)) > 0)
        {
            retval = camera_hal_if_get_buffer(handle, &frame_buffer);
            if (CAMERA_ERROR_NONE != retval)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_get_buffer failed \n");
                return DEVICE_ERROR_UNKNOWN;
            }
            PMLOG_INFO(CONST_MODULE_DC, "buffer start : %p \n", frame_buffer.start);
            PMLOG_INFO(CONST_MODULE_DC, "buffer length : %lu \n", frame_buffer.length);

            if (frame_buffer.start == nullptr)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "no valid memory on frame buffer ptr");
                return DEVICE_ERROR_OUT_OF_MEMORY;
            }

            //[Camera Solution Manager] processing for capture
            if (pCameraSolution != nullptr)
            {
                pCameraSolution->processCapture(frame_buffer);
            }

            // write captured image to /tmp only if startCapture request is made
            if (DEVICE_ERROR_CANNOT_WRITE ==
                writeImageToFile(frame_buffer.start, frame_buffer.length))
                return DEVICE_ERROR_CANNOT_WRITE;

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
    // error case
    return CAMERA_PIXEL_FORMAT_MAX;
}

void DeviceControl::captureThread()
{
    PMLOG_INFO(CONST_MODULE_DC, "started\n");

    pthread_setname_np(pthread_self(), "capture_thread");

    // run capture thread until stopCapture received
    while (b_iscontinuous_capture_)
    {
        auto ret = captureImage(cam_handle_, 1, informat_, str_imagepath_, cstr_continuous);
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

    pthread_setname_np(pthread_self(), "preview_thread");

    // poll for data on buffers and save captured image
    // lock so that if stop preview is called, first this cycle should complete
    std::lock_guard<std::mutex> guard(tMutex);

    // get current saved format for device
    stream_format_t streamformat;
    camera_hal_if_get_format(cam_handle_, &streamformat);
    PMLOG_INFO(CONST_MODULE_DC, "Driver set width : %d height : %d", streamformat.stream_width,
               streamformat.stream_height);

    // "frame_size" is currently not used actually.
    // int framesize =
    //        streamformat.stream_width * streamformat.stream_height * buffer_count + extra_buffer;

    int debug_counter  = 0;
    int debug_interval = 100; // frames
    auto tic           = std::chrono::steady_clock::now();

    while (b_isstreamon_)
    {
        // keep writing data to shared memory
        unsigned int timestamp = 0;

        buffer_t frame_buffer = {0};

        auto retval = camera_hal_if_get_buffer(cam_handle_, &frame_buffer);
        if (retval != CAMERA_ERROR_NONE)
        {
            PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_get_buffer failed \n");

            notifyDeviceFault_();
            break;
        }

        if (frame_buffer.start == nullptr)
        {
            PMLOG_ERROR(CONST_MODULE_DC, "no valid frame buffer obtained");
            notifyDeviceFault_();
            break;
        }

        //[Camera Solution Manager] process for preview
        if (pCameraSolution != nullptr)
        {
            pCameraSolution->processPreview(frame_buffer);
        }

        auto meta = pMemoryListener->getResult();
        if (b_issystemvruning)
        {
            IPCSharedMemory::getInstance().WriteHeader(h_shmsystem_, frame_buffer.index,
                                                       frame_buffer.length);

            IPCSharedMemory::getInstance().WriteMeta(h_shmsystem_, (unsigned char *)meta.c_str(),
                                                     meta.size() + 1);

            // Time stamp is currently not used actually.
            IPCSharedMemory::getInstance().WriteExtra(h_shmsystem_, (unsigned char *)&timestamp,
                                                      sizeof(timestamp));

            IPCSharedMemory::getInstance().IncrementWriteIndex(h_shmsystem_);

            broadcast_();
        }
        else if (b_issystemvruning_mmap)
        {
            auto retshmem = IPCSharedMemory::getInstance().WriteShmemory(
                h_shmsystem_, (unsigned char *)frame_buffer.start, frame_buffer.length,
                (unsigned char *)meta.c_str(), meta.size() + 1, (unsigned char *)&timestamp,
                sizeof(timestamp));

            if (retshmem != SHMEM_IS_OK)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "Write Shared memory error %d \n", retshmem);
            }
            broadcast_();
        }
        else if (b_isposixruning)
        {
            IPCPosixSharedMemory::getInstance().WriteHeader(h_shmposix_, frame_buffer.index,
                                                            frame_buffer.length);

            // Time stamp is currently not used actually.
            IPCPosixSharedMemory::getInstance().WriteExtra(h_shmposix_, (unsigned char *)&timestamp,
                                                           sizeof(timestamp));

            IPCPosixSharedMemory::getInstance().IncrementWriteIndex(h_shmposix_);
        }

        retval = camera_hal_if_release_buffer(cam_handle_, frame_buffer);
        if (retval != CAMERA_ERROR_NONE)
        {
            PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_release_buffer failed \n");
            notifyDeviceFault_();
            break;
        }

        if (++debug_counter >= debug_interval)
        {
            auto toc = std::chrono::steady_clock::now();
            auto us  = std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count();
            PMLOG_INFO(CONST_MODULE_DC, "previewThread cam_handle_(%p) : fps(%3.2f), clients(%lu)",
                       cam_handle_, debug_interval * 1000000.0f / us, client_pool_.size());
            tic           = toc;
            debug_counter = 0;
        }
    }

    PMLOG_INFO(CONST_MODULE_DC, "cam_handle(%p) end!", cam_handle_);
    return;
}

DEVICE_RETURN_CODE_T DeviceControl::open(void *handle, std::string devicenode, int ndev_id,
                                         std::string payload)
{
    PMLOG_INFO(CONST_MODULE_DC, "started\n");

    strdevicenode_ = devicenode;
    camera_id_     = ndev_id;
    payload_       = payload;

    // open camera device
    auto ret = camera_hal_if_open_device(handle, devicenode.c_str(), payload.c_str());
    if (ret != CAMERA_ERROR_NONE)
        return DEVICE_ERROR_CAN_NOT_OPEN;

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::close(void *handle)
{
    PMLOG_INFO(CONST_MODULE_DC, "started \n");

    if (cancel_preview_ == true)
    {
        if (-1 == close_device((camera_handle_t *)handle))
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

DEVICE_RETURN_CODE_T DeviceControl::startPreview(void *handle, std::string memtype, int *pkey,
                                                 LSHandle *sh, const char *subskey)
{
    PMLOG_INFO(CONST_MODULE_DC, "started !\n");

    cam_handle_  = handle;
    str_memtype_ = memtype;
    sh_          = sh;
    subskey_     = subskey ? subskey : "";

    // get current saved format for device
    stream_format_t streamformat;
    camera_hal_if_get_format(handle, &streamformat);
    PMLOG_INFO(CONST_MODULE_DC, "Driver set width : %d height : %d", streamformat.stream_width,
               streamformat.stream_height);

    // buffer_count = 2 (see "constants.h")
    buf_size_ =
        streamformat.stream_width * streamformat.stream_height * buffer_count + extra_buffer;

    //[Camera Solution Manager] initialization
    int32_t meta_size = 0;
    if (pCameraSolution != nullptr)
    {
        pCameraSolution->initialize(streamformat);
        meta_size = pCameraSolution->getMetaSizeHint();
    }

    if (memtype == kMemtypeShmem || memtype == kMemtypeShmemMmap)
    {
        // frame_count = 8 (see "constants.h")
        auto retshmem = IPCSharedMemory::getInstance().CreateShmemory(
            &h_shmsystem_, pkey, buf_size_, meta_size, frame_count, sizeof(unsigned int));
        if (retshmem != SHMEM_IS_OK)
        {
            PMLOG_ERROR(CONST_MODULE_DC, "CreateShmemory error %d \n", retshmem);
            return DEVICE_ERROR_UNKNOWN;
        }
    }
    else // memtype == kMemtypePosixshm
    {
        std::string shmname = "";

        // frame_count = 8 (see "constants.h")
        auto retshmem = IPCPosixSharedMemory::getInstance().CreateShmemory(
            &h_shmposix_, buf_size_, frame_count, sizeof(unsigned int), pkey, &shmname);
        if (retshmem != PSHMEM_IS_OK)
        {
            PMLOG_ERROR(CONST_MODULE_DC, "CreatePosixShmemory error %d \n", retshmem);
            return DEVICE_ERROR_UNKNOWN;
        }

        shmemfd_       = *pkey;
        str_shmemname_ = shmname;
    }

    if (b_isstreamon_ == false)
    {
        if (memtype == kMemtypeShmem)
        {
            // user pointer buffer set-up.
            usrpbufs_ = (buffer_t *)calloc(frame_count, sizeof(buffer_t));
            if (!usrpbufs_)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "USERPTR buffer allocation failed \n");
                IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
                return DEVICE_ERROR_UNKNOWN;
            }
            IPCSharedMemory::getInstance().GetShmemoryBufferInfo(h_shmsystem_, frame_count,
                                                                 usrpbufs_, nullptr);

            // frame_count = 8 (see "constants.h")
            auto retval = camera_hal_if_set_buffer(handle, frame_count, IOMODE_USERPTR, &usrpbufs_);
            if (retval != CAMERA_ERROR_NONE)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_set_buffer failed \n");
                free(usrpbufs_);
                usrpbufs_ = nullptr;
                IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
                return DEVICE_ERROR_UNKNOWN;
            }

            retval = camera_hal_if_start_capture(handle);
            if (retval != CAMERA_ERROR_NONE)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_start_capture failed \n");
                free(usrpbufs_);
                usrpbufs_ = nullptr;
                IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
                return DEVICE_ERROR_UNKNOWN;
            }

            b_isstreamon_     = true;
            b_issystemvruning = true;
        }
        else if (memtype == kMemtypeShmemMmap)
        {
            auto retval = camera_hal_if_set_buffer(handle, 4, IOMODE_MMAP, nullptr);
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

            b_isstreamon_          = true;
            b_issystemvruning_mmap = true;
        }
        else // memtype == kMemtypePosixshm
        {
            // user pointer buffer set-up.
            usrpbufs_ = (buffer_t *)calloc(frame_count, sizeof(buffer_t));
            if (!usrpbufs_)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "USERPTR buffer allocation failed \n");
                IPCPosixSharedMemory::getInstance().CloseShmemory(&h_shmposix_, frame_count,
                                                                  buf_size_, sizeof(unsigned int),
                                                                  str_shmemname_, shmemfd_);
                return DEVICE_ERROR_UNKNOWN;
            }
            IPCPosixSharedMemory::getInstance().GetShmemoryBufferInfo(h_shmposix_, frame_count,
                                                                      usrpbufs_, nullptr);

            // frame_count = 8 (see "constants.h")
            auto retval = camera_hal_if_set_buffer(handle, frame_count, IOMODE_USERPTR, &usrpbufs_);
            if (retval != CAMERA_ERROR_NONE)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_set_buffer failed \n");
                free(usrpbufs_);
                usrpbufs_ = nullptr;
                IPCPosixSharedMemory::getInstance().CloseShmemory(&h_shmposix_, frame_count,
                                                                  buf_size_, sizeof(unsigned int),
                                                                  str_shmemname_, shmemfd_);
                return DEVICE_ERROR_UNKNOWN;
            }

            retval = camera_hal_if_start_capture(handle);
            if (retval != CAMERA_ERROR_NONE)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "camera_hal_if_start_capture failed \n");
                free(usrpbufs_);
                usrpbufs_ = nullptr;
                IPCPosixSharedMemory::getInstance().CloseShmemory(&h_shmposix_, frame_count,
                                                                  buf_size_, sizeof(unsigned int),
                                                                  str_shmemname_, shmemfd_);
                return DEVICE_ERROR_UNKNOWN;
            }

            b_isstreamon_   = true;
            b_isposixruning = true;
        }

        // create thread that will continuously capture images until stopcapture received
        tidPreview = std::thread{[this]() { this->previewThread(); }};
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::stopPreview(void *handle, int memtype)
{
    PMLOG_INFO(CONST_MODULE_DC, "started !\n");

    b_isstreamon_ = false;

    if (tidPreview.joinable())
    {
        PMLOG_INFO(CONST_MODULE_DC, "Thread Closing");
        try
        {
            tidPreview.join();
        }
        catch (const std::system_error &e)
        {
            PMLOG_ERROR(CONST_MODULE_DC, "Caught a system_error with code %d meaning %s",
                        e.code().value(), e.what());
        }
        PMLOG_INFO(CONST_MODULE_DC, "Thread Closed");
    }

    if (cancel_preview_ == true)
    {
        stop_capture((camera_handle_t *)handle);
        destroy_buffer((camera_handle_t *)handle);
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

    if (memtype == SHMEM_SYSTEMV)
    {
        b_issystemvruning      = false;
        b_issystemvruning_mmap = false;
        if (h_shmsystem_ != nullptr)
        {
            auto retshmem = IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
            if (retshmem != SHMEM_IS_OK)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "CloseShmemory error %d \n", retshmem);
            }
            h_shmsystem_ = nullptr;
        }
    }
    else // memtype == SHMEM_POSIX
    {
        b_isposixruning = false;
        if (h_shmposix_ != nullptr)
        {
            auto retshmem = IPCPosixSharedMemory::getInstance().CloseShmemory(
                &h_shmposix_, frame_count, buf_size_, sizeof(unsigned int), str_shmemname_,
                shmemfd_);
            if (retshmem != PSHMEM_IS_OK)
            {
                PMLOG_ERROR(CONST_MODULE_DC, "ClosePosixShmemory error %d \n", retshmem);
            }
            h_shmposix_ = nullptr;
        }
    }

    if (usrpbufs_ != nullptr)
    {
        free(usrpbufs_);
        usrpbufs_ = nullptr;
    }

    //[]Camera Solution Manager] release
    if (pCameraSolution != nullptr)
    {
        pCameraSolution->release();
    }
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startCapture(void *handle, CAMERA_FORMAT sformat,
                                                 const std::string &imagepath)
{
    PMLOG_INFO(CONST_MODULE_DC, "started !\n");

    cam_handle_             = handle;
    informat_.nHeight       = sformat.nHeight;
    informat_.nWidth        = sformat.nWidth;
    informat_.eFormat       = sformat.eFormat;
    b_iscontinuous_capture_ = true;
    str_imagepath_          = imagepath;

    // create thread that will continuously capture images until stopcapture received
    tidCapture = std::thread{[this]() { this->captureThread(); }};

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::stopCapture(void *handle)
{
    PMLOG_INFO(CONST_MODULE_DC, "started !\n");

    // if capture thread is running, stop capture
    if (b_iscontinuous_capture_)
    {
        b_iscontinuous_capture_ = false;
        tidCapture.join();
    }
    else
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::captureImage(void *handle, int ncount, CAMERA_FORMAT sformat,
                                                 const std::string &imagepath,
                                                 const std::string &mode)
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

std::string DeviceControl::getSubsystem(const std::string deviceType)
{
    return "lib" + deviceType + "-camera-plugin.so";
}

DEVICE_RETURN_CODE_T DeviceControl::createHandle(void **handle, std::string deviceType)
{
    PMLOG_INFO(CONST_MODULE_DC, "started \n");

    void *p_cam_handle;
    std::string subsystem = getSubsystem(deviceType);
    auto ret              = camera_hal_if_init(&p_cam_handle, subsystem.c_str());
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

DEVICE_RETURN_CODE_T DeviceControl::getDeviceInfo(std::string strdevicenode, std::string deviceType,
                                                  camera_device_info_t *pinfo)
{
    PMLOG_INFO(CONST_MODULE_DC, "started \n");

    std::string subsystem = getSubsystem(deviceType);
    auto ret              = camera_hal_if_get_info(strdevicenode.c_str(), subsystem.c_str(), pinfo);
    if (ret != CAMERA_ERROR_NONE)
    {
        PMLOG_ERROR(CONST_MODULE_DC, "Failed to get the info\n!!");
        return DEVICE_ERROR_UNKNOWN;
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getDeviceProperty(void *handle, CAMERA_PROPERTIES_T *oparams)
{
    PMLOG_INFO(CONST_MODULE_DC, "started !\n");

    camera_properties_t out_params;

    out_params.nPan                     = CONST_PARAM_DEFAULT_VALUE;
    out_params.nTilt                    = CONST_PARAM_DEFAULT_VALUE;
    out_params.nContrast                = CONST_PARAM_DEFAULT_VALUE;
    out_params.nBrightness              = CONST_PARAM_DEFAULT_VALUE;
    out_params.nSaturation              = CONST_PARAM_DEFAULT_VALUE;
    out_params.nSharpness               = CONST_PARAM_DEFAULT_VALUE;
    out_params.nHue                     = CONST_PARAM_DEFAULT_VALUE;
    out_params.nGain                    = CONST_PARAM_DEFAULT_VALUE;
    out_params.nGamma                   = CONST_PARAM_DEFAULT_VALUE;
    out_params.nFrequency               = CONST_PARAM_DEFAULT_VALUE;
    out_params.nAutoWhiteBalance        = CONST_PARAM_DEFAULT_VALUE;
    out_params.nBacklightCompensation   = CONST_PARAM_DEFAULT_VALUE;
    out_params.nExposure                = CONST_PARAM_DEFAULT_VALUE;
    out_params.nWhiteBalanceTemperature = CONST_PARAM_DEFAULT_VALUE;
    out_params.nAutoExposure            = CONST_PARAM_DEFAULT_VALUE;
    out_params.nZoomAbsolute            = CONST_PARAM_DEFAULT_VALUE;
    out_params.nFocusAbsolute           = CONST_PARAM_DEFAULT_VALUE;
    out_params.nAutoFocus               = CONST_PARAM_DEFAULT_VALUE;

    if (CAMERA_ERROR_NONE != camera_hal_if_get_properties(handle, &out_params))
    {
        return DEVICE_ERROR_UNKNOWN;
    }

    oparams->nPan                     = out_params.nPan;
    oparams->nTilt                    = out_params.nTilt;
    oparams->nContrast                = out_params.nContrast;
    oparams->nBrightness              = out_params.nBrightness;
    oparams->nSaturation              = out_params.nSaturation;
    oparams->nSharpness               = out_params.nSharpness;
    oparams->nHue                     = out_params.nHue;
    oparams->nGain                    = out_params.nGain;
    oparams->nGamma                   = out_params.nGamma;
    oparams->nFrequency               = out_params.nFrequency;
    oparams->nAutoWhiteBalance        = out_params.nAutoWhiteBalance;
    oparams->nBacklightCompensation   = out_params.nBacklightCompensation;
    oparams->nExposure                = out_params.nExposure;
    oparams->nWhiteBalanceTemperature = out_params.nWhiteBalanceTemperature;

    oparams->nAutoExposure  = out_params.nAutoExposure;
    oparams->nZoomAbsolute  = out_params.nZoomAbsolute;
    oparams->nFocusAbsolute = out_params.nFocusAbsolute;
    oparams->nAutoFocus     = out_params.nAutoFocus;

    // update stGetData
    for (int i = 0; i < PROPERTY_END; i++)
    {
        for (int j = 0; j < QUERY_END; j++)
        {
            oparams->stGetData.data[i][j] = out_params.stGetData.data[i][j];
            PMLOG_DEBUG("out_params.stGetData[%d][%d]:%d\n", i, j, out_params.stGetData.data[i][j]);
        }
    }

    // update resolution structure
    for (auto const &v : out_params.stResolution)
    {
        std::vector<std::string> c_res;
        c_res.clear();
        c_res.assign(v.c_res.begin(), v.c_res.end());
        oparams->stResolution.emplace_back(c_res, v.e_format);
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setDeviceProperty(void *handle, CAMERA_PROPERTIES_T *inparams)
{
    PMLOG_INFO(CONST_MODULE_DC, "started!\n");

    camera_properties_t in_params;
    in_params.nFocusAbsolute           = inparams->nFocusAbsolute;
    in_params.nAutoFocus               = inparams->nAutoFocus;
    in_params.nZoomAbsolute            = inparams->nZoomAbsolute;
    in_params.nPan                     = inparams->nPan;
    in_params.nTilt                    = inparams->nTilt;
    in_params.nContrast                = inparams->nContrast;
    in_params.nBrightness              = inparams->nBrightness;
    in_params.nSaturation              = inparams->nSaturation;
    in_params.nSharpness               = inparams->nSharpness;
    in_params.nHue                     = inparams->nHue;
    in_params.nAutoExposure            = inparams->nAutoExposure;
    in_params.nAutoWhiteBalance        = inparams->nAutoWhiteBalance;
    in_params.nExposure                = inparams->nExposure;
    in_params.nWhiteBalanceTemperature = inparams->nWhiteBalanceTemperature;
    in_params.nGain                    = inparams->nGain;
    in_params.nGamma                   = inparams->nGamma;
    in_params.nFrequency               = inparams->nFrequency;
    in_params.nBacklightCompensation   = inparams->nBacklightCompensation;

    camera_hal_if_set_properties(handle, &in_params);

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setFormat(void *handle, CAMERA_FORMAT sformat)
{
    PMLOG_INFO(CONST_MODULE_DC, "sFormat Height %d Width %d Format %d Fps : %d\n", sformat.nHeight,
               sformat.nWidth, sformat.eFormat, sformat.nFps);

    stream_format_t in_format = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
    in_format.stream_height   = sformat.nHeight;
    in_format.stream_width    = sformat.nWidth;
    in_format.stream_fps      = sformat.nFps;
    in_format.pixel_format    = getPixelFormat(sformat.eFormat);
    // error handling
    if (in_format.pixel_format == CAMERA_PIXEL_FORMAT_MAX)
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
    pformat->nWidth  = streamformat.stream_width;
    pformat->eFormat = getCameraFormat(streamformat.pixel_format);
    pformat->nFps    = streamformat.stream_fps;
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
    // error case
    return CAMERA_FORMAT_UNDEFINED;
}

bool DeviceControl::registerClient(pid_t pid, int sig, int devhandle, std::string &outmsg)
{
    std::lock_guard<std::mutex> mlock(client_pool_mutex_);
    {
        auto it = std::find_if(client_pool_.begin(), client_pool_.end(),
                               [=](const CLIENT_INFO_T &p) { return p.pid == pid; });

        if (it == client_pool_.end())
        {
            CLIENT_INFO_T p = {pid, sig, devhandle};
            client_pool_.push_back(p);
            outmsg = "The client of pid " + std::to_string(pid) + " registered with sig " +
                     std::to_string(sig) + " :: OK";
            return true;
        }
        else
        {
            outmsg =
                "The client of pid " + std::to_string(pid) + " is already registered :: ignored";
            PMLOG_INFO(CONST_MODULE_DC, "%s", outmsg.c_str());
            return false;
        }
    }
}

bool DeviceControl::unregisterClient(pid_t pid, std::string &outmsg)
{
    std::lock_guard<std::mutex> mlock(client_pool_mutex_);
    {
        auto it = std::find_if(client_pool_.begin(), client_pool_.end(),
                               [=](const CLIENT_INFO_T &p) { return p.pid == pid; });

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

bool DeviceControl::isRegisteredClient(int devhandle)
{
    auto it = std::find_if(client_pool_.begin(), client_pool_.end(),
                           [=](const CLIENT_INFO_T &p) { return p.handle == devhandle; });
    if (it == client_pool_.end())
    {
        return false;
    }
    return true;
}

void DeviceControl::broadcast_()
{
    std::lock_guard<std::mutex> mlock(client_pool_mutex_);
    {
        PMLOG_DEBUG("Broadcasting to %lu clients\n", client_pool_.size());

        auto it = client_pool_.begin();
        while (it != client_pool_.end())
        {

            PMLOG_DEBUG("About to send a signal %d to the client of pid %d ...\n", it->sig,
                        it->pid);
            int errid = kill(it->pid, it->sig);
            if (errid == -1)
            {
                switch (errno)
                {
                case ESRCH:
                    PMLOG_ERROR(
                        CONST_MODULE_DC,
                        "The client of pid %d does not exist and will be removed from the pool!!",
                        it->pid);
                    // remove this pid from the client pool to make ensure no zombie process exists.
                    it = client_pool_.erase(it);
                    break;
                case EINVAL:
                    PMLOG_ERROR(CONST_MODULE_DC,
                                "The client of pid %d was given an invalid signal "
                                "%d and will be be removed from the pool!!",
                                it->pid, it->sig);
                    it = client_pool_.erase(it);
                    break;
                default: // case errno = EPERM
                    PMLOG_ERROR(CONST_MODULE_DC,
                                "Unexpected error in sending the signal to the client of pid %d\n",
                                it->pid);
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

void DeviceControl::requestPreviewCancel() { cancel_preview_ = true; }

void DeviceControl::notifyDeviceFault_()
{
    int num_subscribers = 0;
    std::string reply;
    LSError lserror;
    LSErrorInit(&lserror);

    if (subskey_ != "")
    {
        num_subscribers = LSSubscriptionGetHandleSubscribersCount(sh_, subskey_.c_str());
        int fd          = -1;
        camera_hal_if_get_fd(cam_handle_, &fd);
        PMLOG_INFO(CONST_MODULE_DC, "[fd : %d] notifying device fault ... num_subscribers = %d\n",
                   fd, num_subscribers);

        if (num_subscribers > 0)
        {
            reply = "{\"returnValue\": true, \"eventType\": \"" +
                    getEventNotificationString(EventType::EVENT_TYPE_DEVICE_FAULT) +
                    "\", \"id\": \"camera" + std::to_string(camera_id_) + "\"}";
            if (!LSSubscriptionReply(sh_, subskey_.c_str(), reply.c_str(), &lserror))
            {
                LSErrorPrint(&lserror, stderr);
                LSErrorFree(&lserror);
                PMLOG_INFO(CONST_MODULE_DC, "[fd : %d] subscription reply failed\n", fd);
                return;
            }
            PMLOG_INFO(CONST_MODULE_DC, "[fd : %d] notified device fault event !!\n", fd);
        }
    }
    LSErrorFree(&lserror);
}

//[Camera Solution Manager] interfaces start
DEVICE_RETURN_CODE_T
DeviceControl::getSupportedCameraSolutionInfo(std::vector<std::string> &solutionsInfo)
{
    if (pCameraSolution != nullptr)
    {
        pCameraSolution->getSupportedSolutionInfo(solutionsInfo);
    }
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T
DeviceControl::getEnabledCameraSolutionInfo(std::vector<std::string> &solutionsInfo)
{
    if (pCameraSolution != nullptr)
    {
        pCameraSolution->getEnabledSolutionInfo(solutionsInfo);
    }
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::enableCameraSolution(const std::vector<std::string> solutions)
{
    PMLOG_INFO(CONST_MODULE_DC, "");

    if (pCameraSolution != nullptr)
    {
        return pCameraSolution->enableCameraSolution(solutions);
    }
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::disableCameraSolution(const std::vector<std::string> solutions)
{
    PMLOG_INFO(CONST_MODULE_DC, "");

    if (pCameraSolution != nullptr)
    {
        return pCameraSolution->disableCameraSolution(solutions);
    }
    return DEVICE_OK;
}
//[Camera Solution Manager] interfaces end
