/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    FaceDetectionCNN.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution FaceDetectionCNN
 *
 */

#include "face_detection_cnn.hpp"
#include "camera_solution_event.h"
#include "camera_types.h"
#include <cstdio>
#include <cstdlib>
#include <facedetectcnn.h>
#include <jpeglib.h>
#include <map>
#include <pbnjson.h>
#include <string>

#define LOG_TAG "FaceDetectionCNN"

const std::map<int, const char *> tblColorSpace = {{0, "UNKNOWN"}, {1, "GRAYSCALE"}, {2, "RGB"},
                                                   {3, "YCbCr"},   {4, "CMYK"},      {5, "YCCK"}};

int getScaleDenom(int height)
{
    if (height >= 2160) // 3840x2160 -> 480x270
    {
        return 8;
    }
    else if (height >= 1080) // 1920x1080 -> 480x270
    {
        return 4;
    }
    else if (height >= 720) // 1280x720 -> 320x180
    {
        return 4;
    }
    else if (height >= 480) // 640x480 -> 320x240
    {
        return 2;
    }

    return 1;
}

FaceDetectionCNN::FaceDetectionCNN(void)
{
    PMLOG_INFO(LOG_TAG, "");
    solutionProperty_ = Property(LG_SOLUTION_PREVIEW | LG_SOLUTION_SNAPSHOT);

// define the buffer size. Do not change the size!
#define DETECT_BUFFER_SIZE 0x20000
    pBuffer_ = new uint8_t[DETECT_BUFFER_SIZE];
}

FaceDetectionCNN::~FaceDetectionCNN(void)
{
    PMLOG_INFO(LOG_TAG, "");
    delete[] pBuffer_;
}

int32_t FaceDetectionCNN::getMetaSizeHint(void)
{
    // 10 * 1 : {"faces":[
    // 56 * n : {"w":xxx,"confidence":xxx,"y":xxxx,"h":xxxx,"x":xxxx},
    // 02 * 1 : ]}
    // n <-- 100
    // size = 10 + 56*100 + 2 = 572
    // size + padding -> 1024
    return 1024;
}

std::string FaceDetectionCNN::getSolutionStr(void) { return SOLUTION_FACE_DETECTION_CNN; }

void FaceDetectionCNN::processing(void)
{
    do
    {
        if (streamFormat_.pixel_format != CAMERA_PIXEL_FORMAT_JPEG)
            break;

        if (!decodeJpeg())
            break;
        if (!detectFace())
            break;

        int scale     = getScaleDenom(oDecodedImage_.srcHeight_);
        int *pResults = (int *)pBuffer_;

        jvalue_ref jsonOutObj    = jobject_create();
        jvalue_ref jsonFaceArray = jarray_create(nullptr);

        PMLOG_INFO(LOG_TAG, "Detected face count : %d", pResults[0]);
        for (int i = 0; i < pResults[0]; ++i)
        {
            short *p = ((short *)(pResults + 1)) + 142 * i;

            jvalue_ref jsonFaceElementObj = jobject_create();
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("confidence"), jnumber_create_i32(p[0]));
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("x"), jnumber_create_i32(p[1] * scale));
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("y"), jnumber_create_i32(p[2] * scale));
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("w"), jnumber_create_i32(p[3] * scale));
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("h"), jnumber_create_i32(p[4] * scale));
            jarray_append(jsonFaceArray, jsonFaceElementObj);
        }

        jobject_put(jsonOutObj, J_CSTR_TO_JVAL("faces"), jsonFaceArray);

        if (pEvent_)
            (pEvent_.load())->onDone(jsonOutObj);

        j_release(&jsonOutObj);
    } while (0);
}

bool FaceDetectionCNN::detectFace(void)
{
    // DO NOT RELEASE pResults !!!
    // int* pResults =
    facedetect_cnn(pBuffer_, oDecodedImage_.pImage_, oDecodedImage_.outWidth_,
                   oDecodedImage_.outHeight_, oDecodedImage_.outStride_);
    return true;
    // TODO : Do we need to decide success or failure from here?
    //        Just now, I think it's a role of applicaiton.
    //        So, the camera solution will report every results.
    // return pResults[0] > 0 ? true : false;
}

bool FaceDetectionCNN::decodeJpeg(void)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);

    auto &buf = queueJob_.front();

    jpeg_mem_src(&cinfo, buf->data_, buf->size_);
    if (jpeg_read_header(&cinfo, TRUE) != 1)
    {
        PMLOG_INFO(LOG_TAG, "Image decoding is failed");
        return false;
    }

    oDecodedImage_.srcColorSpace_ = cinfo.jpeg_color_space;
    oDecodedImage_.srcWidth_      = cinfo.image_width;
    oDecodedImage_.srcHeight_     = cinfo.image_height;

    cinfo.scale_num   = 1;
    cinfo.scale_denom = getScaleDenom(oDecodedImage_.srcHeight_);

    jpeg_start_decompress(&cinfo);

    oDecodedImage_.outColorSpace_ = cinfo.out_color_space;
    oDecodedImage_.outWidth_      = cinfo.output_width;
    oDecodedImage_.outHeight_     = cinfo.output_height;
    oDecodedImage_.outChannels_   = cinfo.num_components;
    oDecodedImage_.outStride_     = cinfo.output_width * cinfo.num_components;

    oDecodedImage_.prepareImage();

    while (cinfo.output_scanline < cinfo.output_height)
    {
        unsigned char *buffer_array[1];
        buffer_array[0] = oDecodedImage_.getLine(cinfo.output_scanline);
        jpeg_read_scanlines(&cinfo, buffer_array, 1);
    }

    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);

    return true;
}
