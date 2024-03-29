/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    Dummy.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution Dummy
 *
 */

#include "dummy.hpp"
#include "camera_types.h"

#define LOG_TAG "Dummy"

Dummy::Dummy(void)
{
    PMLOG_INFO(LOG_TAG, "");
    solutionProperty_ = Property(LG_SOLUTION_PREVIEW | LG_SOLUTION_SNAPSHOT);
}

Dummy::~Dummy(void)
{
    PMLOG_INFO(LOG_TAG, "");
    setEnableValue(false);
}

void Dummy::processForSnapshot(buffer_t inBuf) {}

void Dummy::processForPreview(buffer_t inBuf) {}

void Dummy::release(void) { PMLOG_INFO(LOG_TAG, ""); }
