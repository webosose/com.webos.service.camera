/**
 * Copyright(c) 2023 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolutionService.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution service
 *
 */

#include "CameraSolutionService.h"
#include "camera_types.h"
#include "face_detection_aif.hpp"
#include <pbnjson.hpp>
#include <string>

const char *const CONST_MODULE_CSS     = "CameraSolutionService";
const char *const SOL_SUBSCRIPTION_KEY = "cameraSolution";

CameraSolutionService::CameraSolutionService(const char *service_name)
    : LS::Handle(LS::registerService(service_name)), pCameraSolution(nullptr)
{
    PMLOG_INFO(CONST_MODULE_CSS, "Start : %s", service_name);

    LS_CATEGORY_BEGIN(CameraSolutionService, "/")
    LS_CATEGORY_METHOD(createSolution)
    LS_CATEGORY_METHOD(getMetaSizeHint)
    LS_CATEGORY_METHOD(initialize)
    LS_CATEGORY_METHOD(setEnableValue)
    LS_CATEGORY_METHOD(release)
    LS_CATEGORY_METHOD(subscribe)
    LS_CATEGORY_END;

    // attach to mainloop and run it
    attachToLoop(main_loop_ptr_.get());

    // run the gmainloop
    g_main_loop_run(main_loop_ptr_.get());
}

bool CameraSolutionService::createSolution(LSMessage &message)
{
    bool ret = false;
    std::string solutionName;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CSS, "payload %s", payload);
    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_NAME))
    {
        solutionName = parsed[CONST_PARAM_NAME_NAME].asString();

        if (solutionName.compare(SOLUTION_FACEDETECTION) == 0)
        {
            pCameraSolution = std::make_unique<FaceDetectionAIF>();
            if (pCameraSolution)
            {
                ret = true;
            }
        }
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

bool CameraSolutionService::getMetaSizeHint(LSMessage &message)
{
    bool ret               = true;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CSS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_METASIZE_HINT),
                jnumber_create_i32(pCameraSolution->getMetaSizeHint()));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

bool CameraSolutionService::initialize(LSMessage &message)
{
    bool ret = true;
    stream_format_t streamFormat_{CAMERA_PIXEL_FORMAT_JPEG, 0, 0, 0, 0};
    key_t shmkey           = 0;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CSS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_FORMAT))
    {
        streamFormat_.pixel_format =
            (camera_pixel_format_t)parsed[CONST_PARAM_NAME_FORMAT].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_WIDTH))
    {
        streamFormat_.stream_width = parsed[CONST_PARAM_NAME_WIDTH].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_HEIGHT))
    {
        streamFormat_.stream_height = parsed[CONST_PARAM_NAME_HEIGHT].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_FPS))
    {
        streamFormat_.stream_fps = parsed[CONST_PARAM_NAME_FPS].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_BUFFERSIZE))
    {
        streamFormat_.buffer_size = parsed[CONST_PARAM_NAME_BUFFERSIZE].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_SHMKEY))
    {
        shmkey = parsed[CONST_PARAM_NAME_SHMKEY].asNumber<int>();
        PMLOG_INFO(CONST_MODULE_CSS, "shmkey %d", shmkey);
    }

    pCameraSolution->initialize(streamFormat_, shmkey, this->get());

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

bool CameraSolutionService::setEnableValue(LSMessage &message)
{
    bool ret               = true;
    bool enableValue       = false;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CSS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_ENABLE))
    {
        enableValue = parsed[CONST_PARAM_NAME_ENABLE].asBool();
    }

    pCameraSolution->setEnableValue(enableValue);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

bool CameraSolutionService::release(LSMessage &message)
{
    bool ret               = true;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CSS, "payload %s", payload);

    pCameraSolution->release();

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    g_main_loop_quit(main_loop_ptr_.get());
    return ret;
}

bool CameraSolutionService::subscribe(LSMessage &message)
{
    LSError error;
    LSErrorInit(&error);

    bool ret = LSSubscriptionAdd(this->get(), SOL_SUBSCRIPTION_KEY, &message, &error);
    PMLOG_INFO(CONST_MODULE_CSS, "LSSubscriptionAdd %s", ret ? "ok" : "failed");
    PMLOG_INFO(CONST_MODULE_CSS, "cnt %d",
               LSSubscriptionGetHandleSubscribersCount(this->get(), SOL_SUBSCRIPTION_KEY));
    LSErrorFree(&error);

    jvalue_ref json_outobj = jobject_create();
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));
    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

int main(int argc, char *argv[])
{
    PMLOG_INFO(CONST_MODULE_CSS, "start");
    int c;
    std::string serviceName;

    while ((c = getopt(argc, argv, "s:")) != -1)
    {
        switch (c)
        {
        case 's':
            serviceName = optarg;
            break;

        case '?':
            PMLOG_INFO(CONST_MODULE_CSS, "unknown service name");
            break;

        default:
            break;
        }
    }

    if (serviceName.empty())
    {
        PMLOG_INFO(CONST_MODULE_CSS, "service name is not specified");
        return 1;
    }

    try
    {
        CameraSolutionService CameraSolutionService(serviceName.c_str());
    }
    catch (LS::Error &err)
    {
        LSErrorPrint(err, stdout);
        return 1;
    }

    PMLOG_INFO(CONST_MODULE_CSS, "end");
    return 0;
}
