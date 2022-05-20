/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionDummy.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution Dummy
 *
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/time.h>

#include "LGCameraSolutionDummy.h"

LGCameraSolutionDummy::LGCameraSolutionDummy(CameraSolutionManager *mgr)
        : CameraSolution(mgr)
{
    PMLOG_INFO(CONST_MODULE_SM, " E\n");
    solutionProperty = LG_SOLUTION_PREVIEW | LG_SOLUTION_VIDEO | LG_SOLUTION_SNAPSHOT;
}

LGCameraSolutionDummy::~LGCameraSolutionDummy()
{
    PMLOG_INFO(CONST_MODULE_SM, " E\n", __func__);
    setEnableValue(false);
}

void LGCameraSolutionDummy::initialize(stream_format_t streamformat) {
    PMLOG_INFO(CONST_MODULE_SM, " E\n", __func__);

}

std::string LGCameraSolutionDummy::getSolutionStr(){
    std::string solutionStr = SOLUTION_DUMMY;
    return solutionStr;
}

void LGCameraSolutionDummy::processForSnapshot(buffer_t inBuf,        stream_format_t streamformat)
{

}

void LGCameraSolutionDummy::processForPreview(buffer_t inBuf,        stream_format_t streamformat)
{

}

void LGCameraSolutionDummy::doDummyProcessing(buffer_t inBuf, stream_format_t streamformat)
{
    PMLOG_INFO(CONST_MODULE_SM, " E\n");


    PMLOG_INFO(CONST_MODULE_SM, " X. /n",__func__);
}

void LGCameraSolutionDummy::release() {
    PMLOG_INFO(CONST_MODULE_SM, " E. /n",__func__);
}

