/**
 * Copyright(c) 2015 by LG Electronics Inc.
 * Mobile Communication Company, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionAutoContrast.cpp
 * @contact     kwanghee.choi@lge.com
 *
 * Description  AutoContrast
 *
 */

#include "../CameraSolution.h"
#include "../CameraSolutionCommon.h"
#include "../CameraSolutionManager.h"

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "LGCameraSolutionAutoContrast"

class LGCameraSolutionAutoContrast : public CameraSolution {
public:
    bool mSupportStatus = false;
    bool mEnableStatus = false;

    LGCameraSolutionAutoContrast(CameraSolutionManager *mgr);
    virtual ~LGCameraSolutionAutoContrast();
    void initialize(stream_format_t streamformat, int key);
    bool isSupported(){return mSupportStatus;};
    bool isEnabled(){return mEnableStatus;};
    void setSupportStatus(bool supportStatus){mSupportStatus = supportStatus;};
    void setEnableValue(bool enableValue){
        printf("Solution Auto Contrast setEnableValue %d\n",enableValue);
        mEnableStatus = enableValue;};

    bool needThread(){return true;};
    void startThread(stream_format_t streamformat, int key){};
    std::string getSolutionStr();
    void processForSnapshot(void* inBuf,        stream_format_t streamformat);
    void processForPreview(void* inBuf, stream_format_t streamformat);
    void release();
private:
    void doAutoContrastProcessing(void* inBuf,        stream_format_t streamformat);
    void brightnessEnhancement(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, int minY, int maxY, int enhanceLevel);
    void contrastEnhancement(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, int minY, int maxY, int enhanceLevel);
    int dumpFrame(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, char* filename, char* filepath);
    void *handle;
    bool mIsDumpEnabled;
    bool mIsSimulationEnabled;
    // isOutdoor;
    int input_num;
    bool isLogEnabled;


};


