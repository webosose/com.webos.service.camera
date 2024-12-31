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

#define LOG_TAG "CameraSolution"
#include "camera_solution.h"
#include "camera_types.h"

void CameraSolution::initialize(const void *streamFormat, const std::string &shmName,
                                void *lsHandle)
{
    name_ = getSolutionStr().c_str();

    PLOGI("%s", name_.c_str());

    streamFormat_ = *static_cast<const stream_format_t *>(streamFormat);
    shmName_      = shmName;
    sh_           = static_cast<LSHandle *>(lsHandle);

    if (pEvent_)
        (pEvent_.load())->onInitialized();
}
