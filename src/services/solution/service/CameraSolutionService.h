/**
 * Copyright(c) 2023 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolutionService.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution service
 *
 */

#pragma once

#include "camera_hal_if.h"
#include "camshm.h"
#include "luna-service2/lunaservice.hpp"
#include <glib.h>

class CameraSolution;
class CameraSolutionService : public LS::Handle
{
    using mainloop          = std::unique_ptr<GMainLoop, void (*)(GMainLoop *)>;
    mainloop main_loop_ptr_ = {g_main_loop_new(nullptr, false), g_main_loop_unref};

    std::unique_ptr<CameraSolution> pCameraSolution;

public:
    CameraSolutionService(const char *service_name);

    CameraSolutionService(CameraSolutionService const &)            = delete;
    CameraSolutionService(CameraSolutionService &&)                 = delete;
    CameraSolutionService &operator=(CameraSolutionService const &) = delete;
    CameraSolutionService &operator=(CameraSolutionService &&)      = delete;

    bool createSolution(LSMessage &message);
    bool getMetaSizeHint(LSMessage &message);
    bool initialize(LSMessage &message);
    bool setEnableValue(LSMessage &message);
    bool getProperty(LSMessage &message);
    bool isEnabled(LSMessage &message);
    bool getSolutionStr(LSMessage &message);
    bool processForSnapshot(LSMessage &message);
    bool processForPreview(LSMessage &message);
    bool release(LSMessage &message);
    bool subscribe(LSMessage &);

private:
    bool openFrameBuffer(key_t shmemKey);
    bool readFrameBuffer(SHMEM_HANDLE hShm);

    buffer_t frameBuffer;
    SHMEM_HANDLE hShm;
};
