/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolutionAsync.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution Async
 *
 */

#pragma once

#include "camera_hal_types.h"
#include "camera_solution.h"
#include <atomic>
#include <cstring>
#include <memory>
#include <queue>
#include <thread>

const char *const SOL_SUBSCRIPTION_KEY = "cameraSolution";

class CameraSolutionAsync : public CameraSolution
{
public:
    struct Buffer
    {
        uint8_t *data_{nullptr};
        uint32_t size_{0};
        Buffer(uint8_t *data, uint32_t size);
        ~Buffer(void);
    };
    using Queue  = std::queue<std::unique_ptr<Buffer>>;
    using Thread = std::unique_ptr<std::thread>;

public:
    CameraSolutionAsync(void);
    virtual ~CameraSolutionAsync(void);

public:
    // interface override
    virtual void setEnableValue(bool enableValue) override;
    virtual void processForSnapshot(const void *inBuf) override;
    virtual void processForPreview(const void *inBuf) override;
    virtual void release(void) override;

protected:
    virtual void run(void);
    virtual void processing(void)     = 0;
    virtual void postProcessing(void) = 0;

protected:
    void startThread(void);
    void stopThread(void);
    bool checkAlive(void);
    void setAlive(bool bAlive);

protected:
    void pushJob(buffer_t inBuf);
    void popJob(void);

protected:
    Queue queueJob_;
    Thread threadJob_;
    std::atomic<bool> bAlive_{false};
};
