/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionAIFFaceDetection.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  AI-Framework FaceDetection
 *
 */

#ifndef _AIF_FACE_DETECT_H_
#define _AIF_FACE_DETECT_H_

#include "../CameraSolution.h"
#include "../CameraSolutionManager.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "camera_types.h"
#include <thread>
#include <mutex>

#include <aif/base/DetectorFactory.h>
#include <aif/base/Detector.h>
#include <aif/base/Descriptor.h>
#include <aif/face/FaceDescriptor.h>
#include <aif/tools/Utils.h>
#include <aif/log/Logger.h>

using namespace cv;

#define LOG_TAG "LGCameraSolutionAIFFaceDetection"

class LGCameraSolutionAIFFaceDetection : public CameraSolution {
public:
    LGCameraSolutionAIFFaceDetection(CameraSolutionManager *mgr);
    virtual ~LGCameraSolutionAIFFaceDetection();
    void initialize(stream_format_t streamformat);
    std::string getSolutionStr();
    void processForSnapshot(buffer_t inBuf,        stream_format_t streamformat);
    void processForPreview(buffer_t inBuf, stream_format_t streamformat);
    void release();

    bool needThread(){return true;};
    void startThread(stream_format_t streamformat);

private:

    pthread_mutex_t m_fd_lock;
    pthread_cond_t  m_fd_cond;
    Src_frame_data_t srcBuf;
    std::shared_ptr<aif::Detector> mFaceDetector;
    std::shared_ptr<aif::Descriptor> descriptor;

    bool isThreadRunning;
    bool needInputRefresh;
    bool isInitialized;

    std::thread tidDetect;
    std::vector<Rect> mFaces;
    std::vector<double> mScore;
    std::vector<Rect> faceRects;
    bool mDone;
    //SHMEM_HANDLE hShm;

    Scalar green, red;

    bool mSupportStatus = false;
    bool mEnableStatus = false;

    unsigned char* internalBuffer;

    void print();
    int getFaceRectangle(void* inBuf, int len, stream_format_t, std::string);
    void Start(int shm_key, stream_format_t format);
    void stopThread();
    void GetFaces(std::vector<Rect>& dst);
    int  Draw(void* srcBuf, unsigned int buffer_size);
    //int  DetectFaces(buffer_t&, std::string);
    void SetFormat(stream_format_t);
    void faceDetectionProcessing();
    int dumpFrame(unsigned char* inputY, int width, int height, int frameSize, char* filename, char* filepath);

    bool InitFaceDetector();

};
#endif

