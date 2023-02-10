/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolutionManager.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution Manager
 *
 */

#pragma once

#include "camera_hal_if_types.h"
#include "camera_types.h"
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class CameraSolutionProxy;
class CameraSolutionEvent;
class CameraSolutionManager
{
    CameraSolutionManager(const CameraSolutionManager &)            = delete;
    CameraSolutionManager &operator=(const CameraSolutionManager &) = delete;

public:
    using SolutionNames = std::vector<std::string>;
    using SolutionList  = std::list<std::unique_ptr<CameraSolutionProxy>>;

public:
    CameraSolutionManager(void);
    ~CameraSolutionManager(void);

public:
    // To process
    void setEventListener(CameraSolutionEvent *pEvent);
    int32_t getMetaSizeHint(void);
    void initialize(stream_format_t streamFormat, int shmKey);
    void release(void);
    void processCapture(buffer_t frame_buffer);
    void processPreview(buffer_t frame_buffer);

public:
    // To support luna api
    void getSupportedSolutionInfo(SolutionNames &names);
    void getEnabledSolutionInfo(SolutionNames &names);
    DEVICE_RETURN_CODE_T enableCameraSolution(const SolutionNames &names);
    DEVICE_RETURN_CODE_T disableCameraSolution(const SolutionNames &names);

public:
    static void getSupportedSolutionList(std::vector<std::string> &supportedList,
                                         std::vector<std::string> &enabledList);

private:
    SolutionList lstSolution_;
    std::mutex mtxApi_;
};
