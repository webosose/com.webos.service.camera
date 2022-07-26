/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    LGCameraSolutionAIFFaceDetection.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  AI-Framework FaceDetection
 *
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/time.h>
#include <functional>
#include "LGCameraSolutionAIFFaceDetection.h"
#include <aif/face/CpuFaceDetector.h>

#include <rapidjson/document.h>

#define RAPIDJSON_HAS_STDSTRING 1

namespace rj = rapidjson;

#define CONST_MODULE_NAME "LGCameraSolutionAIFFaceDetection"
//#define DUMP_ENABLED
LGCameraSolutionAIFFaceDetection::LGCameraSolutionAIFFaceDetection(CameraSolutionManager *mgr)
        : CameraSolution(mgr),
          mFaceDetector(nullptr),
          mDone(false),
          isThreadRunning(false),
          needInputRefresh(false),
          isInitialized(false),
          internalBuffer(NULL)
{
    PMLOG_INFO(CONST_MODULE_NAME, " E\n");

    srcBuf.pixel_format = CAMERA_PIXEL_FORMAT_JPEG;
    srcBuf.stream_height = srcBuf.stream_width = srcBuf.buffer_size = 0;
    srcBuf.data = NULL;
    solutionProperty = LG_SOLUTION_PREVIEW;
    pthread_mutex_init(&m_fd_lock, NULL);
    pthread_cond_init(&m_fd_cond, NULL);

}

LGCameraSolutionAIFFaceDetection::~LGCameraSolutionAIFFaceDetection()
{
    PMLOG_INFO(CONST_MODULE_NAME, " E\n");
    setEnableValue(false);
    release();

}

void LGCameraSolutionAIFFaceDetection::initialize(stream_format_t streamformat)
{
    PMLOG_INFO(CONST_MODULE_NAME, " E\n");
}

std::string LGCameraSolutionAIFFaceDetection::getSolutionStr()
{
    std::string solutionStr = SOLUTION_AIF_FACEDETECTION;
    return solutionStr;
}

void LGCameraSolutionAIFFaceDetection::processForSnapshot(buffer_t inBuf,        stream_format_t streamformat)
{

}

void LGCameraSolutionAIFFaceDetection::processForPreview(buffer_t inBuf,        stream_format_t streamformat)
{
    PMLOG_INFO(CONST_MODULE_NAME, " E\n");

    if(needInputRefresh == true)
    {
        PMLOG_INFO(CONST_MODULE_NAME, " input refresh started inBuf.length(%d)\n",inBuf.length);
        if(internalBuffer == NULL)
        {
            int framesize =
                streamformat.stream_width * streamformat.stream_height * 4 + 1024;
            internalBuffer = (unsigned char*)malloc(framesize);

            if(internalBuffer == NULL)
            {
                PMLOG_ERROR(CONST_MODULE_NAME, " alloc failed so return\n");
                return;
            }

        }

        memcpy(internalBuffer, inBuf.start, (int)inBuf.length);

        srcBuf.pixel_format = streamformat.pixel_format;
        srcBuf.stream_height = streamformat.stream_height;
        srcBuf.stream_width = streamformat.stream_width;
        srcBuf.buffer_size = inBuf.length;
        srcBuf.data = internalBuffer;

        PMLOG_INFO(CONST_MODULE_NAME, " input refresh finished\n");

        pthread_mutex_lock(&m_fd_lock);
        pthread_cond_signal(&m_fd_cond);
        pthread_mutex_unlock(&m_fd_lock);

    }

    PMLOG_DEBUG(CONST_MODULE_NAME, " srcBuf.pixel_format(%d)\n", srcBuf.pixel_format);
    PMLOG_DEBUG(CONST_MODULE_NAME, " srcBuf.stream_height(%d)\n", srcBuf.stream_height);
    PMLOG_DEBUG(CONST_MODULE_NAME, " srcBuf.stream_width(%d)\n", srcBuf.stream_width);
    PMLOG_DEBUG(CONST_MODULE_NAME, " srcBuf.buffer_size(%d)\n", srcBuf.buffer_size);
    PMLOG_DEBUG(CONST_MODULE_NAME, " srcBuf.data(%p)\n", srcBuf.data);
    PMLOG_DEBUG(CONST_MODULE_NAME, " isThreadRunning(%d)\n", isThreadRunning);

    if(needThread()
        && isThreadRunning == false)
    {
        startThread(streamformat);
    }

    //Draw(srcBuf.data, srcBuf.buffer_size);

    PMLOG_INFO(CONST_MODULE_NAME, " X\n");
}

void LGCameraSolutionAIFFaceDetection::startThread(stream_format_t streamformat)
{
    PMLOG_INFO(CONST_MODULE_NAME, " E\n");


    tidDetect = std::thread( &LGCameraSolutionAIFFaceDetection::faceDetectionProcessing, this);
    isThreadRunning = true;
}

void LGCameraSolutionAIFFaceDetection::release()
{
    PMLOG_INFO(CONST_MODULE_NAME, " E\n");

    if(isThreadRunning)
    {
        stopThread();
    }

    pthread_mutex_destroy(&m_fd_lock);
    pthread_cond_destroy(&m_fd_cond);

    if(internalBuffer != NULL)
    {
        free(internalBuffer);
        internalBuffer = NULL;
    }
}

void LGCameraSolutionAIFFaceDetection::stopThread()
{
    PMLOG_INFO(CONST_MODULE_NAME, " E\n");

    mDone = true;

    if(tidDetect.joinable()) {
        tidDetect.join();
    }

    PMLOG_INFO(CONST_MODULE_NAME, "isThreadRunning(%d)", isThreadRunning);
    isThreadRunning = false;

}

void LGCameraSolutionAIFFaceDetection::faceDetectionProcessing()
{
    PMLOG_INFO(CONST_MODULE_NAME, " E\n");

    time_t now = time(NULL);
    tm *pnow = localtime(&now);
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    int checkTimeBefore = (int)tmnow.tv_usec;
    int checkTimeAfter = 0;

    if(isInitialized == false)
    {
        InitFaceDetector();
    }

    while(!mDone && isEnabled())
    {
        try {

            needInputRefresh = true;

            PMLOG_INFO(CONST_MODULE_NAME, " wait to get new input S\n");

            int rc = pthread_mutex_lock(&m_fd_lock);
            rc += pthread_cond_wait(&m_fd_cond, &m_fd_lock);
            rc += pthread_mutex_unlock(&m_fd_lock);

            PMLOG_INFO(CONST_MODULE_NAME, " wait to get new input E\n");

            needInputRefresh = false;

            if(rc != SOLUTION_MANAGER_NO_ERROR)
            {
                PMLOG_ERROR(CONST_MODULE_NAME, " getting input is failed so return");
                continue;
            }

            if(srcBuf.data == NULL)
            {
                PMLOG_ERROR(CONST_MODULE_NAME, " srcBuf is null so do nothing");
                continue;
            }

            int inputSize = 0;
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

            PMLOG_INFO(CONST_MODULE_NAME, " srcBuf.pixel_format(%d)", srcBuf.pixel_format);
            if(srcBuf.pixel_format == CAMERA_PIXEL_FORMAT_JPEG)
            {
                Mat rawData = Mat(1, srcBuf.buffer_size, CV_8UC1, (void*)srcBuf.data);
                frame_original = imdecode(rawData, IMREAD_COLOR);
            }
            else if(srcBuf.pixel_format == CAMERA_PIXEL_FORMAT_YUYV)
            {
                rawData = Mat(Size(srcBuf.stream_width, srcBuf.stream_height), CV_8UC2, (void*)internalBuffer);
                frame_original = imdecode(rawData, IMREAD_COLOR);
                //cvtColor(frame_original, grayframe, COLOR_YUV2GRAY_YUYV);
                //resize(grayframe, grayframe, Size(320, 240));
            }
            else
            {
                PMLOG_INFO(CONST_MODULE_NAME, " not supported image format(%d)", srcBuf.pixel_format);
                return;
            }
            gettimeofday(&tmnow, NULL);
            checkTimeAfter = (int)tmnow.tv_usec;
            PMLOG_INFO(CONST_MODULE_NAME, " Data converting time(%d)", checkTimeAfter - checkTimeBefore);


            auto foundFaces = std::dynamic_pointer_cast<aif::FaceDescriptor>(descriptor);
            foundFaces->clear();

            gettimeofday(&tmnow, NULL);
            checkTimeBefore = (int)tmnow.tv_usec;

            mFaceDetector->detect(frame_original, descriptor);

            gettimeofday(&tmnow, NULL);
            checkTimeAfter = (int)tmnow.tv_usec;
            PMLOG_INFO(CONST_MODULE_NAME, " : foundFaces(%d)", foundFaces->size());
            PMLOG_INFO(CONST_MODULE_NAME, " : FD_ProcessingTime(%d)", checkTimeAfter - checkTimeBefore);
            /*
            {
                "faces": [
                    {
                        "score":0.9347445368766785,
                        "region":[0.37328392267227175,0.17393717169761659,0.5720989108085632,0.37275126576423647],
                        "lefteye":[0.3948841094970703,0.23510795831680299],
                        "righteye":[0.48589372634887698,0.2253197431564331],
                        "nosetip":[0.4289666712284088,0.27532318234443667],
                        "mouth":[0.44470444321632388,0.3167915940284729],
                        "leftear":[0.3805391788482666,0.2566477358341217],
                        "rightear":[0.5752524137496948,0.23307925462722779]
                    }
                ]
            }
            */
            //std::cout << descriptor->toStr() << std::endl;

            mScore.clear();
            faceRects.clear();

            rj::Document json;
            json.Parse(descriptor->toStr());

            const rj::Value& faces = json["faces"];
            for (rj::SizeType i = 0; i < faces.Size(); i++) {
                double xmin = faces[i]["region"][0].GetDouble();
                double ymin = faces[i]["region"][1].GetDouble();
                double xmax = faces[i]["region"][2].GetDouble();
                double ymax = faces[i]["region"][3].GetDouble();

                // FaceDetection 결과는 0~1사이로 정규화되므로 원래 사이즈를 이용하여 박스 좌표를 계산해야 한다.
                cv::Rect rect;
                rect.x = xmin * srcBuf.stream_width;
                rect.y = ymin * srcBuf.stream_height;
                rect.width = (xmax - xmin) * srcBuf.stream_width;
                rect.height = (ymax - ymin) * srcBuf.stream_height;
                faceRects.push_back(rect);

                double score = faces[i]["score"].GetDouble();
                mScore.push_back(score);

                PMLOG_INFO(CONST_MODULE_NAME, "rect.x : %d", rect.x);
                PMLOG_INFO(CONST_MODULE_NAME, "rect.y : %d", rect.y);
                PMLOG_INFO(CONST_MODULE_NAME, "rect.width : %d", rect.width);
                PMLOG_INFO(CONST_MODULE_NAME, "rect.height : %d", rect.height);
                PMLOG_INFO(CONST_MODULE_NAME, "score : %f", (float)score);
            }
            mFaces = faceRects;


        } catch(Exception& e) {
            PMLOG_INFO(CONST_MODULE_NAME, "Exception occurred. face : %s", e.what());
                break;
        }
    }

    PMLOG_INFO(CONST_MODULE_NAME, " X\n");
}

void LGCameraSolutionAIFFaceDetection::GetFaces(std::vector<Rect>& dst)
{
    dst = mFaces;
}

void LGCameraSolutionAIFFaceDetection::print()
{
    for(int i = 0; i < mFaces.size(); i++){
        PMLOG_INFO(CONST_MODULE_NAME, "print face[%d] (%d %d) %dx%d", i, mFaces[i].x, mFaces[i].y, mFaces[i].width, mFaces[i].height);
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

int LGCameraSolutionAIFFaceDetection::Draw(void* srcframe, unsigned int buffer_size)
{
    PMLOG_INFO(CONST_MODULE_NAME, "Draw face E");

    int width = srcBuf.stream_width;
    int height = srcBuf.stream_height;
    //rx = mFormat.stream_width/320;
    //ry = mFormat.stream_height/240;

    double rx = 1;
    double ry = 1;

    //void* inBuf = srcBuf;
    //int len = buffer_size;

    unsigned char* inBuf = NULL;
    int len = 0;

#if 1
    char filename[30];
    char filepath[100];
    snprintf(filename,sizeof(filename), "FD_Input");
    snprintf(filepath,sizeof(filepath), "/var/rootdirs/home/root/fd_dump");
    dumpFrame((unsigned char*)inBuf, width, height, buffer_size, filename, filepath);
    //memset(inputY, 0, width*3);
#endif

    try{
        Mat rawData, frame_original;
        Mat img_small;

        #if 0
        PMLOG_INFO(CONST_MODULE_NAME, " readShmem start\n");
        IPCSharedMemory::getInstance().ReadShmem(hShm, &inBuf, &len);
        if(len == 0) return 0;
        #endif

        rawData = Mat(1, len, CV_8UC1, (void*)inBuf);
        frame_original = imdecode(rawData, IMREAD_COLOR );
        rawData.release();

        std::vector<Rect> faces;
        GetFaces(faces);
        PMLOG_INFO(CONST_MODULE_NAME, "faces.size(%d)",faces.size());
        green = Scalar(0, 255, 0);
        red   = Scalar(255, 0, 0); //TODO : need to check

        for(int i=0;i<faces.size();i++){
            Scalar rec_color = (i==0)? green : Scalar(255,255,255);
            int thickness  = (i==0)? 4 : 4;
            Point lb(faces[i].x *rx  + faces[i].width *rx ,
            	faces[i].y*ry  + faces[i].height*ry );
            Point tr(faces[i].x*rx , faces[i].y*ry );

            PMLOG_INFO(CONST_MODULE_NAME, "Draw face[%d] %d %d %d %d", i, tr.x, tr.y, lb.x - tr.x, lb.y - tr.y);
            rectangle(frame_original, lb, tr, rec_color, thickness, 4, 0);

            //if(faces.size()>1)
            {
                cv::Point myPoint;
                myPoint.x = tr.x;
                myPoint.y = tr.y;

                /// Font Face
                int myFontFace = 2 ;

                /// Font Scale
                double myFontScale = 1.0;
#if 1
                if(!mScore.empty())
                {
                //putText(frame_original, std::to_string(i+1), myPoint, myFontFace, myFontScale, red);
                //std::string str = std::to_string (mScores[i]);
                //str.erase ( str.find_last_not_of('0') + 1, std::string::npos );
                char temp_str[8];
                sprintf(temp_str, "%.2f", mScore[i]);
                putText(frame_original, temp_str, myPoint, myFontFace, myFontScale, red);
                }
#endif
            }
        }

        //imshow("Face", frame_original);
        std::vector<uchar> buff;//buffer for coding
        std::vector<int> param = std::vector<int>(2);
        param[0] = IMWRITE_JPEG_QUALITY;
        param[1] = 95;//default(95) 0-100
        imencode(".jpg", frame_original, buff, param);

        //memcpy(inBuf, buff.data(), buff.size());

        memcpy(srcframe, buff.data(), buff.size());
        //frameBuf.length - buff.size();
        buffer_size = buff.size();
    }catch(Exception& e){
    	PMLOG_INFO(CONST_MODULE_NAME, "Exception occurred. face");
    }

#if 1
    filename[30];
    filepath[100];
    snprintf(filename,sizeof(filename), "FD_Output");
    snprintf(filepath,sizeof(filepath), "/var/rootdirs/home/root/fd_dump");
    dumpFrame((unsigned char*)inBuf, width, height, buffer_size, filename, filepath);
    //memset(inputY, 0, width*3);
#endif
    return 0;
}

bool LGCameraSolutionAIFFaceDetection::InitFaceDetector()
{
    PMLOG_INFO(CONST_MODULE_SM, " E\n");

    mFaceDetector = aif::DetectorFactory::get().getDetector("face_short_range_cpu");
    descriptor = std::make_shared<aif::FaceDescriptor>();

    if (mFaceDetector == nullptr) {
        PMLOG_INFO(CONST_MODULE_SM, " faceDetector create error!\n");
        return false;
    }

    if (mFaceDetector->init() != aif::kAifOk) {
        PMLOG_INFO(CONST_MODULE_SM, " faceDetector init error!\n");
        return false;
    }

    isInitialized = true;

    PMLOG_INFO(CONST_MODULE_SM, " X\n");

    return true;
}

void LGCameraSolutionAIFFaceDetection::SetFormat(stream_format_t format)
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

int LGCameraSolutionAIFFaceDetection::dumpFrame(unsigned char* inputY, int width, int height, int frameSize, char* filename, char* filepath)
{
    PMLOG_INFO(CONST_MODULE_SM,  " E\n");

    char buf[128];
    time_t now = time(NULL);
    tm *pnow = localtime(&now);
    struct timeval tmnow;
    gettimeofday(&tmnow, NULL);

    if(pnow == NULL){
        PMLOG_INFO(CONST_MODULE_SM," getting time is failed so do not dump");
        return false;
    }

    snprintf(buf, 128, "%s/%d_%d_%d_%d_%s_%dx%d.jpg", filepath, pnow->tm_hour, pnow->tm_min, pnow->tm_sec,((int)tmnow.tv_usec) / 1000, filename, width, height);
    PMLOG_INFO(CONST_MODULE_SM, " path( %s )\n", buf);

    FILE* file_fd = fopen(buf, "wb");
    if (file_fd == 0)
    {
        PMLOG_INFO(CONST_MODULE_SM, " cannot open file\n");
        return false;
    }
    else
    {
        fwrite(((unsigned char *)inputY), 1, frameSize, file_fd);
    }
    fclose(file_fd);

    PMLOG_INFO(CONST_MODULE_SM, " X");
    return true;

}

