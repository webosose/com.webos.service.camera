/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolution.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution
 *
 */

#pragma once

// System dependencies
#include "camera_hal_if.h"
#include <atomic>
#include <string>

#define SOLUTION_AUTOCONTRAST "AutoContrast"
#define SOLUTION_DUMMY "Dummy"
#define SOLUTION_FACE_DETECTION_AIF "FaceDetectionAIF"

enum Property
{
    LG_SOLUTION_NONE     = 0x0000,
    LG_SOLUTION_PREVIEW  = 0x0001,
    LG_SOLUTION_SNAPSHOT = 0x0002
};

struct CameraSolutionEvent;
class CameraSolution
{
public:
    CameraSolution(void) {}
    virtual ~CameraSolution(void) {}

public:
    // interface - pre-defined (could be overridden)
    virtual void setEventListener(CameraSolutionEvent *pEvent) { pEvent_ = pEvent; }
    virtual int32_t getMetaSizeHint(void) { return 0; }
    virtual void initialize(stream_format_t streamFormat);
    virtual void setEnableValue(bool enableValue) { enableStatus_ = enableValue; };
    virtual Property getProperty() { return solutionProperty_; };
    virtual bool isEnabled(void) { return enableStatus_; };
    // interfce - need to override
    virtual std::string getSolutionStr(void)        = 0;
    virtual void processForSnapshot(buffer_t inBuf) = 0;
    virtual void processForPreview(buffer_t inBuf)  = 0;
    virtual void release(void)                      = 0;

protected:
    Property solutionProperty_{LG_SOLUTION_NONE};
    bool supportStatus_{false};
    bool enableStatus_{false};
    stream_format_t streamFormat_{CAMERA_PIXEL_FORMAT_JPEG, 0, 0, 0, 0};
    std::atomic<CameraSolutionEvent *> pEvent_{nullptr};
};
