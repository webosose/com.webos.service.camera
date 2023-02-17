/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolution.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution
 *
 */

#include "camera_solution.h"
#include "camera_solution_event.h"
#include "camera_types.h"

#define LOG_TAG "CameraSolution"

void CameraSolution::initialize(stream_format_t streamFormat, int shmKey, LSHandle *sh)
{
    PMLOG_INFO(LOG_TAG, "%s", getSolutionStr().c_str());
    streamFormat_ = streamFormat;
    shm_key       = shmKey;
    sh_           = sh;

    if (pEvent_)
        (pEvent_.load())->onInitialized();
}
