/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionDummy.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution Dummy
 *
 */

#ifndef _DUMMY_
#define _DUMMY_

#include "../CameraSolution.h"
#include "../CameraSolutionManager.h"

#define LOG_TAG "LGCameraSolutionDummy"

class LGCameraSolutionDummy : public CameraSolution {
public:
    bool mSupportStatus = false;
    bool mEnableStatus = false;

    LGCameraSolutionDummy(CameraSolutionManager *mgr);
    virtual ~LGCameraSolutionDummy();
    void initialize(stream_format_t streamformat);
    bool needThread(){return true;};
    void startThread(stream_format_t streamformat){};
    std::string getSolutionStr();
    void processForSnapshot(buffer_t inBuf,        stream_format_t streamformat);
    void processForPreview(buffer_t inBuf, stream_format_t streamformat);
    void release();
private:
    void doDummyProcessing(buffer_t inBuf,        stream_format_t streamformat);

};
#endif


