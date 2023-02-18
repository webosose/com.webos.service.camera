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
#include "camera_log.h"
#include "plugin.hpp"
#include <cstdio>
#include <cstdlib>
#include <jpeglib.h>
#include <pbnjson.hpp>
#include <rapidjson/document.h>
#include <string>

using namespace cv;

namespace rj = rapidjson;

#define LOG_TAG "FaceDetectionAIF"

#define AIF_PARAM_FILE "/home/root/aif_param.json"

FaceDetectionAIF::FaceDetectionAIF(void) { PMLOG_INFO(LOG_TAG, ""); }

FaceDetectionAIF::~FaceDetectionAIF(void) { PMLOG_INFO(LOG_TAG, ""); }

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

std::string FaceDetectionAIF::getSolutionStr(void) { return SOLUTION_FACEDETECTION; }

void FaceDetectionAIF::initialize(const void *streamFormat, int shmKey, void *lsHandle)
{
    PMLOG_INFO(LOG_TAG, "");
    solutionProperty_ = Property(LG_SOLUTION_PREVIEW | LG_SOLUTION_SNAPSHOT);

    std::lock_guard<std::mutex> lock(mtxAi_);
    EdgeAIVision::getInstance().startup();

    std::string param = R"({
                                "param": {
                                    "autoDelegate": {
                                        "policy": "PYTORCH_MODEL_GPU",
                                        "cpu_fallback_percentage": 15
                                    },
                                    "modelParam": {
                                    "scoreThreshold": 0.7,
                                    "nmsThreshold": 0.3,
                                    "topK": 5000
                                    }
                                }
                            })";

    if (access(AIF_PARAM_FILE, F_OK) == 0)
    {
        auto obj_aifparam = pbnjson::JDomParser::fromFile(AIF_PARAM_FILE);
        if (obj_aifparam.isObject())
        {
            param = obj_aifparam.stringify();
        }
    }

    PMLOG_INFO(LOG_TAG, "aif_param = %s", param.c_str());
    EdgeAIVision::getInstance().createDetector(type, param);

    CameraSolution::initialize(streamFormat, shmKey, lsHandle);
    PMLOG_INFO(LOG_TAG, "");
}

void FaceDetectionAIF::release(void)
{
    PMLOG_INFO(LOG_TAG, "");
    mtxAi_.lock();
    EdgeAIVision::getInstance().deleteDetector(type);
    EdgeAIVision::getInstance().shutdown();
    mtxAi_.unlock();
    CameraSolutionAsync::release();
    PMLOG_INFO(LOG_TAG, "");
}

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

        if (pEvent_ && getMetaSizeHint() > 0)
            (pEvent_.load())->onDone(jvalue_stringify(jsonOutObj));

        // Subscription reply
        if (sh_)
        {
            int num_subscribers = 0;
            std::string reply;
            LSError lserror;
            LSErrorInit(&lserror);

            {
                std::string subskey_ = "cameraSolution";
                num_subscribers = LSSubscriptionGetHandleSubscribersCount(sh_, subskey_.c_str());
                PMLOG_INFO(LOG_TAG, "cnt %d", num_subscribers);

                if (num_subscribers > 0)
                {
                    reply = jvalue_stringify(jsonOutObj);
                    if (!LSSubscriptionReply(sh_, subskey_.c_str(), reply.c_str(), &lserror))
                    {
                        LSErrorPrint(&lserror, stderr);
                        LSErrorFree(&lserror);
                        PMLOG_INFO(LOG_TAG, "subscription reply failed");
                        return;
                    }
                    PMLOG_INFO(LOG_TAG, "subscription reply ok");
                }
            }
            LSErrorFree(&lserror);
        }

        j_release(&jsonOutObj);
    } while (0);
}

void FaceDetectionAIF::postProcessing(void)
{
    PMLOG_INFO(LOG_TAG, "");
    jvalue_ref jsonOutObj    = jobject_create();
    jvalue_ref jsonFaceArray = jarray_create(nullptr);
    jobject_put(jsonOutObj, J_CSTR_TO_JVAL("faces"), jsonFaceArray);
    if (pEvent_ && getMetaSizeHint() > 0)
        (pEvent_.load())->onDone(jvalue_stringify(jsonOutObj));

    // Subscription reply
    if (sh_)
    {
        int num_subscribers = 0;
        std::string reply;
        LSError lserror;
        LSErrorInit(&lserror);

        {
            std::string subskey_ = "cameraSolution";
            num_subscribers      = LSSubscriptionGetHandleSubscribersCount(sh_, subskey_.c_str());
            PMLOG_INFO(LOG_TAG, "cnt %d", num_subscribers);

            if (num_subscribers > 0)
            {
                reply = jvalue_stringify(jsonOutObj);
                if (!LSSubscriptionReply(sh_, subskey_.c_str(), reply.c_str(), &lserror))
                {
                    LSErrorPrint(&lserror, stderr);
                    LSErrorFree(&lserror);
                    PMLOG_INFO(LOG_TAG, "subscription reply failed");
                    return;
                }
                PMLOG_INFO(LOG_TAG, "subscription reply ok");
            }
        }
        LSErrorFree(&lserror);
    }
}

bool FaceDetectionAIF::detectFace(void)
{
    std::lock_guard<std::mutex> lock(mtxAi_);
    EdgeAIVision::getInstance().detect(
        type,
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
    cinfo.scale_denom     = 1;
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

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
extern "C"
{
    IPlugin *plugin_init(void)
    {
        Plugin *plg = new Plugin();
        plg->setName("FaceDetectionAIF");
        plg->setDescription("Face Detection");
        plg->setCategory("SOLUTION");
        plg->setVersion("1.0.0");
        plg->setOrganization("LG Electronics.");
        plg->registerFeature<FaceDetectionAIF>("FaceDetection");

        return plg;
    }

    void __attribute__((constructor)) plugin_load(void)
    {
        printf("%s:%s\n", __FILENAME__, __PRETTY_FUNCTION__);
    }

    void __attribute__((destructor)) plugin_unload(void)
    {
        printf("%s:%s\n", __FILENAME__, __PRETTY_FUNCTION__);
    }
}
