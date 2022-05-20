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
 * Description  Camera Solution Manager
 *
 */

#include "CameraSolutionManager.h"
#include "CameraSolution.h"

// System definitions
#include <dlfcn.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>


CameraSolution::CameraSolution(CameraSolutionManager* mgr)
    : m_manager(mgr)
{

}

CameraSolution::~CameraSolution() {

}


