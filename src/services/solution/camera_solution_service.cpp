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

#define LOG_TAG "CameraSolutionService"
#include "camera_solution_service.h"
#include "camera_solution_async.h"
#include "camera_types.h"
#include "error_manager.h"
#include <pbnjson.hpp>
#include <string>

CameraSolutionService::CameraSolutionService(const char *service_name)
    : LS::Handle(LS::registerService(service_name))
{
    PLOGI("Start : %s", service_name);

    LS_CATEGORY_BEGIN(CameraSolutionService, "/")
    LS_CATEGORY_METHOD(create)
    LS_CATEGORY_METHOD(init)
    LS_CATEGORY_METHOD(enable)
    LS_CATEGORY_METHOD(release)
    LS_CATEGORY_METHOD(subscribe)
    LS_CATEGORY_END;

    // attach to mainloop and run it
    attachToLoop(main_loop_ptr_.get());

    // run the gmainloop
    g_main_loop_run(main_loop_ptr_.get());
}

bool CameraSolutionService::create(LSMessage &message)
{
    bool ret           = false;
    ErrorCode err_code = ERROR_CODE_END;
    std::string solutionName;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_NAME))
    {
        solutionName = parsed[CONST_PARAM_NAME_NAME].asString();
        if (solutionName.empty())
        {
            err_code = SOLLUTION_NAME_IS_EMPTY;
        }
        else
        {
            pFeature_ = pluginFactory_.createFeature(solutionName.c_str());
            if (pFeature_)
            {
                void *pInterface = nullptr;
                pFeature_->queryInterface(solutionName.c_str(), &pInterface);
                pSolution_ = static_cast<ISolution *>(pInterface);
                if (pSolution_)
                {
                    ret = true;
                }
                else
                {
                    err_code = FAIL_TO_CREATE_SOLUTION;
                }
            }
            else
            {
                err_code = FAIL_TO_OPEN_PLUGIN;
            }
        }
    }
    else
    {
        err_code = SOLLUTION_NAME_IS_REQUIRED;
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));
    if (ret == false)
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_CODE),
                    jnumber_create_i32(static_cast<int32_t>(err_code)));
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_TEXT),
                    jstring_create(ErrorManager::GetErrorText(err_code).c_str()));
    }

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

bool CameraSolutionService::init(LSMessage &message)
{
    bool ret = true;
    stream_format_t streamFormat_{CAMERA_PIXEL_FORMAT_JPEG, 0, 0, 0, 0};
    key_t shmkey           = 0;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_FORMAT))
    {
        streamFormat_.pixel_format =
            (camera_pixel_format_t)parsed[CONST_PARAM_NAME_FORMAT].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_WIDTH))
    {
        int w                      = parsed[CONST_PARAM_NAME_WIDTH].asNumber<int>();
        streamFormat_.stream_width = (w > 0) ? w : 0;
    }

    if (parsed.hasKey(CONST_PARAM_NAME_HEIGHT))
    {
        int h                       = parsed[CONST_PARAM_NAME_HEIGHT].asNumber<int>();
        streamFormat_.stream_height = (h > 0) ? h : 0;
    }

    if (parsed.hasKey(CONST_PARAM_NAME_FPS))
    {
        streamFormat_.stream_fps = parsed[CONST_PARAM_NAME_FPS].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_BUFFERSIZE))
    {
        int sz_buf                = parsed[CONST_PARAM_NAME_BUFFERSIZE].asNumber<int>();
        streamFormat_.buffer_size = (sz_buf > 0) ? sz_buf : 0;
    }

    if (parsed.hasKey(CONST_PARAM_NAME_SHMKEY))
    {
        shmkey = parsed[CONST_PARAM_NAME_SHMKEY].asNumber<int>();
        PLOGI("shmkey %d", shmkey);
    }

    if (pSolution_)
        pSolution_->initialize(&streamFormat_, shmkey, this->get());

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

bool CameraSolutionService::enable(LSMessage &message)
{
    bool ret               = false;
    ErrorCode err_code     = ERROR_CODE_END;
    bool enableValue       = false;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_ENABLE))
    {
        enableValue = parsed[CONST_PARAM_NAME_ENABLE].asBool();

        if (pSolution_)
        {
            pSolution_->setEnableValue(enableValue);
            ret = true;
        }
    }
    else
    {
        err_code = ENABLE_VALUE_IS_REQUIRED;
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));
    if (ret == false)
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_CODE),
                    jnumber_create_i32(static_cast<int32_t>(err_code)));
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_TEXT),
                    jstring_create(ErrorManager::GetErrorText(err_code).c_str()));
    }

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
    PLOGI("payload %s", payload);

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
    PLOGI("LSSubscriptionAdd %s", ret ? "ok" : "failed");
    PLOGI("cnt %d", LSSubscriptionGetHandleSubscribersCount(this->get(), SOL_SUBSCRIPTION_KEY));
    LSErrorFree(&error);

    jvalue_ref json_outobj = jobject_create();
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SUBSCRIBED), jboolean_create(true));
    if (ret == false)
    {
        ErrorCode err_code = FAIL_TO_SUBSCRIBE;
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_CODE),
                    jnumber_create_i32(static_cast<int32_t>(err_code)));
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_TEXT),
                    jstring_create(ErrorManager::GetErrorText(err_code).c_str()));
    }
    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

std::string parseSolutionServiceName(int argc, char *argv[]) noexcept
{
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
            PLOGI("unknown service name");
            break;

        default:
            break;
        }
    }
    if (serviceName.empty())
    {
        PLOGI("service name is not specified");
    }
    return serviceName;
}

int main(int argc, char *argv[])
{
    try
    {
        std::string serviceName = parseSolutionServiceName(argc, argv);
        if (serviceName.empty())
        {
            return 1;
        }
        CameraSolutionService cameraSolutionServiceInstance(serviceName.c_str());
    }
    catch (LS::Error &err)
    {
        LSErrorPrint(err, stdout);
        return 1;
    }
    catch (const std::ios::failure &err)
    {
        std::cerr << err.what() << std::endl;
        return 1;
    }
    catch (...)
    {
        std::cerr << "An unknown exception occurred." << std::endl;
        return 1;
    }
    return 0;
}
