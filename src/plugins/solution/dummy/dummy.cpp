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

#define LOG_TAG "SOLUTION:Dummy"
#include "dummy.hpp"
#include "camera_log.h"

Dummy::Dummy(void)
{
    PLOGI("");
    solutionProperty_ = LG_SOLUTION_PREVIEW | LG_SOLUTION_SNAPSHOT;
}

Dummy::~Dummy(void)
{
    PLOGI("");
    setEnableValue(false);
}

std::string Dummy::getSolutionStr(void) { return SOLUTION_DUMMY; }

void Dummy::processForSnapshot(const void *inBuf) {}

void Dummy::processForPreview(const void *inBuf) {}

void Dummy::release(void) { PLOGI(""); }
