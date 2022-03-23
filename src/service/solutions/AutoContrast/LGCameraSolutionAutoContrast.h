/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionAutoContrast.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  AutoContrast
 *
 */

#ifndef _AUTO_CONTRAST_
#define _AUTO_CONTRAST_

#include "../CameraSolution.h"
#include "../CameraSolutionManager.h"

#define LOG_TAG "LGCameraSolutionAutoContrast"

class LGCameraSolutionAutoContrast : public CameraSolution {
public:
    LGCameraSolutionAutoContrast(CameraSolutionManager *mgr);
    virtual ~LGCameraSolutionAutoContrast();
    void initialize(stream_format_t streamformat);
    bool isSupported(){return mSupportStatus;};
    bool isEnabled(){return mEnableStatus;};
    void setSupportStatus(bool supportStatus){mSupportStatus = supportStatus;};
    void setEnableValue(bool enableValue){        mEnableStatus = enableValue;};

    std::string getSolutionStr();
    void processForSnapshot(buffer_t inBuf,        stream_format_t streamformat);
    void processForPreview(buffer_t inBuf, stream_format_t streamformat);
    void release();

private:
    void doAutoContrastProcessing(buffer_t inBuf,        stream_format_t streamformat);
    void brightnessEnhancement(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, int minY, int maxY, int enhanceLevel);
    void contrastEnhancement(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, int minY, int maxY, int enhanceLevel);
    int dumpFrame(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, char* filename, char* filepath);
    void *handle;
    bool mIsDumpEnabled;
    bool mIsSimulationEnabled;
    // isOutdoor;
    int input_num;
    bool isLogEnabled;
    bool mSupportStatus = false;
    bool mEnableStatus = false;



};
#endif


