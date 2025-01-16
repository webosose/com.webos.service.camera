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

#define LOG_CONTEXT "solution.FaceDetection"
#define LOG_TAG "FaceDetectionAIF"
#include "face_detection_aif.hpp"
#include "camera_constants.h"
#include "camera_log.h"
#include "plugin.hpp"
#include <cstdio>
#include <cstdlib>
#include <jpeglib.h>
#include <json_utils.h>
#include <string>

using namespace cv;

#define AIF_PARAM_FILE "/home/root/aif_param.json"

FaceDetectionAIF::FaceDetectionAIF(void) { PLOGI(""); }

FaceDetectionAIF::~FaceDetectionAIF(void) { PLOGI(""); }

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
    PLOGI("");
    solutionProperty_ = LG_SOLUTION_PREVIEW | LG_SOLUTION_SNAPSHOT;

    std::lock_guard<std::mutex> lock(mtxAi_);
    EdgeAIVision::getInstance().startup();

    // clang-format off
#ifdef PLATFORM_O22
    std::string param = json{
        {
            "param",
            {
                {
                    "autoDelegate",
                    {
                        {"policy", "PYTORCH_MODEL_GPU"},
                        {"cpu_fallback_percentage", 15}
                    }
                },
                {
                    "modelParam",
                    {
                        {"scoreThreshold", 0.7},
                        {"nmsThreshold", 0.3},
                        {"topK", 5000}
                    }
                }
            }
        }}.dump();
#else
    std::string param = json{
        {
            "param",
            {
                {
                    "autoDelegate",
                    {
                        {"policy", "CPU_ONLY"}
                    }
                },
                {
                    "modelParam",
                    {
                        {"scoreThreshold", 0.7},
                        {"nmsThreshold", 0.3},
                        {"topK", 5000}
                    }
                }
            }
        }}.dump();
#endif
    // clang-format on

    if (access(AIF_PARAM_FILE, F_OK) == 0)
    {
        auto obj_aifparam = pbnjson::JDomParser::fromFile(AIF_PARAM_FILE);
        if (obj_aifparam.isObject())
        {
            param = obj_aifparam.stringify();
        }
    }

    PLOGI("aif_param = %s", param.c_str());
    EdgeAIVision::getInstance().createDetector(type, param);

    CameraSolution::initialize(streamFormat, shmKey, lsHandle);
    PLOGI("");
}

void FaceDetectionAIF::release(void)
{
    PLOGI("");
    mtxAi_.lock();
    EdgeAIVision::getInstance().deleteDetector(type);
    EdgeAIVision::getInstance().shutdown();
    mtxAi_.unlock();
    CameraSolutionAsync::release();
    PLOGI("");
}

void FaceDetectionAIF::processing(void)
{
    if (streamFormat_.pixel_format != CAMERA_PIXEL_FORMAT_JPEG)
        return;

    if (!decodeJpeg())
        return;
    if (!detectFace())
        return;

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

    json joutfaces = json::array();
    json jresult   = json::parse(output, nullptr, false);
    if (!jresult.is_discarded())
    {
        json jfaces = get_optional<json>(jresult, "faces").value_or(nullptr);
        if (jfaces != nullptr && jfaces.is_array())
        {
            PLOGI("Detected face count : %zd", jfaces.size());
            for (const auto &jface : jfaces)
            {
                if (!jface.contains("region") || !jface.contains("score"))
                    continue;

                auto score  = get_optional<double>(jface, "score").value_or(0);
                auto region = get_optional<std::vector<double>>(jface, "region")
                                  .value_or(std::vector<double>{});
                if (region.size() == 4)
                {
                    // Since the face detection result is normalized between 0 and 1,
                    // the box size must be calculated using the original frame size
                    json joutface = json::object();
                    joutface["x"] = static_cast<int>(region[0] * oDecodedImage_.srcWidth_);
                    joutface["y"] = static_cast<int>(region[1] * oDecodedImage_.srcHeight_);
                    joutface["w"] = static_cast<int>(region[2] * oDecodedImage_.srcWidth_);
                    joutface["h"] = static_cast<int>(region[3] * oDecodedImage_.srcHeight_);
                    joutface["confidence"] = static_cast<int>(round(score * 100));
                    joutfaces.push_back(joutface);
                }
            }
        }
    }
    json jout;
    jout["faces"]                      = std::move(joutfaces);
    jout[CONST_PARAM_NAME_RETURNVALUE] = true;
    std::string strOutput              = jout.dump();

    if (pEvent_ && getMetaSizeHint() > 0)
        (pEvent_.load())->onDone(strOutput.c_str());

    sendReply(std::move(strOutput));
}

void FaceDetectionAIF::postProcessing(void)
{
    PLOGI("");

    std::string strOutput = json{{"faces", json::array()}}.dump();

    if (pEvent_ && getMetaSizeHint() > 0)
        (pEvent_.load())->onDone(strOutput.c_str());

    sendReply(std::move(strOutput));
}

bool FaceDetectionAIF::detectFace(void)
{
    std::lock_guard<std::mutex> lock(mtxAi_);
    EdgeAIVision::getInstance().detect(
        type,
        Mat(Size((oDecodedImage_.outWidth_ <= INT_MAX) ? oDecodedImage_.outWidth_ : 0,
                 (oDecodedImage_.outHeight_ <= INT_MAX) ? oDecodedImage_.outHeight_ : 0),
            CV_8UC3, oDecodedImage_.pImage_),
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
        PLOGI("Image decoding is failed");
        return false;
    }

    oDecodedImage_.srcColorSpace_ = (cinfo.jpeg_color_space > 0) ? cinfo.jpeg_color_space : 0;
    oDecodedImage_.srcWidth_      = cinfo.image_width;
    oDecodedImage_.srcHeight_     = cinfo.image_height;

    cinfo.scale_num       = 1;
    cinfo.scale_denom     = 1;
    cinfo.out_color_space = JCS_EXT_BGR;

    jpeg_start_decompress(&cinfo);

    oDecodedImage_.outColorSpace_ = cinfo.out_color_space;
    oDecodedImage_.outWidth_      = cinfo.output_width;
    oDecodedImage_.outHeight_     = cinfo.output_height;
    oDecodedImage_.outChannels_   = (cinfo.num_components > 0) ? cinfo.num_components : 0;
    oDecodedImage_.outStride_ =
        cinfo.output_width * ((cinfo.num_components > 0) ? cinfo.num_components : 0);

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

void FaceDetectionAIF::sendReply(std::string message)
{
    if (sh_)
    {
        unsigned int num_subscribers = 0;
        LSError lserror;
        LSErrorInit(&lserror);

        num_subscribers = LSSubscriptionGetHandleSubscribersCount(sh_, SOL_SUBSCRIPTION_KEY);
        PLOGD("cnt %u", num_subscribers);

        if (num_subscribers > 0)
        {
            if (!LSSubscriptionReply(sh_, SOL_SUBSCRIPTION_KEY, message.c_str(), &lserror))
            {
                LSErrorPrint(&lserror, stderr);
                LSErrorFree(&lserror);
                PLOGE("subscription reply failed");
                return;
            }
            PLOGD("subscription reply ok");
        }

        LSErrorFree(&lserror);
    }
}

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
