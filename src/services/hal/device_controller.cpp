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
#define LOG_TAG "DeviceControl"
#include "device_controller.h"
#include "camera_solution_manager.h"
#include <algorithm>
#include <ctime>
#include <errno.h>
#include <json_utils.h>
#include <nlohmann/json.hpp>
#include <pbnjson.h>
#include <poll.h>
#include <signal.h>
#include <sys/time.h>
#include <system_error>

using namespace nlohmann;

/**
 * need to call directly camera base methods in order to cancel preview when the camera is
 * disconnected.
 */

struct MemoryListener : public CameraSolutionEvent
{
    MemoryListener(void) { PLOGI(""); }
    virtual ~MemoryListener(void) { PLOGI(""); }
    virtual void onInitialized(void) override { PLOGI("It's initialized!!"); }
    virtual void onDone(const char *szResult) override
    {
        std::lock_guard<std::mutex> lg(mtxResult_);
        // TODO : We need to composite more than one results.
        //        e.g) {"faces":[{...}, {...}], "poses":[{...}, {...}]}
        json jResult = json::parse(szResult, nullptr, false);
        if (jResult.is_discarded())
        {
            PLOGE("message parsing error!");
            return;
        }

        for (auto &[key, val] : jResult.items())
        {
            if (key == "returnValue")
                continue;
            jsonResult_[key] = val;
        }
        PLOGI("Solution Result: %s", jsonResult_.dump().c_str());
    }
    std::string getResult(void)
    {
        std::lock_guard<std::mutex> lg(mtxResult_);
        if (!jsonResult_.is_null())
        {
            return jsonResult_.dump();
        }
        return "";
    }
    std::mutex mtxResult_;
    json jsonResult_;
};

int DeviceControl::n_imagecount_ = 0;

DeviceControl::DeviceControl()
    : b_iscontinuous_capture_(false), b_isstreamon_(false), b_isposixruning(false),
      b_issystemvruning(false), b_issystemvruning_mmap(false), p_cam_hal(nullptr), shmemfd_(-1),
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
            PLOGE("localtime() given null ptr");
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

    PLOGI("path : %s\n", path.c_str());

    if (NULL == (fp = fopen(path.c_str(), "w")))
    {
        PLOGI("path : fopen failed\n");
        return DEVICE_ERROR_CANNOT_WRITE;
    }
    fwrite((unsigned char *)p, 1, size, fp);
    fclose(fp);
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::checkFormat(CAMERA_FORMAT sformat)
{
    PLOGI("checkFormat for device\n");

    // get current saved format for device
    stream_format_t streamformat;
    auto retval = p_cam_hal->getFormat(&streamformat);
    if (retval != CAMERA_ERROR_NONE)
    {
        PLOGE("getFormat failed");
        return DEVICE_ERROR_UNKNOWN;
    }

    PLOGI("stream_format_t pixel_format : %d \n", streamformat.pixel_format);

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
        PLOGI("Stored format and new format are different\n");
        // stream off, unmap and destroy previous allocated buffers
        // close and again open device to set format again
        int memtype = -1;
        if (str_memtype_ == kMemtypeShmem || str_memtype_ == kMemtypeShmemMmap)
            memtype = SHMEM_SYSTEMV;
        else
            memtype = SHMEM_POSIX;
        stopPreview(memtype);
        close();
        open(strdevicenode_, camera_id_, payload_);

        // set format again
        stream_format_t newstreamformat = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
        newstreamformat.stream_height   = sformat.nHeight;
        newstreamformat.stream_width    = sformat.nWidth;
        newstreamformat.pixel_format    = getPixelFormat(sformat.eFormat);
        // error handling
        if (newstreamformat.pixel_format == CAMERA_PIXEL_FORMAT_MAX)
            return DEVICE_ERROR_UNSUPPORTED_FORMAT;

        retval = p_cam_hal->setFormat(&newstreamformat);
        if (retval != CAMERA_ERROR_NONE)
        {
            PLOGE("setFormat failed");

            // if set format fails then reset format to preview format
            retval = p_cam_hal->setFormat(&streamformat);
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("setFormat with preview format failed");
            }

            // save pixel format for saving captured image
            epixelformat_ = streamformat.pixel_format;
        }
        else
            // save pixel format for saving captured image
            epixelformat_ = newstreamformat.pixel_format;

        // allocate buffers and stream on again
        int key = 0;
        ret     = startPreview(str_memtype_, &key, sh_, subskey_.c_str());
    }

    return ret;
}

DEVICE_RETURN_CODE_T DeviceControl::pollForCapturedImage(int ncount) const
{
    int fd = halFd_;
    int retval;
    struct pollfd poll_set[]
    {
        {.fd = fd, .events = POLLIN},
    };

    int timeout           = 10000;
    buffer_t frame_buffer = {0};

    for (int i = 1; i <= ncount; i++)
    {
        if ((retval = poll(poll_set, 1, timeout)) > 0)
        {
            retval = p_cam_hal->getBuffer(&frame_buffer);
            if (CAMERA_ERROR_NONE != retval)
            {
                PLOGE("getBuffer failed");
                return DEVICE_ERROR_UNKNOWN;
            }
            PLOGI("buffer start : %p \n", frame_buffer.start);
            PLOGI("buffer length : %lu \n", frame_buffer.length);

            if (frame_buffer.start == nullptr)
            {
                PLOGE("no valid memory on frame buffer ptr");
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

            retval = p_cam_hal->releaseBuffer(&frame_buffer);
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("releaseBuffer failed");
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
    PLOGI("started\n");

    pthread_setname_np(pthread_self(), "capture_thread");

    // run capture thread until stopCapture received
    while (b_iscontinuous_capture_)
    {
        auto ret = captureImage(1, informat_, str_imagepath_, cstr_continuous);
        if (ret != DEVICE_OK)
        {
            PLOGE("captureImage failed \n");
            break;
        }
    }
    // set continuous capture to false
    b_iscontinuous_capture_ = false;
    tidCapture.detach();
    n_imagecount_ = 0;
    PLOGI("ended\n");
    return;
}

void DeviceControl::previewThread()
{
    PLOGI("p_cam_hal(%p) start!", p_cam_hal);

    pthread_setname_np(pthread_self(), "preview_thread");

    // poll for data on buffers and save captured image
    // lock so that if stop preview is called, first this cycle should complete
    std::lock_guard<std::mutex> guard(tMutex);

    int debug_counter  = 0;
    int debug_interval = 100; // frames
    auto tic           = std::chrono::steady_clock::now();

    while (b_isstreamon_)
    {
        // keep writing data to shared memory
        unsigned int timestamp = 0;

        buffer_t frame_buffer = {0};

        auto retval = p_cam_hal->getBuffer(&frame_buffer);
        if (retval != CAMERA_ERROR_NONE)
        {
            PLOGE("getBuffer failed");

            notifyDeviceFault_();
            break;
        }

        if (frame_buffer.start == nullptr)
        {
            PLOGE("no valid frame buffer obtained");
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
                PLOGE("Write Shared memory error %d \n", retshmem);
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

        retval = p_cam_hal->releaseBuffer(&frame_buffer);
        if (retval != CAMERA_ERROR_NONE)
        {
            PLOGE("releaseBuffer failed");
            notifyDeviceFault_();
            break;
        }

        if (++debug_counter >= debug_interval)
        {
            auto toc = std::chrono::steady_clock::now();
            auto us  = std::chrono::duration_cast<std::chrono::microseconds>(toc - tic).count();
            PLOGI("previewThread p_cam_hal(%p) : fps(%3.2f), clients(%zu)", p_cam_hal,
                  debug_interval * 1000000.0f / us, client_pool_.size());
            tic           = toc;
            debug_counter = 0;
        }
    }

    PLOGI("p_cam_hal(%p) end!", p_cam_hal);
    return;
}

DEVICE_RETURN_CODE_T DeviceControl::open(std::string devicenode, int ndev_id, std::string payload)
{
    PLOGI("started\n");

    strdevicenode_ = devicenode;
    camera_id_     = ndev_id;
    payload_       = payload;

    auto ret = p_cam_hal->openDevice(devicenode.c_str(), payload.c_str());
    if (ret == CAMERA_ERROR_UNKNOWN)
    {
        PLOGI("open fail!");
        return DEVICE_ERROR_CAN_NOT_OPEN;
    }
    halFd_ = ret;
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::close()
{
    PLOGI("started \n");

    auto ret = p_cam_hal->closeDevice();
    halFd_   = -1;
    if (ret != CAMERA_ERROR_NONE)
    {
        PLOGI("fail");
        return DEVICE_ERROR_CAN_NOT_CLOSE;
    }
    PLOGI("success");
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startPreview(std::string memtype, int *pkey, LSHandle *sh,
                                                 const char *subskey)
{
    PLOGI("started !\n");

    str_memtype_ = memtype;
    sh_          = sh;
    subskey_     = subskey ? subskey : "";

    // get current saved format for device
    stream_format_t streamformat;
    auto retval = p_cam_hal->getFormat(&streamformat);
    if (retval != CAMERA_ERROR_NONE)
    {
        PLOGE("getFormat failed");
        return DEVICE_ERROR_UNKNOWN;
    }

    PLOGI("Driver set width : %d height : %d", streamformat.stream_width,
          streamformat.stream_height);

    buf_size_ = streamformat.buffer_size + extra_buffer;
    PLOGI("buf_size : %d = %d + %d", buf_size_, streamformat.buffer_size, extra_buffer);

    int32_t meta_size = 0;
    if (pCameraSolution != nullptr)
    {
        meta_size = pCameraSolution->getMetaSizeHint();
    }

    if (memtype == kMemtypeShmem || memtype == kMemtypeShmemMmap)
    {
        // frame_count = 8 (see "constants.h")
        auto retshmem = IPCSharedMemory::getInstance().CreateShmemory(
            &h_shmsystem_, pkey, buf_size_, meta_size, frame_count, sizeof(unsigned int));
        if (retshmem != SHMEM_IS_OK)
        {
            PLOGE("CreateShmemory error %d \n", retshmem);
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
            PLOGE("CreatePosixShmemory error %d \n", retshmem);
            return DEVICE_ERROR_UNKNOWN;
        }

        shmemfd_       = *pkey;
        str_shmemname_ = shmname;
    }

    //[Camera Solution Manager] initialization
    if (pCameraSolution != nullptr)
    {
        pCameraSolution->initialize(streamformat, *pkey, sh);
    }

    if (b_isstreamon_ == false)
    {
        if (memtype == kMemtypeShmem)
        {
            // user pointer buffer set-up.
            usrpbufs_ = (buffer_t *)calloc(frame_count, sizeof(buffer_t));
            if (!usrpbufs_)
            {
                PLOGE("USERPTR buffer allocation failed \n");
                SHMEM_STATUS_T status = IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
                PLOGI("CloseShmemory %d", status);
                return DEVICE_ERROR_UNKNOWN;
            }
            IPCSharedMemory::getInstance().GetShmemoryBufferInfo(h_shmsystem_, frame_count,
                                                                 usrpbufs_, nullptr);

            // frame_count = 8 (see "constants.h")
            auto retval = p_cam_hal->setBuffer(frame_count, IOMODE_USERPTR, (void **)&usrpbufs_);
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("setBuffer failed");
                free(usrpbufs_);
                usrpbufs_             = nullptr;
                SHMEM_STATUS_T status = IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
                PLOGI("CloseShmemory %d", status);
                return DEVICE_ERROR_UNKNOWN;
            }

            retval = p_cam_hal->startCapture();
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("startCapture failed");
                free(usrpbufs_);
                usrpbufs_             = nullptr;
                SHMEM_STATUS_T status = IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
                PLOGI("CloseShmemory %d", status);
                return DEVICE_ERROR_UNKNOWN;
            }

            b_isstreamon_     = true;
            b_issystemvruning = true;
        }
        else if (memtype == kMemtypeShmemMmap)
        {
            auto retval = p_cam_hal->setBuffer(4, IOMODE_MMAP, nullptr);
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("setBuffer failed");
                return DEVICE_ERROR_UNKNOWN;
            }

            retval = p_cam_hal->startCapture();
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("startCapture failed");
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
                PLOGE("USERPTR buffer allocation failed \n");
                IPCPosixSharedMemory::getInstance().CloseShmemory(&h_shmposix_, frame_count,
                                                                  buf_size_, sizeof(unsigned int),
                                                                  str_shmemname_, shmemfd_);
                return DEVICE_ERROR_UNKNOWN;
            }
            IPCPosixSharedMemory::getInstance().GetShmemoryBufferInfo(h_shmposix_, frame_count,
                                                                      usrpbufs_, nullptr);

            // frame_count = 8 (see "constants.h")
            auto retval = p_cam_hal->setBuffer(frame_count, IOMODE_USERPTR, (void **)&usrpbufs_);
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("setBuffer failed");
                free(usrpbufs_);
                usrpbufs_ = nullptr;
                IPCPosixSharedMemory::getInstance().CloseShmemory(&h_shmposix_, frame_count,
                                                                  buf_size_, sizeof(unsigned int),
                                                                  str_shmemname_, shmemfd_);
                return DEVICE_ERROR_UNKNOWN;
            }

            retval = p_cam_hal->startCapture();
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("startCapture failed");
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
        PLOGI("make previewThread");
        tidPreview = std::thread{[this]() { this->previewThread(); }};
    }

    PLOGI("end !");
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::stopPreview(int memtype)
{
    PLOGI("started !\n");

    //[]Camera Solution Manager] release
    if (pCameraSolution != nullptr)
    {
        pCameraSolution->release();
    }

    b_isstreamon_ = false;

    if (tidPreview.joinable())
    {
        PLOGI("Thread Closing");
        try
        {
            tidPreview.join();
        }
        catch (const std::system_error &e)
        {
            PLOGE("Caught a system_error with code %d meaning %s", e.code().value(), e.what());
        }
        PLOGI("Thread Closed");
    }

    if (cancel_preview_ == true)
    {
        p_cam_hal->stopCapture();
        p_cam_hal->destroyBuffer();
    }
    else
    {
        auto retval = p_cam_hal->stopCapture();
        if (retval != CAMERA_ERROR_NONE)
        {
            PLOGE("stopCapture failed");
            return DEVICE_ERROR_UNKNOWN;
        }

        retval = p_cam_hal->destroyBuffer();
        if (retval != CAMERA_ERROR_NONE)
        {
            PLOGE("destroyBuffer failed");
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
                PLOGE("CloseShmemory error %d \n", retshmem);
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
                PLOGE("ClosePosixShmemory error %d \n", retshmem);
            }
            h_shmposix_ = nullptr;
        }
    }

    if (usrpbufs_ != nullptr)
    {
        free(usrpbufs_);
        usrpbufs_ = nullptr;
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::startCapture(CAMERA_FORMAT sformat,
                                                 const std::string &imagepath)
{
    PLOGI("started !\n");

    informat_.nHeight       = sformat.nHeight;
    informat_.nWidth        = sformat.nWidth;
    informat_.eFormat       = sformat.eFormat;
    b_iscontinuous_capture_ = true;
    str_imagepath_          = imagepath;

    // create thread that will continuously capture images until stopcapture received
    tidCapture = std::thread{[this]() { this->captureThread(); }};

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::stopCapture()
{
    PLOGI("started !\n");

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

DEVICE_RETURN_CODE_T DeviceControl::captureImage(int ncount, CAMERA_FORMAT sformat,
                                                 const std::string &imagepath,
                                                 const std::string &mode)
{
    PLOGI("started ncount : %d \n", ncount);

    // update image locstion if there is a change
    if (str_imagepath_ != imagepath)
        str_imagepath_ = imagepath;

    if (str_capturemode_ != mode)
        str_capturemode_ = mode;

    // validate if saved format and capture image format are same or not
    if (DEVICE_OK != checkFormat(sformat))
    {
        PLOGE("checkFormat failed \n");
        return DEVICE_ERROR_UNKNOWN;
    }

    // poll for data on buffers and save captured image
    auto retval = pollForCapturedImage(ncount);
    if (retval != DEVICE_OK)
    {
        PLOGE("pollForCapturedImage failed \n");
        return retval;
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::createHal(std::string deviceType)
{
    PLOGI("started \n");

    pFeature_ = pluginFactory_.createFeature(deviceType.c_str());
    if (pFeature_ == nullptr)
        return DEVICE_ERROR_UNKNOWN;

    void *pInterface = nullptr;
    if (pFeature_->queryInterface(deviceType.c_str(), &pInterface))
    {
        p_cam_hal = static_cast<IHal *>(pInterface);
        return DEVICE_OK;
    }
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T DeviceControl::destroyHal()
{
    PLOGI("started \n");
    // p_cam_hal is automatically dlclosed in the destructor of Ifeatureptr.
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getDeviceInfo(std::string strdevicenode, std::string deviceType,
                                                  camera_device_info_t *pinfo)
{
    PLOGI("started strdevicenode : %s, deviceType : %s", strdevicenode.c_str(), deviceType.c_str());

    PluginFactory factory;
    auto pFeature    = factory.createFeature(deviceType.c_str());
    void *pInterface = nullptr;
    if (pFeature && pFeature->queryInterface(deviceType.c_str(), &pInterface))
    {
        static_cast<IHal *>(pInterface)->getInfo(pinfo, strdevicenode);
        return DEVICE_OK;
    }
    return DEVICE_ERROR_UNKNOWN;
}

DEVICE_RETURN_CODE_T DeviceControl::getDeviceProperty(CAMERA_PROPERTIES_T *oparams)
{
    PLOGI("started !\n");

    camera_properties_t out_params;

    for (int i = 0; i < PROPERTY_END; i++)
    {
        out_params.stGetData.data[i][QUERY_VALUE] = CONST_PARAM_DEFAULT_VALUE;
    }

    auto retval = p_cam_hal->getProperties(&out_params);
    if (retval != CAMERA_ERROR_NONE)
    {
        PLOGE("getProperties failed");
        return DEVICE_ERROR_UNKNOWN;
    }

    // update stGetData
    for (int i = 0; i < PROPERTY_END; i++)
    {
        for (int j = 0; j < QUERY_END; j++)
        {
            oparams->stGetData.data[i][j] = out_params.stGetData.data[i][j];
            PLOGD("out_params.stGetData[%d][%d]:%d\n", i, j, out_params.stGetData.data[i][j]);
        }
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setDeviceProperty(CAMERA_PROPERTIES_T *inparams)
{
    PLOGI("started!\n");

    camera_properties_t in_params;

    for (int i = 0; i < PROPERTY_END; i++)
    {
        in_params.stGetData.data[i][QUERY_VALUE] = inparams->stGetData.data[i][QUERY_VALUE];
    }

    auto retval = p_cam_hal->setProperties(&in_params);
    if (retval != CAMERA_ERROR_NONE)
    {
        PLOGE("setProperties failed");
        return DEVICE_ERROR_UNKNOWN;
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::setFormat(CAMERA_FORMAT sformat)
{
    PLOGI("sFormat Height %d Width %d Format %d Fps : %d\n", sformat.nHeight, sformat.nWidth,
          sformat.eFormat, sformat.nFps);

    stream_format_t in_format = {CAMERA_PIXEL_FORMAT_MAX, 0, 0, 0, 0};
    in_format.stream_height   = sformat.nHeight;
    in_format.stream_width    = sformat.nWidth;
    in_format.stream_fps      = sformat.nFps;
    in_format.pixel_format    = getPixelFormat(sformat.eFormat);
    // error handling
    if (in_format.pixel_format == CAMERA_PIXEL_FORMAT_MAX)
        return DEVICE_ERROR_UNSUPPORTED_FORMAT;

    auto ret = p_cam_hal->setFormat(&in_format);
    if (ret != CAMERA_ERROR_NONE)
    {
        PLOGE("setFormat failed");
        return DEVICE_ERROR_UNSUPPORTED_FORMAT;
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getFormat(CAMERA_FORMAT *pformat)
{
    // get current saved format for device
    stream_format_t streamformat;
    auto ret = p_cam_hal->getFormat(&streamformat);
    if (ret != CAMERA_ERROR_NONE)
    {
        PLOGE("getFormat failed");
        return DEVICE_ERROR_UNKNOWN;
    }
    pformat->nHeight = streamformat.stream_height;
    pformat->nWidth  = streamformat.stream_width;
    pformat->eFormat = getCameraFormat(streamformat.pixel_format);
    pformat->nFps    = streamformat.stream_fps;
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getFd(int *posix_shm_fd)
{
    if (h_shmposix_ == nullptr)
    {
        PLOGE("POSIX Shmemory is not used for this session");
        *posix_shm_fd = -1;
        return DEVICE_ERROR_UNKNOWN;
    }
    *posix_shm_fd = shmemfd_;
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

DEVICE_RETURN_CODE_T DeviceControl::registerClient(pid_t pid, int sig, int devhandle,
                                                   std::string &outmsg)
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
            return DEVICE_OK;
        }
        else
        {
            outmsg =
                "The client of pid " + std::to_string(pid) + " is already registered :: ignored";
            PLOGI("%s", outmsg.c_str());
            return DEVICE_ERROR_UNKNOWN;
        }
    }
}

DEVICE_RETURN_CODE_T DeviceControl::unregisterClient(pid_t pid, std::string &outmsg)
{
    std::lock_guard<std::mutex> mlock(client_pool_mutex_);
    {
        auto it = std::find_if(client_pool_.begin(), client_pool_.end(),
                               [=](const CLIENT_INFO_T &p) { return p.pid == pid; });

        if (it != client_pool_.end())
        {
            client_pool_.erase(it);
            outmsg = "The client of pid " + std::to_string(pid) + " unregistered :: OK";
            PLOGI("%s", outmsg.c_str());
            return DEVICE_OK;
        }
        else
        {
            outmsg = "No client of pid " + std::to_string(pid) + " exists :: ignored";
            PLOGI("%s", outmsg.c_str());
            return DEVICE_ERROR_UNKNOWN;
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
        PLOGD("Broadcasting to %zu clients\n", client_pool_.size());

        auto it = client_pool_.begin();
        while (it != client_pool_.end())
        {

            PLOGD("About to send a signal %d to the client of pid %d ...\n", it->sig, it->pid);
            int errid = kill(it->pid, it->sig);
            if (errid == -1)
            {
                switch (errno)
                {
                case ESRCH:
                    PLOGE("The client of pid %d does not exist and will be removed from the pool!!",
                          it->pid);
                    // remove this pid from the client pool to make ensure no zombie process exists.
                    it = client_pool_.erase(it);
                    break;
                case EINVAL:
                    PLOGE("The client of pid %d was given an invalid signal "
                          "%d and will be be removed from the pool!!",
                          it->pid, it->sig);
                    it = client_pool_.erase(it);
                    break;
                default: // case errno = EPERM
                    PLOGE("Unexpected error in sending the signal to the client of pid %d\n",
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
        int fd          = halFd_;

        PLOGI("[fd : %d] notifying device fault ... num_subscribers = %d\n", fd, num_subscribers);

        if (num_subscribers > 0)
        {
            reply = "{\"returnValue\": true, \"eventType\": \"" +
                    getEventNotificationString(EventType::EVENT_TYPE_PREVIEW_FAULT) +
                    "\", \"id\": \"camera" + std::to_string(camera_id_) + "\"}";
            if (!LSSubscriptionReply(sh_, subskey_.c_str(), reply.c_str(), &lserror))
            {
                LSErrorPrint(&lserror, stderr);
                LSErrorFree(&lserror);
                PLOGI("[fd : %d] subscription reply failed\n", fd);
                return;
            }
            PLOGI("[fd : %d] notified device preview event !!\n", fd);
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
    PLOGI("");

    if (pCameraSolution != nullptr)
    {
        return pCameraSolution->enableCameraSolution(solutions);
    }
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::disableCameraSolution(const std::vector<std::string> solutions)
{
    PLOGI("");

    if (pCameraSolution != nullptr)
    {
        return pCameraSolution->disableCameraSolution(solutions);
    }
    return DEVICE_OK;
}
//[Camera Solution Manager] interfaces end
