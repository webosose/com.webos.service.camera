/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    FaceDetectionNative.hpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution FaceDetectionCNN
 *
 */

#pragma once

#include "camera_solution_async.h"

class FaceDetectionCNN : public CameraSolutionAsync
{
    struct RawImage
    {
        uint32_t srcColorSpace_{0};
        uint32_t outColorSpace_{0};
        uint32_t srcWidth_{0};
        uint32_t srcHeight_{0};
        uint32_t outWidth_{0};
        uint32_t outHeight_{0};
        uint32_t outChannels_{0};
        uint32_t outStride_{0};
        uint8_t *pImage_{nullptr};
        RawImage(void) {}
        ~RawImage(void)
        {
            if (pImage_)
                delete[] pImage_;
        }
        void prepareImage(void)
        {
            if (!pImage_ && outHeight_ && outWidth_ && outChannels_)
                pImage_ = new uint8_t[outWidth_ * outHeight_ * outChannels_];
        }
        uint8_t *getLine(uint32_t lineNumber) { return pImage_ + lineNumber * outStride_; }
    };

public:
    FaceDetectionCNN(void);
    virtual ~FaceDetectionCNN(void);

public:
    // interface override
    virtual int32_t getMetaSizeHint(void) override;
    virtual std::string getSolutionStr(void) override;
    // interface override from CameraSolutionAsync
    virtual void processing(void) override;
    virtual void postProcessing(void) override;

private:
    bool detectFace(void);
    bool decodeJpeg(void);

private:
    RawImage oDecodedImage_;
    uint8_t *pBuffer_{nullptr};
};
