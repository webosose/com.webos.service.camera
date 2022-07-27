/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionFaceDetection.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  OpenCV FaceDetection
 *
 */

#ifndef _FACE_DETECT_H_
#define _FACE_DETECT_H_

#include "camera_solution_async.h"
#include "camera_solution_manager.h"

#include "camera_types.h"
#include <opencv2/highgui.hpp>
#include <opencv2/opencv.hpp>
//#include "posixshm.h"
#include <mutex>
#include <thread>

using namespace cv;

class FaceDetectionOpenCV : public CameraSolutionAsync
{
public:
    FaceDetectionOpenCV(CameraSolutionManager *mgr);
    virtual ~FaceDetectionOpenCV();
    std::string getSolutionStr();
    void processForSnapshot(buffer_t inBuf);
    void processForPreview(buffer_t inBuf);
    void release();

    bool needThread() { return true; };
    void startThread(stream_format_t streamformat);

private:
    pthread_mutex_t m_fd_lock;
    pthread_cond_t m_fd_cond;
    std::mutex m;

    Src_frame_data_t srcBuf;

    bool isThreadRunning;
    bool needInputRefresh;
    bool processingDone;

    // cascadeclassifier Ŭ����
    CascadeClassifier face_classifier;
    std::thread tidDetect;
    std::vector<Rect> mFaces;
    bool mDone;

    Scalar green, red;

    bool mSupportStatus = false;
    bool mEnableStatus  = false;

    unsigned char *internalBuffer;

    void print();
    int getFaceRectangle(void *inBuf, int len, stream_format_t, std::string);
    void Start(int shm_key, stream_format_t format);
    void stopThread();
    void GetFaces(std::vector<Rect> &);
    int Draw(buffer_t srcBuf, stream_format_t streamformat);
    // int  DetectFaces(buffer_t&, std::string);
    void SetFormat(stream_format_t);
    void faceDetectionProcessing();
    int dumpFrame(unsigned char *inputY, int width, int height, int frameSize, char *filename,
                  char *filepath);
};
#endif
