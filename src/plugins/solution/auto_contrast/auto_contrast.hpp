/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    AutoContrast.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  AutoContrast
 *
 */

#pragma once

#include "camera_solution.h"

class AutoContrast : public CameraSolution
{
public:
    AutoContrast(void);
    virtual ~AutoContrast(void);

public:
    // interface override
    virtual std::string getSolutionStr(void) override;
    virtual void processForSnapshot(const void *inBuf) override;
    virtual void processForPreview(const void *inBuf) override;
    virtual void release(void) override;

private:
    void doAutoContrastProcessing(buffer_t inBuf);
};
