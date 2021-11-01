/**
 * Copyright(c) 2015 by LG Electronics Inc.
 * Mobile Communication Company, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGFilmEmulator.cpp
 * @contact     camera-architect@lge.com
 *
 * Description  FilmEmulator
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
        : CameraSolution(mgr)
{
    PMLOG_INFO(CONST_MODULE_FD, "%s", __func__);

    //얼굴 인식 xml 로딩
    face_classifier.load("/usr/share/opencv4/haarcascades/haarcascade_frontalface_default.xml");
    mDone = false;
    hShm = NULL;
    isProcessing = false;
    readyForBuffer = false;
    isThreadRunning = false;
    processingDone = false;
}

LGCameraSolutionFaceDetection::~LGCameraSolutionFaceDetection()
{
    PMLOG_INFO(CONST_MODULE_FD, "%s", __func__);

}

void LGCameraSolutionFaceDetection::initialize(stream_format_t streamformat, int key) {
    PMLOG_INFO(CONST_MODULE_FD, "%s : E\n", __func__);
    if(needThread()
        && isThreadRunning == false){
        startThread(streamformat, key);
    }
}

std::string LGCameraSolutionFaceDetection::getSolutionStr(){
    std::string solutionStr = "FaceDetection";
    return solutionStr;
}

void LGCameraSolutionFaceDetection::processForSnapshot(void* inBuf,        stream_format_t streamformat)
{

}

void LGCameraSolutionFaceDetection::processForPreview(void* inBuf,        stream_format_t streamformat)
{
    Draw(inBuf, streamformat);
}

void LGCameraSolutionFaceDetection::startThread(stream_format_t streamformat, int key)
{
    PMLOG_INFO(CONST_MODULE_FD, "[%s] key : %d\n", __func__, key);

    mFormat = streamformat;

    SHMEM_STATUS_T ret = IPCSharedMemory::getInstance().OpenShmem(&hShm, key);
    if( ret != SHMEM_COMM_OK)
    {
        PMLOG_INFO(CONST_MODULE_FD, "Fail : OpenShmem RET => %d\n", ret);
        return ;
    }

    tidDetect = std::thread( &LGCameraSolutionFaceDetection::faceDetectionProcessing, this);
    isThreadRunning = true;
}

#if 0
int LGCameraSolutionFaceDetection::dumpFrame(unsigned char* inputY, unsigned char* inputUV, int width, int height, int stride, int frameSize, char* filename, char* filepath)
{
    PMLOG_INFO(CONST_MODULE_FD, "%s : E",__func__);

    char buf[128];
    time_t now = time(NULL);
    tm *pnow = localtime(&now);
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    if(pnow == NULL){
        printf("%s : getting time is failed so do not dump",__func__);
        return false;
    }

    snprintf(buf, 128, "%s/%d_%d_%d_%d_%s_%dx%d.yuv", filepath, pnow->tm_hour, pnow->tm_min, pnow->tm_sec,((int)tmnow.tv_usec) / 1000, filename, width, height);
    PMLOG_INFO(CONST_MODULE_FD, "%s: path( %s )\n", __func__, buf);

    FILE* file_fd = fopen(buf, "wb");
    if (file_fd == 0)
    {
        PMLOG_INFO(CONST_MODULE_FD, "%s: cannot open file\n", __func__);
        return false;
    }
    else
    {
        fwrite(((unsigned char *)inputY), 1, frameSize, file_fd);
    }
    fclose(file_fd);

    PMLOG_INFO(CONST_MODULE_FD, "%s : X",__func__);
    return true;

}
#endif

void LGCameraSolutionFaceDetection::release() {
    PMLOG_INFO(CONST_MODULE_FD, "%s : E\n", __func__);
    stopThread();
}

void LGCameraSolutionFaceDetection::stopThread()
{
    PMLOG_INFO(CONST_MODULE_FD, "%s", __func__);

    mDone = true;

    if(tidDetect.joinable()) {
        tidDetect.join();
        isThreadRunning = false;
    }

    if(hShm)
    {
        SHMEM_STATUS_T ret = IPCSharedMemory::getInstance().CloseShmemory(&hShm);
        if (ret != SHMEM_COMM_OK)
            PMLOG_ERROR(CONST_MODULE_FD, "CloseShmemory error %d \n", ret);
    }
}

void LGCameraSolutionFaceDetection::faceDetectionProcessing()
{
    PMLOG_INFO(CONST_MODULE_FD, "%s", __func__);

    if(hShm == NULL) {
        PMLOG_INFO(CONST_MODULE_FD, "shared memory is null");
        return;
    }

    while(!mDone)
    {
        try {

            PMLOG_INFO(CONST_MODULE_FD, "%s start", __func__);
            int len = 0;
            Mat rawData, frame_original, frame_small, grayframe;
            unsigned char *sh_mem_addr = NULL;

    	    IPCSharedMemory::getInstance().ReadShmem(hShm, &sh_mem_addr, &len);
            if(len == 0) continue;

            if(mFormat.pixel_format == CAMERA_PIXEL_FORMAT_JPEG)
            {
                rawData = Mat(1, len, CV_8UC1, (void*)sh_mem_addr);
                frame_original = imdecode(rawData, IMREAD_COLOR);
                resize(frame_original, frame_small, Size(320, 240));
                //gray scale로 변환
                cvtColor(frame_small, grayframe, COLOR_BGR2GRAY);
            }
            else if(mFormat.pixel_format == CAMERA_PIXEL_FORMAT_YUYV)
            {
                rawData = Mat(Size(640, 480), CV_8UC2, (void*)sh_mem_addr);
                cvtColor(rawData, grayframe, COLOR_YUV2GRAY_YUYV);
                resize(grayframe, grayframe, Size(320, 240));
            }

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

int LGCameraSolutionFaceDetection::Draw(void* srcBuf, stream_format_t streamformat)
{
    PMLOG_INFO(CONST_MODULE_FD, "Draw face S");

    double rx, ry;

    rx = mFormat.stream_width/320;
    ry = mFormat.stream_height/240;

    void* inBuf = srcBuf;
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

int LGCameraSolutionFaceDetection::DetectFaces(buffer_t& frameBuf, std::string text)
{
    void* inBuf = frameBuf.start;
    int len = frameBuf.length;

	try{
		Mat frame_original;
        Mat frame_small, grayframe;

        if(pixel_format == CAMERA_PIXEL_FORMAT_JPEG)
        {
            Mat rawData = Mat(1, len, CV_8UC1, (void*)inBuf);
            frame_original = imdecode(rawData, IMREAD_COLOR);
            resize(frame_original, frame_small, Size(320, 240));
            cvtColor(frame_small, grayframe, COLOR_BGR2GRAY);
            rawData.release();
        }
        else if(pixel_format == CAMERA_PIXEL_FORMAT_YUYV)
        {
            frame_original = Mat(Size(width, height), CV_8UC2, (void*)inBuf);
            cvtColor(frame_original, grayframe, COLOR_YUV2GRAY_YUYV);
            resize(grayframe, grayframe, Size(320, 240));
        }

		equalizeHist(grayframe, grayframe);

		//이미지 표시용 변수
		std::vector<Rect> faces;

        //얼굴의 위치와 영역을 탐색한다.
		face_classifier.detectMultiScale(grayframe, faces,
			1.1,
			3,
			0,//CV_HAAR_FIND_BIGGEST_OBJECT | CV_HAAR_SCALE_IMAGE,
			Size(30, 30));

		for(int i=0;i<faces.size();i++){
			Point lb(faces[i].x *rx  + faces[i].width *rx ,
				faces[i].y*ry  + faces[i].height*ry );
			Point tr(faces[i].x*rx , faces[i].y*ry );
            PMLOG_INFO(CONST_MODULE_FD, "%s face[%d] (%d %d) %dx%d", __func__, i, faces[i].x, faces[i].y, faces[i].width, faces[i].height);
            rectangle(frame_original, lb, tr, green, 2, 4, 0);
		}

        cv::Point myPoint;
        myPoint.x = 10;
        myPoint.y = 40;

        /// Font Face
        int myFontFace = 2 ;

        /// Font Scale
        double myFontScale = 1.0;
        putText(frame_original, text, myPoint, myFontFace, myFontScale, red);

        //imshow("Face", frame_original);
        if(pixel_format == CAMERA_PIXEL_FORMAT_JPEG)
        {
            std::vector<uchar> buff;//buffer for coding
            std::vector<int> param = std::vector<int>(2);
            param[0] = IMWRITE_JPEG_QUALITY;
            param[1] = 95;//default(95) 0-100
            imencode(".jpg", frame_original, buff, param);
            memcpy(inBuf, buff.data(), buff.size());
            frameBuf.length - buff.size();

        }
        else if(pixel_format == CAMERA_PIXEL_FORMAT_YUYV)
        {
            memcpy(inBuf, frame_original.data, len);
        }
	}catch(Exception& e){
		PMLOG_INFO(CONST_MODULE_FD, "Exception occurred. face");
	}

    return 0;
}

void LGCameraSolutionFaceDetection::SetFormat(stream_format_t format)
{
    mFormat = format;

    width = format.stream_width;
    height = format.stream_height;
    rx = width/320;
    ry = height/240;
    pixel_format = format.pixel_format;

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

