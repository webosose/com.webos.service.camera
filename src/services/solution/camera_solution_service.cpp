/**
 * Copyright(c) 2023 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    camera_solution_service.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution service
 *
 */

#include "camera_solution_service.h"
#include "camera_solution_async.h"
#include "camera_types.h"
#include <pbnjson.hpp>
#include <string>

const char *const CONST_MODULE_CSS = "CameraSolutionService";

CameraSolutionService::CameraSolutionService(const char *service_name)
    : LS::Handle(LS::registerService(service_name))
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
        if (!solutionName.empty())
        {
            pFeature_ = pluginFactory_.createFeature(solutionName.c_str());
            if (pFeature_)
            {
                void *pInterface = nullptr;
                pFeature_->queryInterface(solutionName.c_str(), &pInterface);
                pSolution_ = static_cast<ISolution *>(pInterface);
            }
            if (pSolution_)
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
    if (pSolution_)
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_METASIZE_HINT),
                    jnumber_create_i32(pSolution_->getMetaSizeHint()));

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

    if (pSolution_)
        pSolution_->initialize(&streamFormat_, shmkey, this->get());
    else
        ret = false;

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

    if (pSolution_)
        pSolution_->setEnableValue(enableValue);

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

    if (pSolution_)
        pSolution_->release();

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
            serviceName = optarg ? optarg : "";
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
