// Copyright (c) 2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

/*-----------------------------------------------------------------------------
 #include
 (File Inclusions)
 -- ----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <pbnjson.hpp>
#include "camera_service.h"
#include "service_types.h"
#include "constants.h"
#include "json_parser.h"
#include "json_schema.h"
#include "notifier.h"

const std::string service = "com.webos.service.camera3";
const std::string responsesuccess = "{\"returnValue\": true}";
const std::string responsefailure = "{\"returnValue\": false}";
const std::string responsesubscribed = "{\"returnValue\": true,\"subscribed\": true}";

CameraService::CameraService() : LS::Handle(LS::registerService(service.c_str()))
{
    LS_CATEGORY_BEGIN(CameraService, "/")
    LS_CATEGORY_METHOD(open)
    LS_CATEGORY_METHOD(close)
    LS_CATEGORY_METHOD(getInfo)
    LS_CATEGORY_METHOD(getCameraList)
    LS_CATEGORY_METHOD(getProperties)
    LS_CATEGORY_METHOD(setProperties)
    LS_CATEGORY_METHOD(setFormat)
    LS_CATEGORY_METHOD(startCapture)
    LS_CATEGORY_METHOD(stopCapture)
    LS_CATEGORY_METHOD(startPreview)
    LS_CATEGORY_METHOD(stopPreview)
    LS_CATEGORY_METHOD(getList)
    LS_CATEGORY_END;

    //attach to mainloop and run it
    attachToLoop(main_loop_ptr_.get());

    //sunscribe to pdm client
    Notifier notifier;
    notifier.setLSHandle(this->get());
    notifier.addNotifier(NotifierClient::NOTIFIER_CLIENT_PDM);

    //run the gmainloop
    g_main_loop_run(main_loop_ptr_.get());
}

bool CameraService::open(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);

    //create Open class object and read data from json object after schema validation
    OpenMethod open;
    open.getOpenObject(payload, openSchema);

    DeviceError err_id = DeviceError::DEVICE_OK;

    if (invalid_device_id == open.getCameraId())
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        open.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        //open device here

        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            open.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            open.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
            open.setDeviceHandle(0);
        }
    }

    //create json string now for reply
    std::string output_reply = open.createOpenObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::close(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);

    //create close class object and read data from json object after schema validation
    StopPreviewCaptureCloseMethod obj_close;
    obj_close.getObject(payload, stopCapturePreviewCloseSchema);

    DeviceError err_id = DeviceError::DEVICE_OK;

    int ndev_id = obj_close.getDeviceHandle();

    if (n_invalid_id == ndev_id)
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        obj_close.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::close ndev_id : %d\n", ndev_id);
        //close device here

        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            obj_close.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            obj_close.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
        }
    }

    //create json string now for reply
    std::string output_reply = obj_close.createObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::startPreview(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);
    DeviceError err_id = DeviceError::DEVICE_OK;

    StartPreviewMethod obj_startpreview;
    obj_startpreview.getStartPreviewObject(payload, startPreviewSchema);

    int ndevice_id = obj_startpreview.getDeviceHandle();
    if (n_invalid_id == ndevice_id)
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        obj_startpreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::startPreview DevID : %d\n");
        //start preview here
        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            obj_startpreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            obj_startpreview.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
            obj_startpreview.setKeyValue(0);
        }
    }

    //create json string now for reply
    std::string output_reply = obj_startpreview.createStartPreviewObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::stopPreview(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);
    DeviceError err_id = DeviceError::DEVICE_OK;

    StopPreviewCaptureCloseMethod obj_stoppreview;
    obj_stoppreview.getObject(payload, stopCapturePreviewCloseSchema);

    int ndevice_id = obj_stoppreview.getDeviceHandle();

    if (n_invalid_id == ndevice_id)
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        obj_stoppreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::stopPreview ndevice_id : %d\n", ndevice_id);
        //stop preview here

        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            obj_stoppreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            obj_stoppreview.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
        }
    }

    //create json string now for reply
    std::string output_reply = obj_stoppreview.createObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::startCapture(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);
    DeviceError err_id = DeviceError::DEVICE_OK;

    StartCaptureMethod obj_startcapture;
    obj_startcapture.getStartCaptureObject(payload, startCaptureSchema);

    int ndevice_id = obj_startcapture.getDeviceHandle();

    if (n_invalid_id == ndevice_id)
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        obj_startcapture.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        if ((CameraFormat::CAMERA_FORMAT_JPEG != obj_startcapture.rGetParams().e_format) &&
            (CameraFormat::CAMERA_FORMAT_YUV != obj_startcapture.rGetParams().e_format))
        {
            err_id = DeviceError::DEVICE_ERROR_UNSUPPORTED_FORMAT;
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::startCapture ndevice_id : %d\n", ndevice_id);
            SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::startCapture nImage : %d\n", obj_startcapture.getnImage());

            //capture image here
        }
        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            obj_startcapture.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            obj_startcapture.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
        }
    }

    //create json string now for reply
    std::string output_reply = obj_startcapture.createStartCaptureObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::stopCapture(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);
    DeviceError err_id = DeviceError::DEVICE_OK;

    StopPreviewCaptureCloseMethod obj_stopcapture;
    obj_stopcapture.getObject(payload, stopCapturePreviewCloseSchema);

    int ndevice_id = obj_stopcapture.getDeviceHandle();

    if (n_invalid_id == ndevice_id)
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        obj_stopcapture.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::stopPreview ndevice_id : %d\n", ndevice_id);
        //stop preview here

        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            obj_stopcapture.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            obj_stopcapture.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
        }
    }

    //create json string now for reply
    std::string output_reply = obj_stopcapture.createObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::getInfo(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);
    DeviceError err_id = DeviceError::DEVICE_OK;

    GetInfoMethod obj_getinfo;
    obj_getinfo.getInfoObject(payload, getInfoSchema);

    if (invalid_device_id == obj_getinfo.strGetDeviceId())
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        obj_getinfo.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        //get info here

        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            obj_getinfo.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            obj_getinfo.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
            camera_info_t o_info;
            obj_getinfo.setCameraInfo(o_info);
        }
    }

    //create json string now for reply
    std::string output_reply = obj_getinfo.createInfoObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::getCameraList(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);
    DeviceError err_id = DeviceError::DEVICE_OK;

    GetCameraListMethod obj_getcameralist;
    bool ret = obj_getcameralist.getCameraListObject(payload, getCameraListSchema);

    if (false == ret)
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        obj_getcameralist.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        //get camera list here
        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            obj_getcameralist.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            obj_getcameralist.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
            obj_getcameralist.setCameraCount(0);
        }
    }

    //create json string now for reply
    std::string output_reply = obj_getcameralist.createCameraListObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::getProperties(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);
    DeviceError err_id = DeviceError::DEVICE_OK;

    GetSetPropertiesMethod obj_getproperties;
    obj_getproperties.getPropertiesObject(payload, getPropertiesSchema);

    int devid = obj_getproperties.getDeviceHandle();

    if (n_invalid_id == devid)
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        obj_getproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            obj_getproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            obj_getproperties.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
            camera_properties_t dev_property;
            obj_getproperties.setCameraProperties(dev_property);
        }
    }

    //create json string now for reply
    std::string output_reply = obj_getproperties.createGetPropertiesObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::setProperties(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);
    DeviceError err_id = DeviceError::DEVICE_OK;

    GetSetPropertiesMethod objsetproperties;
    objsetproperties.getSetPropertiesObject(payload, setPropertiesSchema);

    int devid = objsetproperties.getDeviceHandle();

    if (n_invalid_id == devid)
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        objsetproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        //set properties here
        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            objsetproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            objsetproperties.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
        }
    }

    //create json string now for reply
    std::string output_reply = objsetproperties.createSetPropertiesObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::setFormat(LSMessage &message)
{
    const char *payload = LSMessageGetPayload(&message);
    DeviceError err_id = DeviceError::DEVICE_OK;

    SetFormatMethod objsetformat;
    objsetformat.getSetFormatObject(payload, setFormatSchema);

    int devid = objsetformat.getDeviceHandle();

    if (n_invalid_id == devid)
    {
        SRV_LOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
        err_id = DeviceError::DEVICE_ERROR_JSON_PARSING;
        objsetformat.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        //setformat here
        if (DeviceError::DEVICE_OK != err_id)
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
            objsetformat.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            SRV_LOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
            objsetformat.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
        }
    }

    //create json string now for reply
    std::string output_reply = objsetformat.createSetFormatObjectJsonString();
    SRV_LOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::getList(LSMessage &message)
{
    LSError lserror;
    LSErrorInit(&lserror);
    LS::Message request(&message);

    const char *payload = LSMessageGetPayload(&message);

    jerror *json_error = NULL;
    jvalue_ref message_ref = jdom_create(j_cstr_to_buffer(payload), jschema_all(), &json_error);
    if (jis_valid(message_ref))
    {
        bool subscribe = false;
        jboolean_get(jobject_get(message_ref, J_CSTR_TO_BUF("subscribe")), &subscribe);

        if (subscribe)
        {
            request.respond(responsesubscribed.c_str());
        }
        else
        {
            request.respond(responsesuccess.c_str());
        }
    }
    else
    {
        request.respond(responsefailure.c_str());
    }
    j_release(&message_ref);

    return true;
}

int main(int argc, char *argv[])
{
    try
    {
        CameraService camerasrv;
    }
    catch (LS::Error &err)
    {
        LSErrorPrint(err, stdout);
        return 1;
    }
    return 0;
}
