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
#include <filesystem>
#include <json_utils.h>
#include <nlohmann/json.hpp>
#include <pbnjson.h>
#include <signal.h>
#include <sys/time.h>
#include <system_error>

#define FRAME_COUNT 8

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

DeviceControl::DeviceControl()
    : b_iscontinuous_capture_(false), b_isstreamon_(false), b_isposixruning(false),
      b_issystemvruning(false), b_issystemvruning_mmap(false), p_cam_hal(nullptr), shmemfd_(-1),
      usrpbufs_(nullptr), capture_format_(), tMutex(), tCondVar(), h_shmsystem_(nullptr),
      h_shmposix_(nullptr), str_imagepath_(cstr_empty), str_capturemode_(cstr_oneshot),
      str_memtype_(""), str_shmemname_(""), cancel_preview_(false), buf_size_(0), sh_(nullptr),
      subskey_(""), camera_id_(-1)
{
    pCameraSolution = std::make_shared<CameraSolutionManager>();
    pMemoryListener = std::make_shared<MemoryListener>();
    if (pCameraSolution != nullptr && pMemoryListener != nullptr)
    {
        pCameraSolution->setEventListener(pMemoryListener.get());
    }
}

DEVICE_RETURN_CODE_T DeviceControl::writeImageToFile(const void *p, int size, int cnt) const
{
    auto path = str_imagepath_;

    if (path.empty())
        path = "/tmp/";

    // find the file extension to check if file name is provided or path is provided
    std::string extension;
    auto position = path.find_last_of(".");
    if (position != path.npos)
    {
        if (position + 1 < SIZE_MAX)
        {
            extension = path.substr(position + 1);
        }
    }

    if ((extension == "yuv") || (extension == "jpeg") || (extension == "h264"))
    {
        if (cstr_burst == str_capturemode_ || cstr_continuous == str_capturemode_)
        {
            path.insert(position, std::to_string(cnt));
        }
    }
    else
    {
        // check if specified location ends with '/' else add
        char ch = path.back();
        if (ch != '/')
            path += "/";

        time_t t = time(NULL);
        if (t == ((time_t)-1))
        {
            PLOGE("Failed to get current time.");
            t = 0;
        }
        tm *timePtr = localtime(&t);
        if (timePtr == nullptr)
        {
            PLOGE("localtime() given null ptr");
            return DEVICE_ERROR_UNKNOWN;
        }
        struct timeval tmnow;
        gettimeofday(&tmnow, NULL);

        // create file to save data based on format
        char image_name[100] = {};
        int result =
            snprintf(image_name, 100, "Picture%02d%02d%02d-%02d%02d%02d%02jd", timePtr->tm_mday,
                     (timePtr->tm_mon) + 1, (timePtr->tm_year) + 1900, (timePtr->tm_hour),
                     (timePtr->tm_min), (timePtr->tm_sec), (intmax_t)(tmnow.tv_usec / 10000));

        if (result >= 0 && result < 100)
        {
            path += image_name;
        }
        else
        {
            PLOGE("snprintf encountered an error or the formatted string was truncated.");
            return DEVICE_ERROR_UNKNOWN;
        }

        if (cstr_burst == str_capturemode_)
        {
            path += '_' + std::to_string(cnt) + '.';
        }

        std::string ext;
        if (capture_format_.eFormat == CAMERA_FORMAT_YUV)
            path += "yuv";
        else if (capture_format_.eFormat == CAMERA_FORMAT_JPEG)
            path += "jpeg";
        else if (capture_format_.eFormat == CAMERA_FORMAT_H264ES)
            path += "h264";
    }

    PLOGD("path : %s\n", path.c_str());

    FILE *fp;
    if (NULL == (fp = fopen(path.c_str(), "w")))
    {
        PLOGE("path : fopen failed\n");
        return DEVICE_ERROR_CANNOT_WRITE;
    }

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    size_t sz                = (size > 0) ? (size_t)size : 0;
    size_t bytes_written     = fwrite(p, 1, sz, fp);
    if (bytes_written != sz)
    {
        PLOGE("Error writing data to file.");
        ret = DEVICE_ERROR_FAIL_TO_WRITE_FILE;
    }

    if (fclose(fp) != 0)
    {
        PLOGE("fclose error");
    }
    return ret;
}

DEVICE_RETURN_CODE_T DeviceControl::saveShmemory(int ncount) const
{
    SHMEM_HANDLE h_shm = b_isposixruning ? h_shmposix_ : h_shmsystem_;

    if (h_shm == nullptr)
    {
        PLOGE("shared memory handle is null");
        return DEVICE_ERROR_UNKNOWN;
    }

    buffer_t frame_buffer = {0};
    int read_index        = -1;
    int write_index       = -1;
    int nCaptured         = 0;

    DEVICE_RETURN_CODE_T ret;

    const unsigned int sleep_us       = 10000; // 10ms
    const unsigned int max_iterations = 1000;

    while ((nCaptured++ < ncount) || b_iscontinuous_capture_)
    {
        unsigned int cnt = 0;

        while (cnt < max_iterations) // 10s
        {
            write_index = b_isposixruning ? IPCPosixSharedMemory::getInstance().GetWriteIndex(h_shm)
                                          : IPCSharedMemory::getInstance().GetWriteIndex(h_shm);
            if (read_index != write_index)
            {
                break;
            }
            usleep(sleep_us);
            cnt++;
        }
        if (read_index == write_index)
        {
            PLOGE("same write_index=%d", write_index);
        }
        read_index = write_index;

        int len                    = 0;
        unsigned char *sh_mem_addr = NULL;
        if (b_isposixruning)
        {
            IPCPosixSharedMemory::getInstance().ReadShmemory(h_shm, &sh_mem_addr, &len);
        }
        else
        {
            IPCSharedMemory::getInstance().ReadShmem(h_shm, &sh_mem_addr, &len);
        }

        frame_buffer.start  = sh_mem_addr;
        frame_buffer.length = (len > 0) ? len : 0;

        PLOGD("buffer start : %p \n", frame_buffer.start);
        PLOGD("buffer length : %lu \n", frame_buffer.length);

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
        ret = writeImageToFile(frame_buffer.start, frame_buffer.length, nCaptured);
        if (ret != DEVICE_OK)
        {
            PLOGE("file write error");
            return ret;
        }
    }

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

    b_iscontinuous_capture_ = true;

    saveShmemory();

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

            notifyDeviceFault_(EventType::EVENT_TYPE_PREVIEW_FAULT);
            break;
        }

        if (frame_buffer.start == nullptr)
        {
            PLOGE("no valid frame buffer obtained");
            notifyDeviceFault_(EventType::EVENT_TYPE_PREVIEW_FAULT);
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

            IPCSharedMemory::getInstance().WriteMeta(h_shmsystem_, meta.c_str(), meta.size() + 1);

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
                meta.c_str(), meta.size() + 1, (unsigned char *)&timestamp, sizeof(timestamp));

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

            IPCPosixSharedMemory::getInstance().WriteMeta(h_shmposix_, meta.c_str(),
                                                          meta.size() + 1);

            // Time stamp is currently not used actually.
            IPCPosixSharedMemory::getInstance().WriteExtra(h_shmposix_, (unsigned char *)&timestamp,
                                                           sizeof(timestamp));

            IPCPosixSharedMemory::getInstance().IncrementWriteIndex(h_shmposix_);

            broadcast_();
        }

        retval = p_cam_hal->releaseBuffer(&frame_buffer);
        if (retval != CAMERA_ERROR_NONE)
        {
            PLOGE("releaseBuffer failed");
            notifyDeviceFault_(EventType::EVENT_TYPE_PREVIEW_FAULT);
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

    if (streamformat.buffer_size < INT_MAX - extra_buffer)
    {
        buf_size_ = streamformat.buffer_size + extra_buffer;
    }
    PLOGI("buf_size : %d = %d + %d", buf_size_, streamformat.buffer_size, extra_buffer);

    int32_t meta_size = 0;
    if (pCameraSolution != nullptr)
    {
        meta_size = pCameraSolution->getMetaSizeHint();
    }

    if (memtype == kMemtypeShmem || memtype == kMemtypeShmemMmap)
    {
        auto retshmem = IPCSharedMemory::getInstance().CreateShmemory(
            &h_shmsystem_, pkey, buf_size_, meta_size, FRAME_COUNT, sizeof(unsigned int));
        if (retshmem != SHMEM_IS_OK)
        {
            PLOGE("CreateShmemory error %d \n", retshmem);
            return DEVICE_ERROR_UNKNOWN;
        }
    }
    else // memtype == kMemtypePosixshm
    {
        std::string shmname = "";

        auto retshmem = IPCPosixSharedMemory::getInstance().CreateShmemory(
            &h_shmposix_, buf_size_, meta_size, FRAME_COUNT, sizeof(unsigned int), pkey, &shmname);
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
            usrpbufs_ = (buffer_t *)calloc(FRAME_COUNT, sizeof(buffer_t));
            if (!usrpbufs_)
            {
                PLOGE("USERPTR buffer allocation failed \n");
                SHMEM_STATUS_T status = IPCSharedMemory::getInstance().CloseShmemory(&h_shmsystem_);
                PLOGI("CloseShmemory %d", status);
                return DEVICE_ERROR_UNKNOWN;
            }
            IPCSharedMemory::getInstance().GetShmemoryBufferInfo(h_shmsystem_, FRAME_COUNT,
                                                                 usrpbufs_, nullptr);

            auto retval = p_cam_hal->setBuffer(FRAME_COUNT, IOMODE_USERPTR, (void **)&usrpbufs_);
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
            usrpbufs_ = (buffer_t *)calloc(FRAME_COUNT, sizeof(buffer_t));
            if (!usrpbufs_)
            {
                PLOGE("USERPTR buffer allocation failed \n");
                IPCPosixSharedMemory::getInstance().CloseShmemory(
                    &h_shmposix_, FRAME_COUNT, buf_size_, meta_size, sizeof(unsigned int),
                    str_shmemname_, shmemfd_);
                return DEVICE_ERROR_UNKNOWN;
            }
            IPCPosixSharedMemory::getInstance().GetShmemoryBufferInfo(h_shmposix_, FRAME_COUNT,
                                                                      usrpbufs_, nullptr);

            auto retval = p_cam_hal->setBuffer(FRAME_COUNT, IOMODE_USERPTR, (void **)&usrpbufs_);
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("setBuffer failed");
                free(usrpbufs_);
                usrpbufs_ = nullptr;
                IPCPosixSharedMemory::getInstance().CloseShmemory(
                    &h_shmposix_, FRAME_COUNT, buf_size_, meta_size, sizeof(unsigned int),
                    str_shmemname_, shmemfd_);
                return DEVICE_ERROR_UNKNOWN;
            }

            retval = p_cam_hal->startCapture();
            if (retval != CAMERA_ERROR_NONE)
            {
                PLOGE("startCapture failed");
                free(usrpbufs_);
                usrpbufs_ = nullptr;
                IPCPosixSharedMemory::getInstance().CloseShmemory(
                    &h_shmposix_, FRAME_COUNT, buf_size_, meta_size, sizeof(unsigned int),
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
        int32_t meta_size = 0;
        if (pCameraSolution != nullptr)
        {
            meta_size = pCameraSolution->getMetaSizeHint();
        }
        b_isposixruning = false;
        if (h_shmposix_ != nullptr)
        {
            auto retshmem = IPCPosixSharedMemory::getInstance().CloseShmemory(
                &h_shmposix_, FRAME_COUNT, buf_size_, meta_size, sizeof(unsigned int),
                str_shmemname_, shmemfd_);
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

static bool storageMonitorCb(const DEVICE_RETURN_CODE_T errorCode, void *ptr)
{
    DeviceControl *device = (DeviceControl *)ptr;
    if (device)
        device->notifyStorageError(errorCode);

    return true;
}

bool DeviceControl::notifyStorageError(const DEVICE_RETURN_CODE_T ret)
{
    if (b_iscontinuous_capture_)
    {
        PLOGE("Storage reaches the threshold limit. error code %d", (int)ret);
        if (ret != DEVICE_OK)
            notifyDeviceFault_(EventType::EVENT_TYPE_CAPTURE_FAULT, ret);

        stopCapture();
    }

    return true;
}

DEVICE_RETURN_CODE_T DeviceControl::startCapture(CAMERA_FORMAT sformat,
                                                 const std::string &imagepath,
                                                 const std::string &mode, int ncount)
{
    PLOGI("mode = %s, ncount = %d", mode.c_str(), ncount);

    str_imagepath_   = imagepath;
    str_capturemode_ = mode;
    getFormat(&capture_format_);

    if (str_imagepath_.empty())
        str_imagepath_ = "/tmp/";

    std::filesystem::path directory = str_imagepath_;
    if (!std::filesystem::is_directory(directory))
        return DEVICE_ERROR_CANNOT_WRITE;

    if (!StorageMonitor::isEnoughSpaceAvailable(str_imagepath_))
    {
        return DEVICE_ERROR_FAIL_TO_WRITE_FILE;
    }

    storageMonitor_.setPath(str_imagepath_);
    storageMonitor_.registerCallback(storageMonitorCb, this);

    storageMonitor_.startMonitor();

    if (str_capturemode_ == cstr_burst && ncount > MAX_NO_OF_IMAGES_IN_BURST_MODE)
        ncount = MAX_NO_OF_IMAGES_IN_BURST_MODE;

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    if (str_capturemode_ == cstr_continuous)
    {
        // create thread that will continuously capture images until stopcapture received
        tidCapture = std::thread{[this]() { this->captureThread(); }};
    }
    else
    {
        //[TODO] Move to captureThread to prevent LSCall timeout in burst mode.
        ret = saveShmemory(ncount);

        storageMonitor_.stopMonitor();
    }
    return ret;
}

DEVICE_RETURN_CODE_T DeviceControl::stopCapture()
{
    PLOGI("started !\n");

    // if capture thread is running, stop capture
    if (b_iscontinuous_capture_)
    {
        b_iscontinuous_capture_ = false;
        storageMonitor_.stopMonitor();
        tidCapture.join();
    }
    else
        return DEVICE_ERROR_DEVICE_IS_ALREADY_STOPPED;

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

void DeviceControl::notifyDeviceFault_(EventType eventType, DEVICE_RETURN_CODE_T error)
{
    unsigned int num_subscribers = 0;
    std::string reply;
    LSError lserror;
    LSErrorInit(&lserror);

    if (subskey_ != "")
    {
        num_subscribers = LSSubscriptionGetHandleSubscribersCount(sh_, subskey_.c_str());
        int fd          = halFd_;
        auto event_name = getEventNotificationString(eventType);

        PLOGI("[fd : %d] notifying %s... num_subscribers = %u\n", fd, event_name.c_str(),
              num_subscribers);

        if (num_subscribers > 0)
        {
            reply = "{\"returnValue\": true, \"eventType\": \"" + event_name +
                    "\", \"id\": \"camera" + std::to_string(camera_id_) + "\"";
            if (error != DEVICE_OK)
            {
                reply += ", \"errorCode\": " + std::to_string((int)error);
                reply += ", \"errorText\": \"" + getErrorString(error) + "\"";
            }
            reply += "}";
            if (!LSSubscriptionReply(sh_, subskey_.c_str(), reply.c_str(), &lserror))
            {
                LSErrorPrint(&lserror, stderr);
                LSErrorFree(&lserror);
                PLOGI("[fd : %d] subscription reply failed\n", fd);
                return;
            }
            PLOGI("[fd : %d] notified %s !!\n", fd, event_name.c_str());
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
