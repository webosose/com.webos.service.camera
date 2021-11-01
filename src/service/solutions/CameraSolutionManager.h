/*
 * Mobile Communication Company, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2017 by LG Electronics Inc.
 *
 * All rights reserved. No part of this work may be reproduced, stored in a
 * retrieval system, or transmitted by any means without prior written
 * Permission of LG Electronics Inc.
 */

#ifndef __CAMERASOLUTIONMANAGER_H__
#define __CAMERASOLUTIONMANAGER_H__

#include "CameraSolutionCommon.h"
#include "CameraSolution.h"

#include <list>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include "pbnjson.h"
#include "json_utils.h"
#include "json_parser.h"

#define MAX_SOLUTION_FRAME_NUM 20
#define DUMP_BEFORE 0
#define DUMP_AFTER  1

typedef enum {
    AutoContrastIndex = 0,
    FaceDetectionIndex,
    FmkSolutionNum,
}LgSolutionIndex;

typedef enum {
    NO_ERROR = 0,
    NO_SUPPORTED_SOLUTION,
    NO_ENABLED_SOLUTION,
    PARAMETER_ERROR,
    MAX_ERROR_INDEX,
}LgSolutionErrorValue;


class CameraSolutionManager {
public:
    CameraSolutionManager();
    virtual ~CameraSolutionManager();


    void registerSolutionList(stream_format_t streamformat);
    void releaseSolutionList();

    void initializeForSolutions(stream_format_t streamformat, int key);
    void releaseForSolutions();
    void processCaptureForSolutions(void* frame_buffer, stream_format_t streamformat);
    void processPreviewForSolutions(void* frame_buffer, stream_format_t streamformat);

    void releaseSolutions();
    void dumpImage(CameraSolution* solution, image_buf_t images[], int numImageBuffer, int type);

    bool isEnableSolution(int solutionIndex);
    std::string getSupportedSolutionInfo();
    std::string enableCameraSolutionInfo(const char *enabledSolutionList);
    std::string disableCameraSolutionInfo(const char *disabledSolutionList);

    std::list<CameraSolution *> mTotalSolutionList;
    std::list<CameraSolution *> mEnabledSolutionList;

private:
    CameraSolutionManager(const CameraSolutionManager &manager) { };
    CameraSolutionManager& operator=(const CameraSolutionManager& src) { return *this; }

    std::string createSolutionListObjectJsonString();
    std::string enableSolutionListObjectJsonString(const char *enabledSolutionList);
    std::string disableSolutionListObjectJsonString(const char *disabledSolutionList);

    parameter_t mPreviewParameters;
    parameter_t mSnapshotParameters;

    int mEnabledSolutions;
    int mEnabledSolutionCount;

    int mEnableDump;

    CameraSolution* mAutoContrast;
    CameraSolution* mFaceDetection;

};

#endif //__LGCAMERASOLUTIONMANAGER_H__
