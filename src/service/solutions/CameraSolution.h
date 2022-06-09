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
 * Description  Camera Solution Manager
 *
 */


#ifndef __LGCAMERASOLUTION_H__
#define __LGCAMERASOLUTION_H__

// System dependencies

#include <stddef.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include "camera_hal_if.h"
#include <string>

#define SOLUTION_AUTOCONTRAST         "AutoContrast"
#define SOLUTION_DUMMY                "Dummy"
#define SOLUTION_OPENCV_FACEDETECTION "OCVFaceDetection"
#define SOLUTION_AIF_FACEDETECTION    "AIFFaceDetection"

typedef struct
{
    camera_pixel_format_t pixel_format;
    unsigned int stream_width;
    unsigned int stream_height;
    unsigned int buffer_size;
    void* data;
} Src_frame_data_t;

typedef enum {
    LG_SOLUTION_PREVIEW         = 0x0001,
    LG_SOLUTION_VIDEO           = 0x0002,
    LG_SOLUTION_SNAPSHOT        = 0x0004
}LG_Solution_Property;

typedef enum {
    SOLUTION_MANAGER_NO_ERROR = 0,
    SOLUTION_MANAGER_NO_SUPPORTED_SOLUTION,
    SOLUTION_MANAGER_NO_ENABLED_SOLUTION,
    SOLUTION_MANAGER_PARAMETER_ERROR,
    SOLUTION_MANAGER_PARSING_ERROR,
    SOLUTION_MANAGER_MAX_ERROR_INDEX,
}LgSolutionErrorValue;

class CameraSolutionManager;

class CameraSolution {
    public:
        CameraSolution(CameraSolutionManager *mgr);
        virtual ~CameraSolution();

        // interface for snapshot
        virtual void initialize(stream_format_t streamformat) = 0;
        virtual std::string getSolutionStr() = 0;
        virtual void processForSnapshot(buffer_t inBuf,        stream_format_t streamformat) = 0;
        virtual void processForPreview(buffer_t inBuf, stream_format_t streamformat) = 0;
        virtual void release() = 0;

        bool isEnabled(){return mEnableStatus;};
        void setEnableValue(bool enableValue){        mEnableStatus = enableValue;};

        CameraSolutionManager *m_manager;
        int32_t solutionProperty;
        bool mEnableStatus;

    private:

};

#endif //__LGCAMERASOLUTION_H__
