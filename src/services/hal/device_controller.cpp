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
#include <chrono>
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
    : b_iscontinuous_capture_(false), b_isstreamon_(false), p_cam_hal(nullptr), capture_format_(),
      tMutex(), str_imagepath_(cstr_empty), str_capturemode_(cstr_oneshot), sh_(nullptr),
      subskey_(""), camera_id_(-1), shmDataBuffers(nullptr)
{
    pCameraSolution = std::make_shared<CameraSolutionManager>();
    pMemoryListener = std::make_shared<MemoryListener>();
    if (pCameraSolution != nullptr && pMemoryListener != nullptr)
    {
        pCameraSolution->setEventListener(pMemoryListener.get());
    }
}

/* If necessary, use this according to the layout below */
/* Currently, don't use this */
/* meta json
{
    "video":
    {
        "timestamp": 123,
        "orientation": 270,
    },
    "extra":
    {
        "buffer" : {
            "colorspace" : 123123,
            "width" : 1280,
            "height" : 720,
            "planes" : 1,
            "stride" : 1280,
            "size" : 123123,
            "timestamp" : 123123
        }
    }
}
*/
bool DeviceControl::updateMetaBuffer(const buffer_t &buffer, const json &videoMeta,
                                     const json &extraMeta)
{
    PLOGD("start(%p), length(%ld)", buffer.start, buffer.length);

    json jmeta     = json::object();
    jmeta["video"] = videoMeta;
    jmeta["extra"] = extraMeta;

    std::string strMeta = jmeta.dump();
    PLOGD("meta string : %s", strMeta.c_str());

    if (strMeta.size() > 4096 - 1)
    {
        PLOGE("meta size is larger than buffer size");
        return false;
    }

    memcpy((char *)buffer.start, strMeta.c_str(), strMeta.size() + 1);
    return true;
}

/* If necessary, use this according to the layout below */
/* Currently, don't use this */
/* Currnetly use it : Solution Result:
 * {"faces":[{"confidence":96,"h":287,"w":191,"x":141,"y":97}],"subscribed":true}
 */
/*
{
    "solutions":[
        {
            "name":"Solution1",
            "data": {},
            "binary": {
                "offset":123,
                "size":100
            },
            "timestamp":123
        },
        {
            "name":"Solution2",
            "data": {},
            "timestamp" : 123
        }
    ]
}
*/
bool DeviceControl::updateSolutionBuffer(const buffer_t &buffer)
{
    // std::lock_guard<std::mutex> lg(mtxResult_);

    PLOGD("start(%p), length(%ld), txtSize(%d), binSize(%d)", buffer.start, buffer.length,
          solutionTextSize_, solutionBinarySize_);

    if (!buffer.start || !buffer.length)
    {
        PLOGE("buffer error! buffer.start(%p), buffer.length(%ld)", buffer.start, buffer.length);
        return false;
    }

    // int cur_offset  = solutionTextSize_;
    // int buffer_size = solutionTextSize_ + solutionBinarySize_;
    // json jresult    = json::object();
    // json jsolutions = json::array();
    // for (const auto &[key, solution] : result_map_)
    // {
    //     json cur_json = solution.jResult_;
    //     if (solution.nResult_ && cur_offset + solution.nResult_ <= buffer_size)
    //     {
    //         json jbinary      = json::object();
    //         jbinary["offset"] = cur_offset;
    //         jbinary["size"]   = solution.nResult_;

    //         memcpy((char *)buffer.start + cur_offset, solution.pbResult_, solution.nResult_);
    //         PLOGI("dest %p, src %p, nResult %d", (char *)buffer.start + cur_offset,
    //               solution.pbResult_, solution.nResult_);
    //         cur_offset += solution.nResult_;

    //         cur_json["binary"] = jbinary;
    //         PLOGI("binary = %s", jbinary.dump().c_str());
    //     }

    //     PLOGD("<timestamp> %u, solution %s",
    //           get_optional<unsigned int>(cur_json, "timestamp").value_or(0),
    //           (get_optional<std::string>(cur_json, "name").value_or("")).c_str());

    //     jsolutions.push_back(cur_json);
    // }
    // jresult["solutions"]  = jsolutions;
    // std::string strResult = jresult.dump();
    // PLOGD("strResult %s", strResult.c_str());

    // if (strResult.size() > static_cast<size_t>(solutionTextSize_ - 1))
    // {
    //     PLOGE("solution text size is larger than solutionTextSize_");
    //     return false;
    // }

    auto meta = pMemoryListener->getResult();

    memcpy((char *)buffer.start, meta.c_str(), meta.size() + 1);
    return true;
}

// deprecated
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

        std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
        std::time_t t                             = std::chrono::system_clock::to_time_t(now);
        if (t == static_cast<std::time_t>(-1))
        {
            PLOGE("Failed to get current time.");
            t = 0;
        }
        std::tm *timePtr = std::localtime(&t);
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
            path += '_' + std::to_string(cnt);
        }

        std::string ext;
        if (capture_format_.eFormat == CAMERA_FORMAT_YUV)
            path += ".yuv";
        else if (capture_format_.eFormat == CAMERA_FORMAT_JPEG)
            path += ".jpeg";
        else if (capture_format_.eFormat == CAMERA_FORMAT_H264ES)
            path += ".h264";
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

// deprecated
DEVICE_RETURN_CODE_T DeviceControl::saveShmemory(int ncount) const
{
    if (!shmem_)
    {
        PLOGE("Shared memory does not exist");
        return DEVICE_ERROR_UNKNOWN;
    }

    buffer_t frame_buffer = {0};
    int read_index        = -1;
    int write_index       = -1;
    int nCaptured         = 0;

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    const unsigned int sleep_us       = 10000; // 10ms
    const unsigned int max_iterations = 1000;

    while ((nCaptured++ < ncount) || b_iscontinuous_capture_)
    {
        unsigned int cnt = 0;

        while (cnt < max_iterations) // 10s
        {
            write_index = shmem_->getWriteIndex();

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
        read_index = (write_index - 1 + FRAME_COUNT) % FRAME_COUNT;

        frame_buffer.start  = shmDataBuffers[read_index].start;
        frame_buffer.length = shmDataBuffers[read_index].length;

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

DEVICE_RETURN_CODE_T DeviceControl::writeImageToFile(const void *p, unsigned long size, int cnt,
                                                     std::vector<std::string> &capturedFiles) const
{
    auto capturePath = createCaptureFileName(cnt);

    FILE *fp;
    if (NULL == (fp = fopen(capturePath.c_str(), "w")))
    {
        PLOGE("capturePath : fopen failed");
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

    capturedFiles.push_back(capturePath);

    if (fclose(fp) != 0)
    {
        PLOGE("fclose error");
    }
    return ret;
}

DEVICE_RETURN_CODE_T DeviceControl::saveShmemory(int ncount,
                                                 std::vector<std::string> &capturedFiles) const
{
    if (!shmem_)
    {
        PLOGE("Shared memory does not exist");
        return DEVICE_ERROR_UNKNOWN;
    }

    buffer_t frame_buffer = {0};
    int read_index        = -1;
    int write_index       = -1;
    int nCaptured         = 0;

    DEVICE_RETURN_CODE_T ret = DEVICE_OK;

    const unsigned int sleep_us       = 10000; // 10ms
    const unsigned int max_iterations = 1000;

    while ((nCaptured++ < ncount) || b_iscontinuous_capture_)
    {
        unsigned int cnt = 0;

        while (cnt < max_iterations) // 10s
        {
            write_index = shmem_->getWriteIndex();

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
        read_index = (write_index - 1 + FRAME_COUNT) % FRAME_COUNT;

        frame_buffer.start  = shmDataBuffers[read_index].start;
        frame_buffer.length = shmDataBuffers[read_index].length;

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
        ret = writeImageToFile(frame_buffer.start, frame_buffer.length, nCaptured, capturedFiles);
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
        buffer_t buffer = {0};

        auto retval = p_cam_hal->getBuffer(&buffer);
        if (retval != CAMERA_ERROR_NONE)
        {
            PLOGE("getBuffer failed");
            notifyDeviceFault_(EventType::EVENT_TYPE_PREVIEW_FAULT);
            break;
        }

        if (buffer.start == nullptr)
        {
            PLOGE("no valid frame buffer obtained");
            notifyDeviceFault_(EventType::EVENT_TYPE_PREVIEW_FAULT);
            break;
        }

        //[Camera Solution Manager] process for preview
        if (pCameraSolution != nullptr)
        {
            pCameraSolution->processPreview(buffer);
        }

        if (!shmem_)
        {
            PLOGE("Shared memory does not exist");
            notifyDeviceFault_(EventType::EVENT_TYPE_PREVIEW_FAULT);
            break;
        }

        PLOGD("buffer: start(%p) index(%zu) length(%lu)", buffer.start, buffer.index,
              buffer.length);

        // shared memory index
        int shm_index = buffer.index;

        shmDataBuffers[buffer.index].start  = buffer.start;
        shmDataBuffers[buffer.index].length = buffer.length;

        // Create meta_data
        updateMetaBuffer(shmMetaBuffers_[shm_index], nullptr, nullptr);
        updateSolutionBuffer(shmSolutionBuffers_[shm_index]);

        shmem_->writeHeader(buffer.index, buffer.length);
        shmem_->incrementWriteIndex();

        shmem_->notifySignal();

        retval = p_cam_hal->releaseBuffer(&buffer);
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
            PLOGI("previewThread p_cam_hal(%p) : fps(%3.2f)", p_cam_hal,
                  debug_interval * 1000000.0f / us);
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

DEVICE_RETURN_CODE_T DeviceControl::startPreview(LSHandle *sh, const char *subskey)
{
    PLOGI("started !\n");

    sh_      = sh;
    subskey_ = subskey ? subskey : "";

    // get current saved format for device
    stream_format_t streamformat;
    auto retValue = p_cam_hal->getFormat(&streamformat);
    if (retValue != CAMERA_ERROR_NONE)
    {
        PLOGE("getFormat failed");
        return DEVICE_ERROR_UNKNOWN;
    }

    PLOGI("Driver set width : %d height : %d fps : %d", streamformat.stream_width,
          streamformat.stream_height, streamformat.stream_fps);

    solutionTextSize_   = 0;
    solutionBinarySize_ = 0;

    if (pCameraSolution != nullptr)
    {
        solutionTextSize_ = pCameraSolution->getMetaSizeHint();
    }

    size_t shmDataSize     = streamformat.buffer_size + extra_buffer;
    size_t shmMetaSize     = extra_buffer; // 1024
    size_t shmExtraSize    = sizeof(unsigned int);
    size_t shmSolutionSize = solutionTextSize_;

    shmDataBuffers = nullptr;
    std::string shmemName;
    shmem_ = std::make_unique<CameraSharedMemoryEx>();
    if (!shmem_)
    {
        PLOGE("Fail to create CameraSharedMemoryEx");
        return DEVICE_ERROR_UNKNOWN;
    }

    shmemName    = std::string("/camera.shm.") + std::to_string(getpid());
    shmBufferFd_ = shmem_->create(shmemName, shmDataSize, shmMetaSize, shmExtraSize,
                                  shmSolutionSize, FRAME_COUNT);
    if (shmBufferFd_ < 0)
    {
        PLOGE("Fail to create CameraSharedMemory : invalid FD");
        closeShmemoryIfNeeded();
        return DEVICE_ERROR_UNKNOWN;
    }

    //[Camera Solution Manager] initialization
    if (pCameraSolution != nullptr)
    {
        pCameraSolution->initialize(streamformat, shmemName, sh);
    }

    if (b_isstreamon_)
    {
        PLOGW("stream is already on!");
        return DEVICE_OK;
    }

    // user pointer buffer set-up.
    shmDataBuffers = (buffer_t *)calloc(FRAME_COUNT, sizeof(buffer_t));
    if (!shmDataBuffers)
    {
        PLOGE("USERPTR buffer allocation failed");
        closeShmemoryIfNeeded();
        return DEVICE_ERROR_UNKNOWN;
    }

    shmMetaBuffers_.resize(FRAME_COUNT);
    shmExtraBuffers_.resize(FRAME_COUNT);
    shmSolutionBuffers_.resize(FRAME_COUNT);

    std::vector<void *> dataList, metaList, extraList, solutionList;
    shmem_->getBufferList(&dataList, &metaList, &extraList, &solutionList);
    if (dataList.size() != FRAME_COUNT || metaList.size() != FRAME_COUNT ||
        extraList.size() != FRAME_COUNT)
    {
        PLOGE("buffer size error!");
        closeShmemoryIfNeeded();
        return DEVICE_ERROR_UNKNOWN;
    }

    size_t dataSize, metaSize, extraSize, solutionSize;
    shmem_->getBufferInfo(nullptr, &dataSize, &metaSize, &extraSize, &solutionSize);
    for (size_t i = 0; i < FRAME_COUNT; i++)
    {
        shmDataBuffers[i].start       = dataList[i];
        shmDataBuffers[i].length      = dataSize;
        shmMetaBuffers_[i].start      = metaList[i];
        shmMetaBuffers_[i].length     = metaSize;
        shmExtraBuffers_[i].start     = extraList[i];
        shmExtraBuffers_[i].length    = extraSize;
        shmSolutionBuffers_[i].start  = solutionList[i];
        shmSolutionBuffers_[i].length = solutionSize;
    }

    // [TODO][WRR-15621] Need support for mmap internally.
    // Handle cases where user pointer is not used.
    // In device_control, branch based on conditions other than memtype.
    // The shared memory implementation class must support writedata() for memcpy

    auto retval = p_cam_hal->setBuffer(FRAME_COUNT, IOMODE_USERPTR, (void **)&shmDataBuffers);
    if (retval != CAMERA_ERROR_NONE)
    {
        PLOGE("setBuffer failed");
        closeShmemoryIfNeeded();
        return DEVICE_ERROR_UNKNOWN;
    }

    retval = p_cam_hal->startCapture();
    if (retval != CAMERA_ERROR_NONE)
    {
        PLOGE("startCapture failed");
        closeShmemoryIfNeeded();
        return DEVICE_ERROR_UNKNOWN;
    }

    b_isstreamon_ = true;

    // create thread that will continuously capture images until stopcapture received
    PLOGI("make previewThread");
    tidPreview = std::thread{[this]() { this->previewThread(); }};

    PLOGI("end !");
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::stopPreview(bool forceComplete)
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

    if (forceComplete)
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

    if (shmem_)
    {
        shmem_.reset();
        shmBufferFd_ = -1;
        shmSignalFdMap_.clear();
    }

    shmMetaBuffers_.clear();
    shmExtraBuffers_.clear();

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

// deprecated
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

// deprecated
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

DEVICE_RETURN_CODE_T DeviceControl::capture(int ncount, const std::string &imagepath,
                                            std::vector<std::string> &capturedFiles)
{
    PLOGI("started ncount : %d \n", ncount);

    str_imagepath_   = imagepath;
    str_capturemode_ = (ncount == 1) ? cstr_oneshot : cstr_burst;
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

    ret = saveShmemory(ncount, capturedFiles);

    storageMonitor_.stopMonitor();

    return ret;
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
        static_cast<IHal *>(pInterface)->getInfo(pinfo, std::move(strdevicenode));
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

DEVICE_RETURN_CODE_T DeviceControl::addClient(int id)
{
    PLOGI("id %d", id);

    if (shmSignalFdMap_.count(id) > 0)
    {
        PLOGE("Already registered client %d", id);
        return DEVICE_OK;
    }

    if (!shmem_)
    {
        PLOGE("unable status to add client");
        return DEVICE_ERROR_UNKNOWN;
    }

    std::string name = std::string("signal.") + std::to_string(getpid()) + "." + std::to_string(id);
    int signalFd     = shmem_->createSignal(name);
    if (signalFd < 0)
    {
        PLOGE("Fail to create Signal for client %d", id);
        return DEVICE_ERROR_UNKNOWN;
    }

    shmSignalFdMap_[id] = signalFd;
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::removeClient(int id)
{
    PLOGI("id %d", id);
    if (shmSignalFdMap_.count(id) > 0)
    {
        if (shmem_)
        {
            std::string name =
                std::string("signal.") + std::to_string(getpid()) + "." + std::to_string(id);
            shmem_->detachSignal(name);
        }
        shmSignalFdMap_.erase(id);
    }
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getShmBufferFd(int *fd)
{
    if (shmBufferFd_ == -1)
    {
        PLOGE("Shmemory is not used for this session");
        *fd = -1;
        return DEVICE_ERROR_UNKNOWN;
    }

    *fd = shmBufferFd_;
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::getShmSignalFd(int id, int *fd)
{
    PLOGI("id %d", id);
    if (shmSignalFdMap_.count(id) == 0)
    {
        PLOGE("unregistered id");
        return DEVICE_ERROR_UNKNOWN;
    }

    *fd = shmSignalFdMap_[id];
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

DEVICE_RETURN_CODE_T DeviceControl::enableCameraSolution(const std::vector<std::string> &solutions)
{
    PLOGI("");

    if (pCameraSolution != nullptr)
    {
        return pCameraSolution->enableCameraSolution(solutions);
    }
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T DeviceControl::disableCameraSolution(const std::vector<std::string> &solutions)
{
    PLOGI("");

    if (pCameraSolution != nullptr)
    {
        return pCameraSolution->disableCameraSolution(solutions);
    }
    return DEVICE_OK;
}
//[Camera Solution Manager] interfaces end

std::string DeviceControl::createCaptureFileName(int cnt) const
{
    auto path = str_imagepath_;

    // check if specified location ends with '/' else add
    if (!path.empty())
    {
        char ch = path.back();
        if (ch != '/')
            path += "/";
    }
    path += "Picture";

    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t t                             = std::chrono::system_clock::to_time_t(now);
    if (t == static_cast<std::time_t>(-1))
    {
        PLOGE("failed to get current time");
        return path;
    }

    std::tm timeInfo;
    if (localtime_r(&t, &timeInfo) == nullptr)
    {
        PLOGE("localtime_r() failed");
        return path;
    }

    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    // "DDMMYYYY-HHMMSSss"
    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << timeInfo.tm_mday << std::setw(2)
        << std::setfill('0') << (timeInfo.tm_mon) + 1 << std::setw(2) << std::setfill('0')
        << (timeInfo.tm_year) + 1900 << "-" << std::setw(2) << std::setfill('0') << timeInfo.tm_hour
        << std::setw(2) << std::setfill('0') << timeInfo.tm_min << std::setw(2) << std::setfill('0')
        << timeInfo.tm_sec << std::setw(2) << std::setfill('0') << tmnow.tv_usec / 10000;

    path += oss.str();

    if (cstr_burst == str_capturemode_)
    {
        path += '_' + std::to_string(cnt);
    }

    if (capture_format_.eFormat == CAMERA_FORMAT_JPEG)
        path += ".jpeg";
    else if (capture_format_.eFormat == CAMERA_FORMAT_YUV)
        path += ".yuv";
    else if (capture_format_.eFormat == CAMERA_FORMAT_H264ES)
        path += ".h264";

    PLOGD("path : %s", path.c_str());
    return path;
}

void DeviceControl::closeShmemoryIfNeeded()
{
    if (shmDataBuffers)
    {
        free(shmDataBuffers);
        shmDataBuffers = nullptr;
    }

    shmMetaBuffers_.clear();
    shmExtraBuffers_.clear();

    if (shmem_)
    {
        shmem_.reset();
        shmBufferFd_ = -1;
    }

    PLOGI("closed Shmemory !");
}
