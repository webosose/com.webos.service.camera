/**
 * Copyright(c) 2021 by LG Electronics Inc.
 * Mobile Communication Company, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionFaceDetection.cpp
 * @contact     kwanghee.choi@lge.com
 *
 * Description  FaceDetection
 *
 */

#ifndef _FACE_DETECT_H_
#define _FACE_DETECT_H_

#include "../CameraSolution.h"
#include "../CameraSolutionCommon.h"
#include "../CameraSolutionManager.h"

#include <opencv2/opencv.hpp>
#include <opencv2/highgui.hpp>
#include "camera_types.h"
#include "camshm.h"
#include <thread>
#include <mutex>

using namespace cv;

#define LOG_TAG "LGCameraSolutionFaceDetection"

class LGCameraSolutionFaceDetection : public CameraSolution {
public:
    bool mSupportStatus = false;
    bool mEnableStatus = false;

    LGCameraSolutionFaceDetection(CameraSolutionManager *mgr);
    virtual ~LGCameraSolutionFaceDetection();
    void initialize(stream_format_t streamformat, int key);
    bool isSupported(){return mSupportStatus;};
    bool isEnabled(){return mEnableStatus;};
    void setSupportStatus(bool supportStatus){mSupportStatus = supportStatus;};
    void setEnableValue(bool enableValue){
        printf("Solution Auto Contrast setEnableValue %d\n",enableValue);
        mEnableStatus = enableValue;};

    bool needThread(){return true;};
    void startThread();
    std::string getSolutionStr();
    void processForSnapshot(void* inBuf,        stream_format_t streamformat);
    void processForPreview(void* inBuf, stream_format_t streamformat);
    void release();


private:

    void *handle;
    bool mIsDumpEnabled;
    bool mIsSimulationEnabled;
    // isOutdoor;
    int input_num;
    bool isLogEnabled;
    bool isProcessing;
    bool readyForBuffer;
    bool isThreadRunning;
    bool processingDone;

    //cascadeclassifier Ŭ����
    CascadeClassifier face_classifier;
    std::thread tidDetect;
    std::mutex m;
    std::vector<Rect> mFaces;
    bool mDone;
    SHMEM_HANDLE hShm;
    stream_format_t mFormat;

    camera_pixel_format_t pixel_format;
    int width, height;
    double rx, ry;
    Scalar green, red;

    void print();
    int getFaceRectangle(void* inBuf, int len, stream_format_t, std::string);
    void Start(int shm_key, stream_format_t format);
    void startThread(stream_format_t streamformat, int key);
    void stopThread();
    void GetFaces(std::vector<Rect>&);
    int  Draw(void* srcBuf, stream_format_t streamformat);
    int  DetectFaces(buffer_t&, std::string);
    void SetFormat(stream_format_t);
    void faceDetectionProcessing();


};
#endif

