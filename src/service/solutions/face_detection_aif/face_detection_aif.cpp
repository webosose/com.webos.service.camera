/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    face_detection_aif.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution FaceDetectionAIF
 *
 */

#include "face_detection_aif.hpp"
#include "camera_solution_event.h"
#include "camera_types.h"
#include <cstdio>
#include <cstdlib>
#include <jpeglib.h>
#include <pbnjson.h>
#include <rapidjson/document.h>
#include <string>

using namespace cv;

namespace rj = rapidjson;

#define LOG_TAG "FaceDetectionAIF"

int getScaleDenomAIF(int height)
{
    if (height >= 2160)
    { // 3840x2160 -> 480x270
        return 8;
    }
    else if (height >= 1080)
    { // 1920x1080 -> 480x270
        return 4;
    }
    else if (height >= 720)
    { // 1280x720 -> 320x180
        return 4;
    }
    else if (height >= 480)
    { // 640x480 -> 320x240
        return 2;
    }

    return 1;
}

FaceDetectionAIF::FaceDetectionAIF(void)
{
    PMLOG_INFO(LOG_TAG, "");
    solutionProperty_ = Property(LG_SOLUTION_PREVIEW | LG_SOLUTION_SNAPSHOT);

    ai.startup();

    std::string param = "{ \
          \"model\" : \"face_full_range_cpu\",    \
          \"param\": { \
              \"common\" : { \
                  \"useXnnpack\": true, \
                  \"numThreads\": 1 \
              },\
              \"modelParam\": { \
                  \"strides\": [4], \
                  \"optAspectRatios\": [1.0], \
                  \"interpolatedScaleAspectRatio\": 0.0, \
                  \"anchorOffsetX\": 0.5, \
                  \"anchorOffsetY\": 0.5, \
                  \"minScale\": 0.1484375, \
                  \"maxScale\": 0.75, \
                  \"reduceBoxesInLowestLayer\": false, \
                  \"scoreThreshold\": 0.5, \
                  \"iouThreshold\": 0.2, \
                  \"maxOutputSize\": 100, \
                  \"updateThreshold\": 0.3 \
              } \
           } \
      }";
    ai.createDetector(type, param);
}

FaceDetectionAIF::~FaceDetectionAIF(void)
{
    PMLOG_INFO(LOG_TAG, "");
    ai.deleteDetector(type);
    ai.shutdown();
    PMLOG_INFO(LOG_TAG, "");
}

int32_t FaceDetectionAIF::getMetaSizeHint(void)
{
    // 10 * 1 : {"faces":[
    // 56 * n : {"w":xxx,"confidence":xxx,"y":xxxx,"h":xxxx,"x":xxxx},
    // 02 * 1 : ]}
    // n <-- 100
    // size = 10 + 56*100 + 2 = 572
    // size + padding -> 1024
    return 1024;
}

std::string FaceDetectionAIF::getSolutionStr(void) { return SOLUTION_FACE_DETECTION_AIF; }

void FaceDetectionAIF::processing(void)
{
    do
    {
        if (streamFormat_.pixel_format != CAMERA_PIXEL_FORMAT_JPEG)
            break;

        if (!decodeJpeg())
            break;
        if (!detectFace())
            break;

        jvalue_ref jsonOutObj    = jobject_create();
        jvalue_ref jsonFaceArray = jarray_create(nullptr);

        rj::Document json;
        json.Parse(output.c_str());

        /*
        {
          "faces": [
            {
              "score" 0.7765,
              "region":[0.4437,0.5496,0.2013,0.2013],
              "lefteye":[0.501,0.6329],
              "righteye":[0.5936,0.6522],
              "nosetip":[0.5324,0.7263],
              "mouth":[0.5458,0.7388],
              "leftear":[0.4869,0.5744],
              "rightear":[0.687,0.5901]
            }
          ],
          "returnCode": 0
        }
        */

        const rj::Value &faces = json["faces"];
        PMLOG_INFO(LOG_TAG, "Detected face count : %d", faces.Size());

        for (rj::SizeType i = 0; i < faces.Size(); i++)
        {
            double dx    = faces[i]["region"][0].GetDouble();
            double dy    = faces[i]["region"][1].GetDouble();
            double dw    = faces[i]["region"][2].GetDouble();
            double dh    = faces[i]["region"][3].GetDouble();
            double score = faces[i]["score"].GetDouble();

            // Since the face detection result is normalized between 0 and 1,
            // the box size must be calculated using the original frame size
            int x          = dx * oDecodedImage_.srcWidth_;
            int y          = dy * oDecodedImage_.srcHeight_;
            int w          = dw * oDecodedImage_.srcWidth_;
            int h          = dh * oDecodedImage_.srcHeight_;
            int confidence = static_cast<int>(round(score * 100));

            jvalue_ref jsonFaceElementObj = jobject_create();
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("confidence"),
                        jnumber_create_i32(confidence));
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("x"), jnumber_create_i32(x));
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("y"), jnumber_create_i32(y));
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("w"), jnumber_create_i32(w));
            jobject_put(jsonFaceElementObj, J_CSTR_TO_JVAL("h"), jnumber_create_i32(h));
            jarray_append(jsonFaceArray, jsonFaceElementObj);
        }

        jobject_put(jsonOutObj, J_CSTR_TO_JVAL("faces"), jsonFaceArray);

        if (pEvent_)
            (pEvent_.load())->onDone(jsonOutObj);

        j_release(&jsonOutObj);
    } while (0);
}

bool FaceDetectionAIF::detectFace(void)
{
    ai.detect(type,
              Mat(Size(oDecodedImage_.outWidth_, oDecodedImage_.outHeight_), CV_8UC3,
                  oDecodedImage_.pImage_),
              output);
    return true;
    // TODO : Do we need to decide success or failure from here?
    //        Just now, I think it's a role of applicaiton.
    //        So, the camera solution will report every results.
    // return pResults[0] > 0 ? true : false;
}

bool FaceDetectionAIF::decodeJpeg(void)
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

    cinfo.scale_num       = 1;
    cinfo.scale_denom     = getScaleDenomAIF(oDecodedImage_.srcHeight_);
    cinfo.out_color_space = JCS_EXT_BGR;

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
