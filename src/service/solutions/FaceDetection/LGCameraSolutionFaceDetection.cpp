/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionFaceDetection.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  OpenCV FaceDetection
 *
 */
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/time.h>

#include "LGCameraSolutionFaceDetection.h"

#define CONST_MODULE_FD "FaceDetector"

LGCameraSolutionFaceDetection::LGCameraSolutionFaceDetection(CameraSolutionManager *mgr)
        : CameraSolution(mgr),
        mDone(false),
        isThreadRunning(false),
        processingDone(false),
        needInputRefresh(false),
        internalBuffer(NULL)

{
    PMLOG_INFO(CONST_MODULE_FD, "%s", __func__);

    srcBuf.pixel_format = CAMERA_PIXEL_FORMAT_JPEG;
    srcBuf.stream_height = srcBuf.stream_width = srcBuf.buffer_size = 0;
    srcBuf.data = NULL;
    solutionProperty = LG_SOLUTION_PREVIEW;
    pthread_mutex_init(&m_fd_lock, NULL);
    pthread_cond_init(&m_fd_cond, NULL);

}

LGCameraSolutionFaceDetection::~LGCameraSolutionFaceDetection()
{
    PMLOG_INFO(CONST_MODULE_FD, "%s", __func__);
    setEnableValue(false);
    release();
}

void LGCameraSolutionFaceDetection::initialize(stream_format_t streamformat)
{
    PMLOG_INFO(CONST_MODULE_FD, "%s : E\n", __func__);
}

std::string LGCameraSolutionFaceDetection::getSolutionStr(){
    std::string solutionStr = SOLUTION_OPENCV_FACEDETECTION;
    return solutionStr;
}

void LGCameraSolutionFaceDetection::processForSnapshot(buffer_t inBuf,        stream_format_t streamformat)
{

}

void LGCameraSolutionFaceDetection::processForPreview(buffer_t inBuf,        stream_format_t streamformat)
{

    if(needInputRefresh == true)
    {
        PMLOG_INFO(CONST_MODULE_FD, "%s : input refresh started inBuf.length(%d)\n", __func__,inBuf.length);
        if(internalBuffer == NULL)
        {
            int framesize =
                streamformat.stream_width * streamformat.stream_height * 4 + 1024;
            internalBuffer = (unsigned char*)malloc(framesize);

            if(internalBuffer == NULL)
            {
                PMLOG_ERROR(CONST_MODULE_FD, "%s : alloc failed so return\n", __func__);
                return;
            }

        }

        memcpy(internalBuffer, inBuf.start, (int)inBuf.length);

        srcBuf.pixel_format = streamformat.pixel_format;
        srcBuf.stream_height = streamformat.stream_height;
        srcBuf.stream_width = streamformat.stream_width;
        srcBuf.buffer_size = inBuf.length;
        srcBuf.data = internalBuffer;

        PMLOG_INFO(CONST_MODULE_FD, "%s : input refresh finished\n", __func__);

        pthread_mutex_lock(&m_fd_lock);
        pthread_cond_signal(&m_fd_cond);
        pthread_mutex_unlock(&m_fd_lock);

    }

    PMLOG_INFO(CONST_MODULE_FD, "%s : srcBuf.pixel_format(%d)\n", __func__,srcBuf.pixel_format);
    PMLOG_INFO(CONST_MODULE_FD, "%s : srcBuf.stream_height(%d)\n", __func__,srcBuf.stream_height);
    PMLOG_INFO(CONST_MODULE_FD, "%s : srcBuf.stream_width(%d)\n", __func__,srcBuf.stream_width);
    PMLOG_INFO(CONST_MODULE_FD, "%s : srcBuf.buffer_size(%d)\n", __func__,srcBuf.buffer_size);
    PMLOG_INFO(CONST_MODULE_FD, "%s : srcBuf.data(%p)\n", __func__,srcBuf.data);
    PMLOG_INFO(CONST_MODULE_FD, "%s : isThreadRunning(%d)\n", __func__,isThreadRunning);
    if(needThread()
        && isThreadRunning == false){
        startThread(streamformat);
    }

    //Draw(inBuf, streamformat);
}

void LGCameraSolutionFaceDetection::startThread(stream_format_t streamformat)
{
    PMLOG_INFO(CONST_MODULE_FD, "[%s] \n", __func__);

    tidDetect = std::thread( &LGCameraSolutionFaceDetection::faceDetectionProcessing, this);
    isThreadRunning = true;
}

void LGCameraSolutionFaceDetection::release() {
    PMLOG_INFO(CONST_MODULE_FD, "%s : E\n", __func__);

    pthread_mutex_destroy(&m_fd_lock);
    pthread_cond_destroy(&m_fd_cond);

    if(isThreadRunning)
    {
        stopThread();
    }

    if(internalBuffer != NULL)
    {
        free(internalBuffer);
        internalBuffer = NULL;
    }
}

void LGCameraSolutionFaceDetection::stopThread()
{
    PMLOG_INFO(CONST_MODULE_FD, "%s", __func__);

    mDone = true;

    if(tidDetect.joinable()) {
        tidDetect.join();
        isThreadRunning = false;
    }

}

void LGCameraSolutionFaceDetection::faceDetectionProcessing()
{
    PMLOG_INFO(CONST_MODULE_FD, "%s", __func__);
    //얼굴 인식 xml 로딩
    face_classifier.load("/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml");

    time_t now = time(NULL);
    tm *pnow = localtime(&now);
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    int checkTimeBefore = (int)tmnow.tv_usec;
    int checkTimeAfter = 0;


    while(!mDone && isEnabled())
    {

        try {

            needInputRefresh = true;

            PMLOG_INFO(CONST_MODULE_FD, "%s : wait to get new input S\n", __func__);

            int rc = pthread_mutex_lock(&m_fd_lock);
            rc += pthread_cond_wait(&m_fd_cond, &m_fd_lock);
            rc += pthread_mutex_unlock(&m_fd_lock);
            PMLOG_INFO(CONST_MODULE_FD, "%s : wait to get new input E\n", __func__);

            needInputRefresh = false;

            if(rc != SOLUTION_MANAGER_NO_ERROR)
            {
                PMLOG_ERROR(CONST_MODULE_FD, "%s getting input is failed so return", __func__);
                continue;
            }

            if(srcBuf.data == NULL)
            {
                PMLOG_ERROR(CONST_MODULE_FD, "%s srcBuf is null so do nothing", __func__);
                continue;
            }

            int inputSize = srcBuf.buffer_size;
            Mat rawData, frame_original, frame_small, grayframe;

#ifdef DUMP_ENABLED
            char filename[30];
            char filepath[100];
            snprintf(filename,sizeof(filename), "FD_Input");
            snprintf(filepath,sizeof(filepath), "/var/rootdirs/home/root/fd_dump");
            dumpFrame(internalBuffer, srcBuf.stream_width, srcBuf.stream_height, inputSize, filename, filepath);
#endif

            gettimeofday(&tmnow, NULL);
            checkTimeBefore = (int)tmnow.tv_usec;

            PMLOG_INFO(CONST_MODULE_FD, "%s : srcBuf.pixel_format(%d)", __func__,srcBuf.pixel_format);

            if(srcBuf.pixel_format == CAMERA_PIXEL_FORMAT_JPEG)
            {
                rawData = Mat(1, inputSize, CV_8UC1, (void*)internalBuffer);
                frame_original = imdecode(rawData, IMREAD_COLOR);
                resize(frame_original, frame_small, Size(320, 240));
                //gray scale로 변환
                cvtColor(frame_small, grayframe, COLOR_BGR2GRAY);
            }
            else if(srcBuf.pixel_format == CAMERA_PIXEL_FORMAT_YUYV)
            {
                rawData = Mat(Size(640, 480), CV_8UC2, (void*)internalBuffer);
                cvtColor(rawData, grayframe, COLOR_YUV2GRAY_YUYV);
                resize(grayframe, grayframe, Size(320, 240));
            }
            else
            {
                PMLOG_ERROR(CONST_MODULE_FD, "%s not supported format", __func__);
                continue;
            }

            gettimeofday(&tmnow, NULL);
            checkTimeAfter = (int)tmnow.tv_usec;
            PMLOG_INFO(CONST_MODULE_FD, "%s : Data converting time(%d)", __func__,checkTimeAfter - checkTimeBefore);

            rawData.release();

            //histogram 얻기
            equalizeHist(grayframe, grayframe);

            //이미지 표시용 변수
            std::vector<Rect> faces;
            //얼굴의 위치와 영역을 탐색한다.
            face_classifier.detectMultiScale(grayframe, faces,
            1.1,
            3,
            0,//CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_SCALE_IMAGE,
            Size(30, 30));

            PMLOG_INFO(CONST_MODULE_FD, "%s faces(%d)", __func__,faces.size());

            {
                std::lock_guard<std::mutex> lg(m);
                mFaces = faces;
                print();
            }
            PMLOG_INFO(CONST_MODULE_FD, "%s end", __func__);

        } catch(Exception& e) {
            PMLOG_INFO(CONST_MODULE_FD, "Exception occurred. face : %s", e.what());
                break;
        }
    }

    processingDone = true;
    PMLOG_INFO(CONST_MODULE_FD, "~%s", __func__);
}

void LGCameraSolutionFaceDetection::GetFaces(std::vector<Rect>& dst)
{
    std::lock_guard<std::mutex> lg(m);
    dst = mFaces;
}

void LGCameraSolutionFaceDetection::print()
{
    for(int i = 0; i < mFaces.size(); i++){
        PMLOG_INFO(CONST_MODULE_FD, "print face[%d] (%d %d) %dx%d", i, mFaces[i].x, mFaces[i].y, mFaces[i].width, mFaces[i].height);
    }

    int cx, cy;

    if(mFaces.size() > 0) {
        cx = mFaces[0].x + mFaces[0].width / 2;
        cy = mFaces[0].y + mFaces[0].height / 2;
    } else
        cx = cy = 0;

    FILE *fp = fopen("/home/root/fd_test.txt", "w");
    fprintf(fp,"%dx%d\n", cx, cy);
    fclose(fp);
}

int LGCameraSolutionFaceDetection::Draw(buffer_t srcframe, stream_format_t streamformat)
{
    PMLOG_INFO(CONST_MODULE_FD, "Draw face S");

    double rx, ry;
    int width = srcBuf.stream_width;
    int height = srcBuf.stream_height;

    void* inBuf = srcframe.start;
    int len = streamformat.buffer_size;

    try{
        Mat rawData, frame_original;
        Mat img_small;
        rawData = Mat(1, len, CV_8UC1, (void*)inBuf);
        frame_original = imdecode(rawData, IMREAD_COLOR );
        rawData.release();

        std::vector<Rect> faces;
        GetFaces(faces);

        for(int i=0;i<faces.size();i++){
            Point lb(faces[i].x *rx  + faces[i].width *rx ,
            	faces[i].y*ry  + faces[i].height*ry );
            Point tr(faces[i].x*rx , faces[i].y*ry );

            PMLOG_INFO(CONST_MODULE_FD, "Draw face[%d] %d %d %d %d", i, tr.x, tr.y, lb.x - tr.x, lb.y - tr.y);
            rectangle(frame_original, lb, tr, Scalar(0, 255, 0), 2, 4, 0);
        }

        //imshow("Face", frame_original);
        std::vector<uchar> buff;//buffer for coding
        std::vector<int> param = std::vector<int>(2);
        param[0] = IMWRITE_JPEG_QUALITY;
        param[1] = 95;//default(95) 0-100
        imencode(".jpg", frame_original, buff, param);
        memcpy(inBuf, buff.data(), buff.size());
        len - buff.size();
    }catch(Exception& e){
        PMLOG_INFO(CONST_MODULE_FD, "Exception occurred. face");
    }
    PMLOG_INFO(CONST_MODULE_FD, "Draw face E");

    return 0;
}

void LGCameraSolutionFaceDetection::SetFormat(stream_format_t format)
{

    int width = format.stream_width;
    int height = format.stream_height;
    double rx = width/320;
    double ry = height/240;
    camera_pixel_format_t pixel_format = format.pixel_format;

    if(pixel_format == CAMERA_PIXEL_FORMAT_JPEG)
    {
        green = Scalar(0, 255, 0);
        red   = Scalar(0, 0, 255);
    }
    else if(pixel_format == CAMERA_PIXEL_FORMAT_YUYV)
    {
        green = Scalar(149, 43, 21);
        red   = Scalar(107, 255, 29); //TODO : need to check
    }
}



int LGCameraSolutionFaceDetection::dumpFrame(unsigned char* inputY, int width, int height, int frameSize, char* filename, char* filepath)
{
    PMLOG_INFO(CONST_MODULE_SM,  " E\n");

    char buf[128];
    time_t now = time(NULL);
    tm *pnow = localtime(&now);
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    if(pnow == NULL){
        PMLOG_INFO(CONST_MODULE_SM,"%s : getting time is failed so do not dump",__func__);
        return false;
    }

    snprintf(buf, 128, "%s/%d_%d_%d_%d_%s_%dx%d.yuv", filepath, pnow->tm_hour, pnow->tm_min, pnow->tm_sec,((int)tmnow.tv_usec) / 1000, filename, width, height);
    PMLOG_INFO(CONST_MODULE_SM, "%s: path( %s )\n", __func__, buf);

    FILE* file_fd = fopen(buf, "wb");
    if (file_fd == 0)
    {
        PMLOG_INFO(CONST_MODULE_SM, "%s: cannot open file\n", __func__);
        return false;
    }
    else
    {
        fwrite(((unsigned char *)inputY), 1, frameSize, file_fd);
    }
    fclose(file_fd);

    PMLOG_INFO(CONST_MODULE_SM, " X",__func__);
    return true;

}

