/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolutionAsync.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution Async
 *
 */

#define LOG_TAG "CameraSolutionAsync"
#include "camera_solution_async.h"
#include "camera_shared_memory_ex.h"
#include "camera_types.h"
#include <list>
#include <numeric>
#include <system_error>

using namespace std::chrono_literals;

struct PerformanceControl
{
    int64_t timeMultiple_{0};
    int64_t timeScale_{10000};
    double targetFPS_{3.0f};
    double avrFPS_{0.0f};
    size_t frameCount_{0};
    int64_t prevClk_{0};
    std::list<int64_t> lstDur_;

    void sampleFPS(void)
    {
        if (prevClk_ == 0)
        {
            prevClk_ = g_get_monotonic_time();
        }
        else
        {
            int64_t currClk = g_get_monotonic_time();
            int64_t dur     = currClk - prevClk_;
            lstDur_.push_back(dur);
            prevClk_ = currClk;
        }
        frameCount_ = lstDur_.size();
    }

    void resetFPS(void)
    {
        lstDur_.clear();
        frameCount_ = 0;
    }

    void calculateFPS(void)
    {
        if (timeMultiple_ > 0)
        {
            long usleep_time = timeScale_ * timeMultiple_;
            g_usleep((usleep_time > 0) ? (unsigned long)usleep_time : 0);
        }

        int adj_int      = (abs(avrFPS_ - 0.0) < 1e-9) ? 0 : (avrFPS_ > 1 ? 27 / (int)avrFPS_ : 27);
        size_t adj       = (adj_int > 0) ? adj_int : 0;
        size_t adj_check = (adj < 30) ? 30 - adj : 0;

        if (frameCount_ < adj_check)
            return;

        int tot_dur_int   = std::accumulate(lstDur_.begin(), lstDur_.end(), 0);
        size_t totalDur   = (tot_dur_int > 0) ? (size_t)tot_dur_int : 0;
        size_t ave_dur_ul = totalDur / (frameCount_ - 1);
        long avrDur       = (ave_dur_ul > LONG_MAX) ? LONG_MAX : (long)ave_dur_ul;
        avrFPS_           = 1000000.0f / avrDur;

        if (avrFPS_ > targetFPS_)
        {
            if ((avrFPS_ - targetFPS_) > 0.5)
                timeMultiple_ += 10;
            else
                timeMultiple_ += 1;
        }
        else
        {
            if (timeMultiple_ > 0)
                timeMultiple_ -= 1;
        }

        PLOGI(">>>>>> fps : %f, target_fps : %f <<<<<<", avrFPS_, targetFPS_);

        resetFPS();
    }

    void targetFPS(double targetFPS) { targetFPS_ = targetFPS; }
};

CameraSolutionAsync::Buffer::Buffer(uint8_t *data, uint32_t size)
{
    if (data_ == nullptr && data != nullptr && size != 0)
    {
        size_ = size;
        data_ = new uint8_t[size_];
        memcpy(data_, data, size_);
    }
}

CameraSolutionAsync::Buffer::~Buffer(void)
{
    if (data_ != nullptr)
    {
        delete[] data_;
    }
}

CameraSolutionAsync::CameraSolutionAsync(void) {}

CameraSolutionAsync::~CameraSolutionAsync(void) { PLOGI(""); }

void CameraSolutionAsync::release()
{
    PLOGI("");
    setEnableValue(false);
    PLOGI("");
}

void CameraSolutionAsync::setEnableValue(bool enableValue)
{
    CameraSolution::setEnableValue(enableValue);
    if (enableStatus_ == true)
    {
        startThread();
    }
    else
    {
        stopThread();
    }
}

void CameraSolutionAsync::processForSnapshot(const void *inBuf) {}

void CameraSolutionAsync::processForPreview(const void *inBuf) {}

void CameraSolutionAsync::run(void)
{
    PerformanceControl oPC;
    oPC.targetFPS(1.0f);
    int shmBufferFd = -1;
    PLOGI("[%s] shmName(%s)", name_.c_str(), shmName_.c_str());

    pthread_setname_np(pthread_self(), "solution_async");

    camShmem_ = std::make_unique<CameraSharedMemoryEx>();
    if (!camShmem_)
    {
        PLOGE("Fail to create CameraSharedMemroyEx");
        return;
    }

    shmBufferFd = camShmem_->open(shmName_);
    PLOGI("[%s] camShmem_->open() fd(%d)", name_.c_str(), shmBufferFd);

    if (shmBufferFd < 0)
    {
        PLOGE("Fail to open CameraSharedMemory");

        if (camShmem_)
            camShmem_.reset();
        return;
    }

    // [TODO][WRR-15623] Apply sync operation to the solution
    // It is necessary to register a solution signal FD in a half process.
    // After adding this, the skipsignal of open() should be erased.

    // shmSignalFd_ = camShmem_->createSignal();
    // PLOGI("[%s] camShmem_->createSignal() fd(%d)", name_.c_str(), shmSignalFd_);

    // if (shmSignalFd_ < 0)
    // {
    //     PLOGE("[%s] Fail to create Signal in CameraSharedMemory", name_.c_str());

    //     if (camShmem_)
    //         camShmem_.reset();
    //     return;
    // }

    while (checkAlive())
    {
        size_t data_len           = 0;
        size_t extra_len          = 0;
        size_t meta_len           = 0;
        unsigned char *data_addr  = NULL;
        unsigned char *extra_addr = NULL;
        unsigned char *meta_addr  = NULL;

        // Read Shared memory
        if (!camShmem_)
        {
            PLOGE("[%s] camShmem_ fail", name_.c_str());
            break;
        }

        bool status = camShmem_->read(&data_addr, &data_len, &meta_addr, &meta_len, &extra_addr,
                                      &extra_len, nullptr, nullptr, 1000, true);

        PLOGD("[%s] camShmem_->read() data_len(%zu) meta_len(%zu) extra_len(%zu)", name_.c_str(),
              data_len, meta_len, extra_len);

        if (status == false)
        {
            PLOGE("[%s] shared memory read fail", name_.c_str());
            break;
        }

        if (data_len == 0)
        {
            g_usleep(1000);
            continue;
        }

        buffer_t inBuf;
        inBuf.start  = data_addr;
        inBuf.length = data_len;
        pushJob(inBuf);

        if (checkAlive())
        {
            processing();
            oPC.sampleFPS();
            oPC.calculateFPS();
            popJob();
        }
    }
    postProcessing();

    if (camShmem_)
    {
        PLOGI("[%s] camShmem_.close", name_.c_str());
        camShmem_->close();
        camShmem_.reset();
    }
}

void CameraSolutionAsync::startThread(void)
{
    if (threadJob_ == nullptr)
    {
        PLOGI("Thread Start");
        try
        {
            setAlive(true);
            threadJob_ = std::make_unique<std::thread>([&](void) { run(); });
        }
        catch (const std::system_error &e)
        {
            PLOGI("Caught a system error with code %d meaning %s", e.code().value(), e.what());
        }
        PLOGI("Thread Started");
    }
}

void CameraSolutionAsync::stopThread(void)
{
    if (threadJob_ != nullptr && threadJob_->joinable())
    {
        PLOGI("Thread Closing");
        try
        {
            setAlive(false);
            threadJob_->join();
        }
        catch (const std::system_error &e)
        {
            PLOGI("Caught a system error with code %d meaning %s", e.code().value(), e.what());
        }
        threadJob_.reset();
        PLOGI("Thread Closed");
    }
}

bool CameraSolutionAsync::checkAlive(void) { return bAlive_; }

void CameraSolutionAsync::setAlive(bool bAlive) { bAlive_ = bAlive; }

void CameraSolutionAsync::pushJob(buffer_t inBuf)
{
    if (queueJob_.empty())
    {
        queueJob_.push(std::make_unique<Buffer>((uint8_t *)inBuf.start, inBuf.length));
    }
}

void CameraSolutionAsync::popJob(void)
{
    if (!queueJob_.empty())
    {
        queueJob_.pop();
    }
}
