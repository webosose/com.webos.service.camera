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
#include "camera_types.h"
#include "ipc_shared_memory.h"
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
    uint32_t frameCount_{0};
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
            g_usleep(timeScale_ * timeMultiple_);

        uint32_t adj = abs(avrFPS_ - 0.0) < 1e-9 ? 0 : avrFPS_ > 1 ? 27 / (int)avrFPS_ : 27;

        if (frameCount_ < (30 - adj))
            return;

        uint64_t totalDur = std::accumulate(lstDur_.begin(), lstDur_.end(), 0);
        long avrDur       = totalDur / (frameCount_ - 1);
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

    pthread_setname_np(pthread_self(), "solution_async");

    SHMEM_HANDLE hShm  = nullptr;
    SHMEM_STATUS_T ret = IPCSharedMemory::getInstance().OpenShmem(&hShm, shm_key);
    PLOGI("OpenShem %d", ret);

    if (ret != SHMEM_IS_OK)
    {
        PLOGE("Fail : OpenShmem RET => %d\n", ret);
        return;
    }

    while (checkAlive())
    {
        int len                    = 0;
        unsigned char *sh_mem_addr = NULL;
        IPCSharedMemory::getInstance().ReadShmem(hShm, &sh_mem_addr, &len);
        if (len == 0)
        {
            g_usleep(1000);
            continue;
        }

        buffer_t inBuf;
        inBuf.start  = sh_mem_addr;
        inBuf.length = len;
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

    if (hShm)
    {
        SHMEM_STATUS_T ret = IPCSharedMemory::getInstance().CloseShmemory(&hShm);
        PLOGI("CloseShmemory %d", ret);
        if (ret != SHMEM_IS_OK)
            PLOGE("CloseShmemory error %d \n", ret);
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
