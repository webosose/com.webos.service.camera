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
#include "camera_types.h"
#include "device_controller.h"
#include <pbnjson.hpp>
#include <string>

const char *const CONST_MODULE_CHS = "CameraHalService";
const char *const SUBSCRIPTION_KEY = "cameraHal";

CameraHalService::CameraHalService(const char *service_name)
    : LS::Handle(LS::registerService(service_name)), pCamHandle(NULL)
{
    PMLOG_INFO(CONST_MODULE_CHS, "Start : %s", service_name);

    LS_CATEGORY_BEGIN(CameraHalService, "/")
    LS_CATEGORY_METHOD(createHandle)
    LS_CATEGORY_METHOD(destroyHandle)
    LS_CATEGORY_METHOD(finishProcess)
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
    std::string device_type;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_SUBSYSTEM))
    {
        device_type = parsed[CONST_PARAM_NAME_SUBSYSTEM].asString();
    }

    DEVICE_RETURN_CODE_T ret = pDeviceControl->createHandle(&pCamHandle, device_type);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::destroyHandle(LSMessage &message)
{
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    DEVICE_RETURN_CODE_T ret = pDeviceControl->destroyHandle(pCamHandle);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    g_main_loop_quit(main_loop_ptr_.get());
    return true;
}

bool CameraHalService::finishProcess(LSMessage &message)
{
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    g_main_loop_quit(main_loop_ptr_.get());

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE),
                jnumber_create_i32(DEVICE_OK));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::open(LSMessage &message)
{
    std::string devicenode;
    int ndev_id = 0;
    std::string payload_;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_DEVICE_PATH))
    {
        devicenode = parsed[CONST_PARAM_NAME_DEVICE_PATH].asString();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_CAMERAID))
    {
        ndev_id = parsed[CONST_PARAM_NAME_CAMERAID].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_PAYLOAD))
    {
        payload_ = parsed[CONST_PARAM_NAME_PAYLOAD].asString();
    }

    DEVICE_RETURN_CODE_T ret = pDeviceControl->open(pCamHandle, devicenode, ndev_id, payload_);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::close(LSMessage &message)
{
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    DEVICE_RETURN_CODE_T ret = pDeviceControl->close(pCamHandle);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::startPreview(LSMessage &message)
{
    int pkey               = 0;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    std::string memtype;
    if (parsed.hasKey(CONST_PARAM_NAME_MEMTYPE))
    {
        memtype = parsed[CONST_PARAM_NAME_MEMTYPE].asString();
    }
    PMLOG_INFO(CONST_MODULE_CHS, "memtype(%s)", memtype.c_str());

    DEVICE_RETURN_CODE_T ret =
        pDeviceControl->startPreview(pCamHandle, memtype, &pkey, this->get(), SUBSCRIPTION_KEY);
    if (ret == DEVICE_OK)
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHMKEY), jnumber_create_i32(pkey));
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::stopPreview(LSMessage &message)
{
    int memtype = 0;
    ;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);
    if (parsed.hasKey(CONST_PARAM_NAME_MEMTYPE))
    {
        memtype = parsed[CONST_PARAM_NAME_MEMTYPE].asNumber<int>();
    }

    DEVICE_RETURN_CODE_T ret = pDeviceControl->stopPreview(pCamHandle, memtype);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::captureImage(LSMessage &message)
{
    int ncount = 1;
    CAMERA_FORMAT sformat;
    std::string imagepath;
    std::string mode;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_WIDTH))
    {
        sformat.nWidth = parsed[CONST_PARAM_NAME_WIDTH].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_HEIGHT))
    {
        sformat.nHeight = parsed[CONST_PARAM_NAME_HEIGHT].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_FORMAT))
    {
        sformat.eFormat = (camera_format_t)parsed[CONST_PARAM_NAME_FORMAT].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_IMAGE_PATH))
    {
        imagepath = parsed[CONST_PARAM_NAME_IMAGE_PATH].asString();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_MODE))
    {
        mode = parsed[CONST_PARAM_NAME_MODE].asString();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_NCOUNT))
    {
        ncount = parsed[CONST_PARAM_NAME_NCOUNT].asNumber<int>();
    }

    DEVICE_RETURN_CODE_T ret =
        pDeviceControl->captureImage(pCamHandle, ncount, sformat, imagepath, mode);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::startCapture(LSMessage &message)
{
    CAMERA_FORMAT sformat;
    std::string imagepath;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_WIDTH))
    {
        sformat.nWidth = parsed[CONST_PARAM_NAME_WIDTH].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_HEIGHT))
    {
        sformat.nHeight = parsed[CONST_PARAM_NAME_HEIGHT].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_FORMAT))
    {
        sformat.eFormat = (camera_format_t)parsed[CONST_PARAM_NAME_FORMAT].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_IMAGE_PATH))
    {
        imagepath = parsed[CONST_PARAM_NAME_IMAGE_PATH].asString();
    }

    DEVICE_RETURN_CODE_T ret = pDeviceControl->startCapture(pCamHandle, sformat, imagepath);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::stopCapture(LSMessage &message)
{
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    DEVICE_RETURN_CODE_T ret = pDeviceControl->stopCapture(pCamHandle);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    return true;
}

bool CameraHalService::getDeviceProperty(LSMessage &message)
{
    CAMERA_PROPERTIES_T oparams;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    DEVICE_RETURN_CODE_T ret = pDeviceControl->getDeviceProperty(pCamHandle, &oparams);
    if (ret == DEVICE_OK)
    {
        for (int i = 0; i < PROPERTY_END; i++)
        {
            jvalue_ref json_outqueryparams = jobject_create();

            for (int j = 0; j < QUERY_END; j++)
            {
                if (oparams.stGetData.data[i][j] != CONST_PARAM_DEFAULT_VALUE)
                {
                    jobject_put(json_outqueryparams, jstring_create(getQueryString(j).c_str()),
                                jnumber_create_i32(oparams.stGetData.data[i][j]));
                }
            }
            jobject_put(json_outobj, jstring_create(getParamString(i).c_str()),
                        json_outqueryparams);
        }
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::setDeviceProperty(LSMessage &message)
{
    CAMERA_PROPERTIES_T inparams;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    // put the value into device property
    for (int i = 0; i < PROPERTY_END; i++)
    {
        if (parsed.hasKey(getParamString(i).c_str()))
        {
            inparams.stGetData.data[i][QUERY_VALUE] =
                parsed[getParamString(i).c_str()].asNumber<int>();
        }
    }

    DEVICE_RETURN_CODE_T ret = pDeviceControl->setDeviceProperty(pCamHandle, &inparams);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::setFormat(LSMessage &message)
{
    CAMERA_FORMAT sformat;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_WIDTH))
    {
        sformat.nWidth = parsed[CONST_PARAM_NAME_WIDTH].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_HEIGHT))
    {
        sformat.nHeight = parsed[CONST_PARAM_NAME_HEIGHT].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_FPS))
    {
        sformat.nFps = parsed[CONST_PARAM_NAME_FPS].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_FORMAT))
    {
        int eFormat     = parsed[CONST_PARAM_NAME_FORMAT].asNumber<int>();
        sformat.eFormat = (camera_format_t)eFormat;
    }

    DEVICE_RETURN_CODE_T ret = pDeviceControl->setFormat(pCamHandle, sformat);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::getFormat(LSMessage &message)
{
    CAMERA_FORMAT sformat;
    jvalue_ref json_outobj = jobject_create();
    auto *payload          = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    DEVICE_RETURN_CODE_T ret = pDeviceControl->getFormat(pCamHandle, &sformat);
    if (ret == DEVICE_OK)
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WIDTH),
                    jnumber_create_i32(sformat.nWidth));
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HEIGHT),
                    jnumber_create_i32(sformat.nHeight));
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FPS),
                    jnumber_create_i32(sformat.nFps));
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT),
                    jnumber_create_i32((int)sformat.eFormat));
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::getDeviceInfo(LSMessage &message)
{
    std::string strdevicenode;
    std::string device_type;
    camera_device_info_t cameraInfo;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_DEVICE_PATH))
    {
        strdevicenode = parsed[CONST_PARAM_NAME_DEVICE_PATH].asString();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_SUBSYSTEM))
    {
        device_type = parsed[CONST_PARAM_NAME_SUBSYSTEM].asString();
    }
    PMLOG_INFO(CONST_MODULE_CHS, "device_type(%s)", device_type.c_str());

    DEVICE_RETURN_CODE_T ret =
        pDeviceControl->getDeviceInfo(strdevicenode, device_type, &cameraInfo);
    if (ret == DEVICE_OK)
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEVICE_TYPE),
                    jnumber_create_i32(cameraInfo.n_devicetype));
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BUILTIN),
                    jnumber_create_i32(cameraInfo.b_builtin));

        jvalue_ref json_resolutionobj                   = jobject_create();
        std::vector<camera_resolution_t> resolutionInfo = cameraInfo.stResolution;
        for (auto &i : resolutionInfo)
        {
            jvalue_ref json_resolution_array = jarray_create(0);
            std::vector<std::string> resStr  = i.c_res;
            for (auto &s : resStr)
            {
                jarray_append(json_resolution_array, jstring_create(s.c_str()));
                PMLOG_INFO(CONST_MODULE_CHS, "resFmt(%s) resStr(%s)",
                           getResolutionString(i.e_format).c_str(), s.c_str());
            }
            PMLOG_INFO(CONST_MODULE_CHS, "resFmt(%s)", getResolutionString(i.e_format).c_str());
            jobject_put(json_resolutionobj, jstring_create(getResolutionString(i.e_format).c_str()),
                        json_resolution_array);
        }
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RESOLUTION), json_resolutionobj);
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    g_main_loop_quit(main_loop_ptr_.get());
    return true;
}

bool CameraHalService::registerClient(LSMessage &message)
{
    int pid       = 0;
    int sig       = 0;
    int devhandle = 0;
    std::string payload_;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_CLIENT_PROCESS_ID))
    {
        pid = parsed[CONST_CLIENT_PROCESS_ID].asNumber<int>();
    }

    if (parsed.hasKey(CONST_CLIENT_SIGNAL_NUM))
    {
        sig = parsed[CONST_CLIENT_SIGNAL_NUM].asNumber<int>();
    }

    if (parsed.hasKey(CONST_PARAM_NAME_DEVHANDLE))
    {
        devhandle = parsed[CONST_PARAM_NAME_DEVHANDLE].asNumber<int>();
    }

    std::string outmsg;
    bool ret = pDeviceControl->registerClient(pid, sig, devhandle, outmsg);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_OUTMSG),
                jstring_create(outmsg.c_str()));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::unregisterClient(LSMessage &message)
{
    int pid = 0;
    std::string payload_;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_CLIENT_PROCESS_ID))
    {
        pid = parsed[CONST_CLIENT_PROCESS_ID].asNumber<int>();
    }

    std::string outmsg;
    bool ret = pDeviceControl->unregisterClient(pid, outmsg);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_OUTMSG),
                jstring_create(outmsg.c_str()));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::isRegisteredClient(LSMessage &message)
{
    int devhandle = 0;
    std::string payload_;
    jvalue_ref json_outobj = jobject_create();

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_DEVHANDLE))
    {
        devhandle = parsed[CONST_PARAM_NAME_DEVHANDLE].asNumber<int>();
    }

    std::string outmsg;
    bool ret = pDeviceControl->isRegisteredClient(devhandle);

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::requestPreviewCancel(LSMessage &message)
{
    jvalue_ref json_outobj = jobject_create();

    pDeviceControl->requestPreviewCancel();

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(true));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::getSupportedCameraSolutionInfo(LSMessage &message)
{
    std::vector<std::string> solutionsInfo;

    jvalue_ref json_outobj          = jobject_create();
    jvalue_ref json_solutions_array = jarray_create(0);

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    DEVICE_RETURN_CODE_T ret = pDeviceControl->getSupportedCameraSolutionInfo(solutionsInfo);
    if (ret == DEVICE_OK)
    {
        for (auto solution : solutionsInfo)
        {
            jarray_append(json_solutions_array, jstring_create(solution.c_str()));
        }
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SOLUTIONS), json_solutions_array);
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::getEnabledCameraSolutionInfo(LSMessage &message)
{
    std::vector<std::string> solutionsInfo;

    jvalue_ref json_outobj          = jobject_create();
    jvalue_ref json_solutions_array = jarray_create(0);

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    DEVICE_RETURN_CODE_T ret = pDeviceControl->getEnabledCameraSolutionInfo(solutionsInfo);
    if (ret == DEVICE_OK)
    {
        for (auto solution : solutionsInfo)
        {
            jarray_append(json_solutions_array, jstring_create(solution.c_str()));
        }

        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SOLUTIONS), json_solutions_array);
    }
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::enableCameraSolution(LSMessage &message)
{
    jvalue_ref json_outobj   = jobject_create();
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    std::vector<std::string> solutionList;

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_SOLUTIONS))
    {
        auto obj_solutions = parsed[CONST_PARAM_NAME_SOLUTIONS];
        size_t count       = obj_solutions.arraySize();
        for (size_t i = 0; i < count; i++)
        {
            std::string name = obj_solutions[i].asString();
            solutionList.push_back(name.c_str());
            PMLOG_INFO(CONST_MODULE_CHS, "enable solution list(%s)", name.c_str());
        }

        ret = pDeviceControl->enableCameraSolution(solutionList);
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_CHS, "doesn't have solutions key");
        ret = DEVICE_ERROR_PARAM_IS_MISSING;
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
}

bool CameraHalService::disableCameraSolution(LSMessage &message)
{
    jvalue_ref json_outobj   = jobject_create();
    DEVICE_RETURN_CODE_T ret = DEVICE_OK;
    std::vector<std::string> solutionList;

    auto *payload = LSMessageGetPayload(&message);
    PMLOG_INFO(CONST_MODULE_CHS, "payload %s", payload);

    pbnjson::JValue parsed = pbnjson::JDomParser::fromString(payload);

    if (parsed.hasKey(CONST_PARAM_NAME_SOLUTIONS))
    {
        auto obj_solutions = parsed[CONST_PARAM_NAME_SOLUTIONS];
        size_t count       = obj_solutions.arraySize();
        for (size_t i = 0; i < count; i++)
        {
            std::string name = obj_solutions[i].asString();
            solutionList.push_back(name.c_str());
            PMLOG_INFO(CONST_MODULE_CHS, "disable solution list(%s)", name.c_str());
        }

        ret = pDeviceControl->disableCameraSolution(solutionList);
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_CHS, "doesn't have solutions key");
        ret = DEVICE_ERROR_PARAM_IS_MISSING;
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNCODE), jnumber_create_i32(ret));

    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return true;
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
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(ret));
    LS::Message request(&message);
    request.respond(jvalue_stringify(json_outobj));
    PMLOG_INFO(CONST_MODULE_CHS, "response message : %s", jvalue_stringify(json_outobj));

    j_release(&json_outobj);

    return ret;
}

#include <gst/gst.h>
int main(int argc, char *argv[])
{
    PMLOG_INFO(CONST_MODULE_CHS, "start");
    gst_init(NULL, NULL);

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
            PMLOG_INFO(CONST_MODULE_CHS, "unknown service name");
            break;

        default:
            break;
        }
    }

    if (serviceName.empty())
    {
        PMLOG_INFO(CONST_MODULE_CHS, "service name is not specified");
        return 1;
    }

    try
    {
        CameraHalService CameraHalService(serviceName.c_str());
    }
    catch (LS::Error &err)
    {
        LSErrorPrint(err, stdout);
        return 1;
    }

    PMLOG_INFO(CONST_MODULE_CHS, "end");
    return 0;
}
