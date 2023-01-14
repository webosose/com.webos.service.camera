/**
 * Copyright(c) 2023 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraHalService.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera HAL service
 *
 */

#include "CameraHalService.h"
#include "PmLogLib.h"
#include "camera_types.h"
#include "device_controller.h"
#include <pbnjson.hpp>
#include <string>

#define CONST_MODULE_CHS "CameraHalService"
#define MAX_SERVICE_STRING 120

const char *const SUBSCRIPTION_KEY = "cameraHal";

CameraHalService::CameraHalService(const char *service_name)
    : LS::Handle(LS::registerService(service_name)), pCamHandle(NULL)
{
    PMLOG_INFO(CONST_MODULE_CHS, "Start : %s", service_name);

    LS_CATEGORY_BEGIN(CameraHalService, "/")
    LS_CATEGORY_METHOD(createHandle)
    LS_CATEGORY_METHOD(destroyHandle)
    LS_CATEGORY_METHOD(open)
    LS_CATEGORY_METHOD(close)
    LS_CATEGORY_METHOD(startPreview)
    LS_CATEGORY_METHOD(stopPreview)
    LS_CATEGORY_METHOD(captureImage)
    LS_CATEGORY_METHOD(startCapture)
    LS_CATEGORY_METHOD(stopCapture)
    LS_CATEGORY_METHOD(getDeviceProperty)
    LS_CATEGORY_METHOD(setDeviceProperty)
    LS_CATEGORY_METHOD(setFormat)
    LS_CATEGORY_METHOD(getFormat)
    LS_CATEGORY_METHOD(getDeviceInfo)
    LS_CATEGORY_METHOD(registerClient)
    LS_CATEGORY_METHOD(unregisterClient)
    LS_CATEGORY_METHOD(isRegisteredClient)
    LS_CATEGORY_METHOD(requestPreviewCancel)
    LS_CATEGORY_METHOD(getSupportedCameraSolutionInfo)
    LS_CATEGORY_METHOD(getEnabledCameraSolutionInfo)
    LS_CATEGORY_METHOD(enableCameraSolution)
    LS_CATEGORY_METHOD(disableCameraSolution)
    LS_CATEGORY_METHOD(subscribe)
    LS_CATEGORY_END;

    pDeviceControl = std::make_unique<DeviceControl>();

    // attach to mainloop and run it
    attachToLoop(main_loop_ptr_.get());

    // run the gmainloop
    g_main_loop_run(main_loop_ptr_.get());
}

bool CameraHalService::createHandle(LSMessage &message)
{
    bool ret = false;
    std::string deviceType;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("subSystem"))
    {
        deviceType = parsed["subSystem"].asString().c_str();
    }

    if (pDeviceControl->createHandle(&pCamHandle, deviceType) == DEVICE_OK)
    {
        ret = true;
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    return ret;
}

bool CameraHalService::destroyHandle(LSMessage &message)
{
    bool ret               = false;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    if (pDeviceControl->destroyHandle(pCamHandle) == DEVICE_OK)
    {
        ret = true;
    }
    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    g_main_loop_quit(main_loop_ptr_.get());
    return ret;
}

bool CameraHalService::open(LSMessage &message)
{
    bool ret = false;
    std::string devicenode;
    int ndev_id = 0;
    std::string payload_;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("devPath"))
    {
        devicenode = parsed["devPath"].asString().c_str();
    }

    if (parsed.hasKey("cameraID"))
    {
        ndev_id = parsed["cameraID"].asNumber<int>();
    }

    if (parsed.hasKey("payload"))
    {
        payload_ = parsed["payload"].asString().c_str();
    }

    if (pDeviceControl->open(pCamHandle, devicenode, ndev_id, payload_) == DEVICE_OK)
    {
        ret = true;
    }
    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::close(LSMessage &message)
{
    bool ret               = false;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    if (pDeviceControl->close(pCamHandle) == DEVICE_OK)
    {
        ret = true;
    }
    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::startPreview(LSMessage &message)
{
    bool ret               = false;
    int pkey               = 0;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    std::string memtype;
    if (parsed.hasKey("memType"))
    {
        memtype = parsed["memType"].asString().c_str();
    }
    PMLOG_INFO(CONST_MODULE_CHS, "memtype(%s)", memtype.c_str());

    if (pDeviceControl->startPreview(pCamHandle, memtype, &pkey, this->get(), SUBSCRIPTION_KEY) ==
        DEVICE_OK)
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
        jobject_put(json_outobj, J_CSTR_TO_JVAL("shmKey"), jnumber_create_i32(pkey));
    }
    else
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(false));
    }

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::stopPreview(LSMessage &message)
{
    bool ret    = false;
    int memtype = 0;
    ;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);
    if (parsed.hasKey("memType"))
    {
        memtype = parsed["memType"].asNumber<int>();
    }

    if (pDeviceControl->stopPreview(pCamHandle, memtype) == DEVICE_OK)
    {
        ret = true;
    }
    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::captureImage(LSMessage &message)
{
    bool ret   = true;
    int ncount = 1;
    CAMERA_FORMAT sformat;
    const std::string imagepath;
    const std::string mode;
    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);
    pDeviceControl->captureImage(pCamHandle, ncount, sformat, imagepath, mode);
    return ret;
}

bool CameraHalService::startCapture(LSMessage &message)
{
    bool ret = false;
    CAMERA_FORMAT sformat;
    std::string imagepath;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("width"))
    {
        sformat.nWidth = parsed["width"].asNumber<int>();
    }

    if (parsed.hasKey("height"))
    {
        sformat.nHeight = parsed["height"].asNumber<int>();
    }

    if (parsed.hasKey("format"))
    {
        sformat.eFormat = (camera_format_t)parsed["height"].asNumber<int>();
    }

    if (parsed.hasKey("path"))
    {
        imagepath = parsed["path"].asString().c_str();
    }

    if (pDeviceControl->startCapture(pCamHandle, sformat, imagepath) == DEVICE_OK)
    {
        ret = true;
    }
    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::stopCapture(LSMessage &message)
{
    bool ret      = true;
    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);
    return ret;
}

bool CameraHalService::getDeviceProperty(LSMessage &message)
{
    bool ret      = true;
    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);
    return ret;
}

bool CameraHalService::setDeviceProperty(LSMessage &message)
{
    bool ret      = true;
    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);
    return ret;
}

bool CameraHalService::setFormat(LSMessage &message)
{
    bool ret = false;
    CAMERA_FORMAT sformat;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("width"))
    {
        sformat.nWidth = parsed["width"].asNumber<int>();
    }

    if (parsed.hasKey("height"))
    {
        sformat.nHeight = parsed["height"].asNumber<int>();
    }

    if (parsed.hasKey("fps"))
    {
        sformat.nFps = parsed["fps"].asNumber<int>();
    }

    if (parsed.hasKey("cameraFormat"))
    {
        int eFormat     = parsed["cameraFormat"].asNumber<int>();
        sformat.eFormat = (camera_format_t)eFormat;
    }

    if (pDeviceControl->setFormat(pCamHandle, sformat) == DEVICE_OK)
    {
        ret = true;
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::getFormat(LSMessage &message)
{
    bool ret      = true;
    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);
    return ret;
}

bool CameraHalService::getDeviceInfo(LSMessage &message)
{
    bool ret = false;
    std::string strdevicenode;
    std::string deviceType;
    camera_device_info_t cameraInfo;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("devPath"))
    {
        strdevicenode = parsed["devPath"].asString().c_str();
    }

    if (parsed.hasKey("subSystem"))
    {
        deviceType = parsed["subSystem"].asString().c_str();
    }
    PMLOG_INFO(CONST_MODULE_CHS, "deviceType(%s)", deviceType.c_str());

    if (pDeviceControl->getDeviceInfo(strdevicenode, deviceType, &cameraInfo) == DEVICE_OK)
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));
        jobject_put(json_outobj, J_CSTR_TO_JVAL("deviceType"),
                    jnumber_create_i32(cameraInfo.n_devicetype));
        jobject_put(json_outobj, J_CSTR_TO_JVAL("buildin"),
                    jnumber_create_i32(cameraInfo.b_builtin));
    }
    else
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(false));
    }

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    g_main_loop_quit(main_loop_ptr_.get());
    return ret;
}

bool CameraHalService::registerClient(LSMessage &message)
{
    bool ret      = false;
    int pid       = 0;
    int sig       = 0;
    int devHandle = 0;
    std::string payload_;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("pid"))
    {
        pid = parsed["pid"].asNumber<int>();
    }

    if (parsed.hasKey("sig"))
    {
        sig = parsed["sig"].asNumber<int>();
    }

    if (parsed.hasKey("devHandle"))
    {
        devHandle = parsed["devHandle"].asNumber<int>();
    }

    std::string outmsg;
    ret = pDeviceControl->registerClient(pid, sig, devHandle, outmsg);

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));
    jobject_put(json_outobj, J_CSTR_TO_JVAL("outMsg"), jstring_create(outmsg.c_str()));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::unregisterClient(LSMessage &message)
{
    bool ret = false;
    int pid  = 0;
    std::string payload_;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("pid"))
    {
        pid = parsed["pid"].asNumber<int>();
    }

    std::string outmsg;
    ret = pDeviceControl->unregisterClient(pid, outmsg);

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));
    jobject_put(json_outobj, J_CSTR_TO_JVAL("outMsg"), jstring_create(outmsg.c_str()));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::isRegisteredClient(LSMessage &message)
{
    bool ret      = false;
    int devHandle = 0;
    std::string payload_;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("devHandle"))
    {
        devHandle = parsed["devHandle"].asNumber<int>();
    }

    std::string outmsg;
    ret = pDeviceControl->isRegisteredClient(devHandle);

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::requestPreviewCancel(LSMessage &message)
{
    bool ret               = true;
    jvalue_ref json_outobj = jobject_create();

    pDeviceControl->requestPreviewCancel();

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::getSupportedCameraSolutionInfo(LSMessage &message)
{
    bool ret = false;

    std::vector<std::string> solutionsInfo;

    jvalue_ref json_outobj          = jobject_create();
    jvalue_ref json_solutions_array = jarray_create(0);

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    if (pDeviceControl->getSupportedCameraSolutionInfo(solutionsInfo) == DEVICE_OK)
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));

        for (auto solution : solutionsInfo)
        {
            jarray_append(json_solutions_array, jstring_create(solution.c_str()));
        }
        jobject_put(json_outobj, J_CSTR_TO_JVAL("solutions"), json_solutions_array);
        ret = true;
    }
    else
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(false));
        ret = false;
    }

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::getEnabledCameraSolutionInfo(LSMessage &message)
{
    bool ret = false;

    std::vector<std::string> solutionsInfo;

    jvalue_ref json_outobj          = jobject_create();
    jvalue_ref json_solutions_array = jarray_create(0);

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    if (pDeviceControl->getEnabledCameraSolutionInfo(solutionsInfo) == DEVICE_OK)
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(true));

        for (auto solution : solutionsInfo)
        {
            jarray_append(json_solutions_array, jstring_create(solution.c_str()));
        }

        jobject_put(json_outobj, J_CSTR_TO_JVAL("solutions"), json_solutions_array);
        ret = true;
    }
    else
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(false));
        ret = false;
    }

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::enableCameraSolution(LSMessage &message)
{
    bool ret               = false;
    jvalue_ref json_outobj = jobject_create();
    std::vector<std::string> solutionList;

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("solutions"))
    {
        auto obj_solutions = parsed["solutions"];
        size_t count       = obj_solutions.arraySize();
        for (size_t i = 0; i < count; i++)
        {
            std::string name = obj_solutions[i].asString();
            solutionList.push_back(name.c_str());
            PMLOG_INFO(CONST_MODULE_CHS, "enable solution list(%s)", name.c_str());
        }

        if (pDeviceControl->enableCameraSolution(solutionList) == DEVICE_OK)
        {
            ret = true;
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_CHS, "doesn't have solutions key");
        ret = false;
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::disableCameraSolution(LSMessage &message)
{
    bool ret               = false;
    jvalue_ref json_outobj = jobject_create();
    std::vector<std::string> solutionList;

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey("solutions"))
    {
        auto obj_solutions = parsed["solutions"];
        size_t count       = obj_solutions.arraySize();
        for (size_t i = 0; i < count; i++)
        {
            std::string name = obj_solutions[i].asString();
            solutionList.push_back(name.c_str());
            PMLOG_INFO(CONST_MODULE_CHS, "disable solution list(%s)", name.c_str());
        }

        if (pDeviceControl->disableCameraSolution(solutionList) == DEVICE_OK)
        {
            ret = true;
        }
        else
        {
            ret = false;
        }
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_CHS, "doesn't have solutions key");
        ret = false;
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

bool CameraHalService::subscribe(LSMessage &message)
{
    LSError error;
    LSErrorInit(&error);

    bool ret = LSSubscriptionAdd(this->get(), SUBSCRIPTION_KEY, &message, &error);
    PMLOG_INFO(CONST_MODULE_CHS, "LSSubscriptionAdd %s", ret ? "ok" : "failed");
    PMLOG_INFO(CONST_MODULE_CHS, "cnt %d",
               LSSubscriptionGetHandleSubscribersCount(this->get(), SUBSCRIPTION_KEY));
    LSErrorFree(&error);

    jvalue_ref json_outobj = jobject_create();
    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jboolean_create(ret));
    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));

    return ret;
}

#include <gst/gst.h>
int main(int argc, char *argv[])
{
    gst_init(NULL, NULL);

    int c;
    char service_name[MAX_SERVICE_STRING + 1] = {
        '\0',
    };
    bool service_name_specified = false;

    while ((c = getopt(argc, argv, "s:")) != -1)
    {
        switch (c)
        {
        case 's':
            snprintf(service_name, MAX_SERVICE_STRING, "%s", optarg);
            service_name_specified = true;
            break;

        case '?':
            PMLOG_INFO(CONST_MODULE_CHS, "unknown service name");
            break;

        default:
            break;
        }
    }

    if (!service_name_specified)
    {
        PMLOG_INFO(CONST_MODULE_CHS, "service name is not specified");
        return 1;
    }

    try
    {
        CameraHalService CameraHalService(service_name);
    }
    catch (LS::Error &err)
    {
        LSErrorPrint(err, stdout);
        return 1;
    }
    return 0;
}
