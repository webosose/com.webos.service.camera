/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolutionManager.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution Manager
 *
 */

#ifndef __CAMERASOLUTIONMANAGER_H__
#define __CAMERASOLUTIONMANAGER_H__

#include "CameraSolution.h"

#include <list>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <pbnjson.hpp>
#include "json_utils.h"
#include "json_parser.h"

class CameraSolutionManager {
public:
    CameraSolutionManager();
    virtual ~CameraSolutionManager();

    void initializeForSolutions(stream_format_t streamformat);
    void releaseForSolutions();
    void processCaptureForSolutions(buffer_t frame_buffer, stream_format_t streamformat);
    void processPreviewForSolutions(buffer_t frame_buffer, stream_format_t streamformat);
    void getSupportedSolutionInfo(std::vector<std::string>&);
    void getEnabledSolutionInfo(std::vector<std::string>& solutionsInfo);
    DEVICE_RETURN_CODE_T enableCameraSolution(const std::vector<std::string>);
    DEVICE_RETURN_CODE_T disableCameraSolution(const std::vector<std::string>);

private:

    CameraSolutionManager(const CameraSolutionManager &manager) { };
    CameraSolutionManager& operator=(const CameraSolutionManager& src) { return *this; }

    void releaseSolutionList();
    LgSolutionErrorValue loadSolutionList(pbnjson::JValue& json);
    bool isSolutionSupported(const pbnjson::JValue& json, const std::string& key);

    std::list<CameraSolution *> mTotalSolutionList;

};

#endif //__LGCAMERASOLUTIONMANAGER_H__
