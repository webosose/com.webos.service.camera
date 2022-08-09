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

#include "camera_solution_async.h"
#include "camera_types.h"
#include <list>
#include <numeric>

#define LOG_TAG "CameraSolutionAsync"

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

        if (frameCount_ < 30)
            return;

        uint64_t totalDur = std::accumulate(lstDur_.begin(), lstDur_.end(), 0);
        long avrDur       = totalDur / (frameCount_ - 1);
        avrFPS_           = 1000000.0f / avrDur;

        if (avrFPS_ > targetFPS_)
        {
            if ((avrFPS_ - targetFPS_) > 1)
                timeMultiple_ += 5;
            else
                timeMultiple_ += 1;
        }
        else
        {
            if (timeMultiple_ > 0)
                timeMultiple_ -= 1;
        }

        PMLOG_INFO(LOG_TAG, ">>>>>> fps : %f, target_fps : %f <<<<<<", avrFPS_, targetFPS_);

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

CameraSolutionAsync::~CameraSolutionAsync(void) { PMLOG_INFO(LOG_TAG, ""); }

void CameraSolutionAsync::release()
{
    PMLOG_INFO(LOG_TAG, "");
    setEnableValue(false);
    PMLOG_INFO(LOG_TAG, "");
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

void CameraSolutionAsync::processForSnapshot(buffer_t inBuf) { pushJob(inBuf); }

void CameraSolutionAsync::processForPreview(buffer_t inBuf) { pushJob(inBuf); }

void CameraSolutionAsync::run(void)
{
    PerformanceControl oPC;
    while (checkAlive())
    {
        WaitResult resWait = wait();
        if (resWait == TIMEOUT)
        {
            popJob();
            continue;
        }
        if (resWait == ERROR)
        {
            setAlive(false);
            break;
        }
        if (checkAlive())
        {
            processing();
            oPC.sampleFPS();
            oPC.calculateFPS();
            popJob();
        }
    }
    postProcessing();
}

void CameraSolutionAsync::startThread(void)
{
    if (threadJob_ == nullptr)
    {
        PMLOG_INFO(LOG_TAG, "Thread Start");
        try
        {
            setAlive(true);
            threadJob_ = std::make_unique<std::thread>([&](void) { run(); });
        }
        catch (const std::system_error &e)
        {
            PMLOG_INFO(LOG_TAG, "Caught a system error with code %d meaning %s", e.code().value(),
                       e.what());
        }
        PMLOG_INFO(LOG_TAG, "Thread Started");
    }
}

void CameraSolutionAsync::stopThread(void)
{
    if (threadJob_ != nullptr && threadJob_->joinable())
    {
        PMLOG_INFO(LOG_TAG, "Thread Closing");
        try
        {
            setAlive(false);
            notify();
            threadJob_->join();
        }
        catch (const std::system_error &e)
        {
            PMLOG_INFO(LOG_TAG, "Caught a system error with code %d meaning %s", e.code().value(),
                       e.what());
        }
        threadJob_.reset();
        PMLOG_INFO(LOG_TAG, "Thread Closed");
    }
}

void CameraSolutionAsync::notify(void) { cv_.notify_all(); }

CameraSolutionAsync::WaitResult CameraSolutionAsync::wait(void)
{
    WaitResult res = OK;
    try
    {
        std::unique_lock<std::mutex> lock(m_);
        auto status = cv_.wait_for(lock, 1s);
        if (status == std::cv_status::timeout)
        {
            PMLOG_INFO(LOG_TAG, "Timeout to wait for Feed");
            res = TIMEOUT;
        }
    }
    catch (std::system_error &e)
    {
        PMLOG_INFO(LOG_TAG, "Caught a system_error with code %d meaning %s", e.code().value(),
                   e.what());
        res = ERROR;
    }

    return res;
}

bool CameraSolutionAsync::checkAlive(void) { return bAlive_; }

void CameraSolutionAsync::setAlive(bool bAlive) { bAlive_ = bAlive; }

void CameraSolutionAsync::pushJob(buffer_t inBuf)
{
    if (queueJob_.empty())
    {
        std::lock_guard<std::mutex> lg(mtxJob_);
        queueJob_.push(std::make_unique<Buffer>((uint8_t *)inBuf.start, inBuf.length));
        notify();
    }
}

void CameraSolutionAsync::popJob(void)
{
    std::lock_guard<std::mutex> lg(mtxJob_);
    if (!queueJob_.empty())
    {
        queueJob_.pop();
    }
}
