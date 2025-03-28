// Copyright (c) 2019-2021 LG Electronics, Inc.
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
 (File Inclusions)
 ----------------------------------------------------------------------------*/
#define LOG_TAG "CameraService"
#define GET_CLIENT_INFO // FIXME: This is a temporary code and will be fixed. [WRR-7088]
#include "camera_service.h"
#include "addon.h"
#include "camera_constants.h"
#include "camera_types.h"
#include "command_manager.h"
#include "device_manager.h"
#include "json_schema.h"
#include "notifier.h"
#include "whitelist_checker.h"
#include <pbnjson.hpp>
#include <signal.h>
#include <string>

struct sigaction sigact_service_crash;
extern "C" void signal_handler_service_crash(int sig);
bool install_handler_service_crash() noexcept;

CameraService::CameraService() : LS::Handle(LS::registerService(cstr_uricameramain.c_str()))
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
    LS_CATEGORY_METHOD(capture)
    LS_CATEGORY_METHOD(startCamera)
    LS_CATEGORY_METHOD(stopCamera)
    LS_CATEGORY_METHOD(startPreview)
    LS_CATEGORY_METHOD(stopPreview)
    LS_CATEGORY_METHOD(getEventNotification)
    LS_CATEGORY_METHOD(getFd)
    LS_CATEGORY_METHOD(setSolutions)
    LS_CATEGORY_METHOD(getSolutions)
    LS_CATEGORY_METHOD(getFormat)
    LS_CATEGORY_END;

    // attach to mainloop and run it
    attachToLoop(main_loop_ptr_.get());

    // load/initialize addon before subscription to pdm client
    pAddon_ = std::make_shared<AddOn>();
    if (pAddon_ && pAddon_->hasImplementation())
    {
        pAddon_->initialize(this->get());
    }
    CommandManager::getInstance().setAddon(pAddon_);
    DeviceManager::getInstance().setAddon(pAddon_);

    // set LS handle for device manager
    DeviceManager::getInstance().setLSHandle(this->get());

    // subscribe to pdm client
    Notifier notifier;
    notifier.setLSHandle(this->get());
    notifier.addNotifiers(main_loop_ptr_.get());

    // run the gmainloop
    g_main_loop_run(main_loop_ptr_.get());
}

int CameraService::getId(const std::string &cameraid)
{
    int num               = n_invalid_id;
    std::string camerastr = "camera";
    std::string extractedNumbers;

    if ((cameraid.compare(0, camerastr.length(), camerastr)) == 0)
    {
        extractedNumbers = cameraid.substr(camerastr.length());
    }
    else
    {
        return num;
    }

    if (!extractedNumbers.empty())
    {
        try
        {
            num = std::stoi(extractedNumbers);
        }
        catch (const std::exception &e)
        {
            PLOGE("Error: %s", e.what());
        }
    }
    return num;
}

void CameraService::createEventMessage(EventType etype, void *p_old_data, int devhandle,
                                       std::string event_key)
{
    if (event_obj.getSubscribeCount(this->get(), event_key) > 0)
    {
        if (EventType::EVENT_TYPE_FORMAT == etype)
        {
            PLOGI("EVENT_TYPE_FORMAT Event received\n");
            CAMERA_FORMAT newformat;
            CommandManager::getInstance().getFormat(devhandle, &newformat);

            auto *p_old_format = static_cast<CAMERA_FORMAT *>(p_old_data);

            if (*p_old_format != newformat)
            {
                auto *p_cur_data = static_cast<void *>(&newformat);
                event_obj.eventReply(this->get(), event_key.c_str(), etype, p_cur_data, p_old_data);
            }
        }
        else if (EventType::EVENT_TYPE_PROPERTIES == etype)
        {
            PLOGI("EVENT_TYPE_PROPERTIES Event received\n");
            CAMERA_PROPERTIES_T new_property;
            CommandManager::getInstance().getProperty(devhandle, &new_property);

            auto *p_old_property = static_cast<CAMERA_PROPERTIES_T *>(p_old_data);

            if (*p_old_property != new_property)
            {
                auto *p_cur_data = static_cast<void *>(&new_property);
                event_obj.eventReply(this->get(), event_key.c_str(), etype, p_cur_data, p_old_data);
            }
        }
        else
        {
            PLOGI("Unknown Event received\n");
        }
    }
}

bool CameraService::open(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);

    PLOGI("payload %s", payload);
    // create Open class object and read data from json object after schema validation
    OpenMethod open;
    open.getOpenObject(payload, openSchema);

    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    std::string app_id = open.getAppId();
    PLOGI("appId : %s", app_id.c_str());
    // camera id & appId validation check
    if (cstr_invaliddeviceid == open.getCameraId())
    {
        PLOGE("DEVICE_ERROR_JSON_PARSING");
        err_id = DEVICE_ERROR_JSON_PARSING;
        open.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else if (pAddon_ && pAddon_->hasImplementation() && !pAddon_->isAppPermission(app_id))
    {
        PLOGE("CameraService::App Permission Fail\n");
        err_id = DEVICE_ERROR_APP_PERMISSION;
        open.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        int ndev_id = getId(open.getCameraId());
        PLOGI("device Id %d\n", ndev_id);
        std::string app_priority = open.getAppPriority();
        PLOGI("priority : %s \n", app_priority.c_str());
        int ndevice_handle = n_invalid_id;

        // open camera device and save fd
        err_id = CommandManager::getInstance().open(ndev_id, &ndevice_handle, std::move(app_id),
                                                    std::move(app_priority));
        if (DEVICE_OK != err_id)
        {
            PLOGE("err_id != DEVICE_OK\n");
            open.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
        }
        else
        {
            PLOGD("err_id == DEVICE_OK\n");
            open.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
            open.setDeviceHandle(ndevice_handle);

            addClientWatcher(this->get(), &message, ndevice_handle);
        }
    }

    // create json string now for LS reply
    std::string output_reply = open.createOpenObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::close(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    // create close class object and read data from json object after schema validation
    StopCameraPreviewCaptureCloseMethod obj_close;
    obj_close.getObject(payload, stopCaptureCameraPreviewCloseSchema);

    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    int ndevhandle = obj_close.getDeviceHandle();

    err_id = validateClient(&message, ndevhandle);

    // close the device if there is no error on previous checks
    if (err_id == DEVICE_OK)
    {
        // close device here
        err_id = CommandManager::getInstance().close(ndevhandle);

        if (DEVICE_OK != err_id)
        {
            PLOGD("err_id != DEVICE_OK\n");
        }
        else
        {
            PLOGD("err_id == DEVICE_OK\n");
        }
    }

    std::string errorMsg = getErrorString(err_id);
    obj_close.setMethodReply(err_id == DEVICE_OK, (int)err_id, std::move(errorMsg));

    // create json string now for reply
    std::string output_reply = obj_close.createObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::startCamera(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    StartCameraMethod obj_startcamera;
    obj_startcamera.getStartCameraObject(payload, startCameraSchema);

    int ndevhandle = obj_startcamera.getDeviceHandle();

    err_id = validateClient(&message, ndevhandle);

    if (err_id == DEVICE_OK)
    {
        // start preview here
        err_id = CommandManager::getInstance().startCamera(ndevhandle, this->get());
        if (DEVICE_OK != err_id)
        {
            PLOGE("err_id != DEVICE_OK\n");
        }
        else
        {
            PLOGI("err_id == DEVICE_OK\n");
        }
    }

    obj_startcamera.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));

    // create json string now for reply
    std::string output_reply = obj_startcamera.createStartCameraObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());
    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::stopCamera(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    StopCameraPreviewCaptureCloseMethod obj_stopcamera;
    obj_stopcamera.getObject(payload, stopCaptureCameraPreviewCloseSchema);

    int ndevhandle = obj_stopcamera.getDeviceHandle();

    err_id = validateClient(&message, ndevhandle);

    if (err_id == DEVICE_OK)
    {
        // stop preview here
        err_id = CommandManager::getInstance().stopCamera(ndevhandle);

        if (DEVICE_OK != err_id)
        {
            PLOGD("err_id != DEVICE_OK\n");
        }
        else
        {
            PLOGD("err_id == DEVICE_OK\n");
        }
    }

    obj_stopcamera.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));
    // create json string now for reply
    std::string output_reply = obj_stopcamera.createObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::startPreview(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;
    camera_display_source_t dispType;

    StartPreviewMethod obj_startpreview;
    obj_startpreview.getStartPreviewObject(payload, startPreviewSchema);

    int ndevhandle = obj_startpreview.getDeviceHandle();

    err_id = validateClient(&message, ndevhandle);

    if (err_id == DEVICE_OK)
    {
        // start preview here
        dispType = obj_startpreview.rGetDpyParams();

        err_id = CommandManager::getInstance().startPreview(ndevhandle, dispType.str_window_id,
                                                            this->get());

        if (DEVICE_OK != err_id)
        {
            PLOGE("err_id != DEVICE_OK\n");
        }
        else
        {
            PLOGI("err_id == DEVICE_OK\n");
        }
    }

    obj_startpreview.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));

    // create json string now for reply
    std::string output_reply = obj_startpreview.createStartPreviewObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());
    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::stopPreview(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    StopCameraPreviewCaptureCloseMethod obj_stoppreview;
    obj_stoppreview.getObject(payload, stopCaptureCameraPreviewCloseSchema);

    int ndevhandle = obj_stoppreview.getDeviceHandle();

    err_id = validateClient(&message, ndevhandle);

    if (err_id == DEVICE_OK)
    {
        // stop preview here
        err_id = CommandManager::getInstance().stopPreview(ndevhandle);

        if (DEVICE_OK != err_id)
        {
            PLOGD("err_id != DEVICE_OK\n");
        }
        else
        {
            PLOGD("err_id == DEVICE_OK\n");
        }
    }

    obj_stoppreview.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));
    // create json string now for reply
    std::string output_reply = obj_stoppreview.createObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::startCapture(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    StartCaptureMethod obj_startcapture;
    obj_startcapture.getStartCaptureObject(payload, startCaptureSchema);

    int ndevhandle = obj_startcapture.getDeviceHandle();

    err_id = validateClient(&message, ndevhandle);

    if (err_id == DEVICE_OK)
    {
        if ((CAMERA_FORMAT_JPEG != obj_startcapture.rGetParams().eFormat) &&
            (CAMERA_FORMAT_YUV != obj_startcapture.rGetParams().eFormat))
        {
            err_id = DEVICE_ERROR_UNSUPPORTED_FORMAT;
        }
        else if (obj_startcapture.strGetCaptureMode() == cstr_burst &&
                 obj_startcapture.getnImage() < 1)
        {
            err_id = DEVICE_ERROR_JSON_PARSING;
        }
        else
        {
            PLOGI("ndevhandle %d\n", ndevhandle);
            PLOGI("path: %s\n", obj_startcapture.getImagePath().c_str());
            PLOGI("mode: %s\n", obj_startcapture.strGetCaptureMode().c_str());
            PLOGI("nImage : %d\n", obj_startcapture.getnImage());

            uid_t requestor_uid = -1;
#if DAC_ENABLED
            requestor_uid = LSMessageGetSenderUid(&message);
            PLOGI("uid : %d\n", requestor_uid);
#endif
            err_id = CommandManager::getInstance().startCapture(
                ndevhandle, obj_startcapture.rGetParams(), obj_startcapture.getImagePath(),
                obj_startcapture.strGetCaptureMode(), obj_startcapture.getnImage(), requestor_uid);
        }
        if (DEVICE_OK != err_id)
        {
            PLOGD("err_id != DEVICE_OK\n");
        }
        else
        {
            PLOGD("err_id == DEVICE_OK\n");
        }
    }

    obj_startcapture.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));
    // create json string now for reply
    std::string output_reply = obj_startcapture.createStartCaptureObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::stopCapture(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    StopCameraPreviewCaptureCloseMethod obj_stopcapture;
    obj_stopcapture.getObject(payload, stopCaptureCameraPreviewCloseSchema);

    int ndevhandle = obj_stopcapture.getDeviceHandle();

    err_id = validateClient(&message, ndevhandle);

    if (err_id == DEVICE_OK)
    {
        // stop capture here
        err_id = CommandManager::getInstance().stopCapture(ndevhandle);

        if (DEVICE_OK != err_id)
        {
            PLOGD("err_id != DEVICE_OK\n");
        }
        else
        {
            PLOGD("err_id == DEVICE_OK\n");
        }
    }

    obj_stopcapture.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));
    // create json string now for reply
    std::string output_reply = obj_stopcapture.createObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::capture(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;
    const int max_capture       = 30;

    CaptureMethod obj_capture;
    obj_capture.getCaptureObject(payload, captureSchema);

    int ndevhandle = obj_capture.getDeviceHandle();
    std::vector<std::string> capturedFileNames;

    err_id = validateClient(&message, ndevhandle);

    if (err_id == DEVICE_OK)
    {
        PLOGI("ndevhandle %d\n", ndevhandle);
        PLOGI("nImage : %d\n", obj_capture.getnImage());
        PLOGI("path: %s\n", obj_capture.getImagePath().c_str());

        if (obj_capture.getnImage() > 0 && obj_capture.getnImage() <= max_capture)
        {
            uid_t requestor_uid = -1;

#if DAC_ENABLED
            requestor_uid = LSMessageGetSenderUid(&message);
            PLOGI("uid : %d\n", requestor_uid);
#endif

            // capture image here
            err_id = CommandManager::getInstance().capture(ndevhandle, obj_capture.getnImage(),
                                                           obj_capture.getImagePath(),
                                                           capturedFileNames, requestor_uid);
        }
        else
        {
            err_id = DEVICE_ERROR_OUT_OF_PARAM_RANGE;
        }
    }

    if (DEVICE_OK != err_id)
    {
        PLOGD("err_id != DEVICE_OK\n");
    }
    else
    {
        PLOGD("err_id == DEVICE_OK\n");
    }

    obj_capture.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));
    // create json string now for reply
    std::string output_reply = obj_capture.createCaptureObjectJsonString(capturedFileNames);
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::getInfo(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;
    bool supported              = false;

    GetInfoMethod obj_getinfo;
    obj_getinfo.getInfoObject(payload, getInfoSchema);

    if (cstr_invaliddeviceid == obj_getinfo.strGetDeviceId())
    {
        PLOGI("DEVICE_ERROR_JSON_PARSING");
        err_id = DEVICE_ERROR_JSON_PARSING;
        obj_getinfo.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        // get info here
        camera_device_info_t o_camerainfo;

        int ndev_id = getId(obj_getinfo.strGetDeviceId());
        PLOGI("device Id %d\n", ndev_id);

        err_id = CommandManager::getInstance().getDeviceInfo(ndev_id, &o_camerainfo);

        if (DEVICE_OK != err_id)
        {
            PLOGD("err_id != DEVICE_OK\n");
            obj_getinfo.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                       getErrorString(err_id));
        }
        else
        {
            PLOGD("err_id == DEVICE_OK\n");
            obj_getinfo.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
            obj_getinfo.setCameraInfo(o_camerainfo);

            if (pAddon_ && pAddon_->hasImplementation())
            {
                supported = pAddon_->isSupportedCamera(o_camerainfo.str_productid,
                                                       o_camerainfo.str_vendorid);
            }
            else
            {
                supported = WhitelistChecker::isSupportedCamera(o_camerainfo.str_productid,
                                                                o_camerainfo.str_vendorid);
            }
        }
    }

    // create json string now for reply
    std::string output_reply = obj_getinfo.createInfoObjectJsonString(supported);
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::getCameraList(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    // create CameraList class object and read data from json object after schema validation
    GetCameraListMethod obj_getcameralist;
    bool ret = obj_getcameralist.getCameraListObject(payload, getCameraListSchema);

    if (false == ret)
    {
        PLOGI("DEVICE_ERROR_JSON_PARSING");
        err_id = DEVICE_ERROR_JSON_PARSING;
        obj_getcameralist.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                         getErrorString(err_id));
    }
    else
    {
        // get camera list here
        std::vector<int> idList;
        err_id = CommandManager::getInstance().getDeviceList(idList);

        bool bsubscribed =
            event_obj.addSubscription(this->get(), CONST_EVENT_KEY_CAMERA_LIST, message);
        PLOGI("bsubscribed (%d) \n", bsubscribed);
        obj_getcameralist.setSubcribed(bsubscribed);

        if (DEVICE_OK != err_id)
        {
            PLOGD("err_id != DEVICE_OK");
            obj_getcameralist.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                             getErrorString(err_id));
        }
        else
        {
            PLOGD("err_id == DEVICE_OK");
            obj_getcameralist.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id,
                                             getErrorString(err_id));
            obj_getcameralist.setCameraCount(idList.size());
            for (std::size_t i = 0; i < idList.size(); i++)
            {
                obj_getcameralist.setCameraList(
                    CONST_DEVICE_NAME_CAMERA + std::to_string(idList[i]), i);
            }
        }
    }

    // create json string now for reply
    std::string output_reply = obj_getcameralist.createCameraListObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::getProperties(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    GetSetPropertiesMethod obj_getproperties;
    obj_getproperties.getPropertiesObject(payload, getPropertiesSchema);

    int ndevhandle = n_invalid_id;
    int ncamId     = getId(obj_getproperties.getCameraId());

    PLOGI("ncamId (%d) \n", ncamId);

    if (n_invalid_id == ncamId)
    {
        err_id = DEVICE_ERROR_WRONG_PARAM;
        PLOGI("err_id(%d)\n", err_id);
        obj_getproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                         getErrorString(err_id));
    }
    else
    {
        // add subscription
        std::string event_key = CONST_EVENT_KEY_PROPERTIES;
        event_key += "_";
        event_key += obj_getproperties.getCameraId();
        bool bsubscribed = event_obj.addSubscription(this->get(), std::move(event_key), message);
        PLOGI("bsubscribed (%d) \n", bsubscribed);
        obj_getproperties.setSubcribed(bsubscribed);

        ndevhandle = CommandManager::getInstance().getCameraHandle(ncamId);
        PLOGI("devhandel by camera(%d) is (%d)\n", ncamId, ndevhandle);

        if (n_invalid_id != ndevhandle)
        {
            err_id = validateClient(&message, ndevhandle);

            if (err_id == DEVICE_OK)
            {
                // get properties here
                CAMERA_PROPERTIES_T dev_property;
                err_id = CommandManager::getInstance().getProperty(ndevhandle, &dev_property);

                if (DEVICE_OK != err_id)
                {
                    PLOGD("err_id != DEVICE_OK\n");
                }
                else
                {
                    PLOGD("err_id == DEVICE_OK\n");
                    obj_getproperties.setCameraProperties(dev_property);
                }
            }
        }
        else
        {
            PLOGI("DEVICE_ERROR_DEVICE_IS_NOT_OPENED");
            err_id = DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
        }
    }

    obj_getproperties.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));
    // create json string now for reply
    std::string output_reply = obj_getproperties.createGetPropertiesObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::setProperties(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    GetSetPropertiesMethod objsetproperties;
    objsetproperties.getSetPropertiesObject(payload, setPropertiesSchema);

    int ndevhandle = objsetproperties.getDeviceHandle();

    err_id = validateClient(&message, ndevhandle);

    if (err_id == DEVICE_OK)
    {
        // check params object is empty or not
        if (objsetproperties.isParamsEmpty(payload, setPropertiesSchema))
        {
            PLOGI("Params object is empty\n");
            err_id = DEVICE_ERROR_WRONG_PARAM;
        }
        else
        {
            // get old properties before setting new
            CAMERA_PROPERTIES_T old_property;
            std::string event_key =
                event_obj.getEventKeyWithId(ndevhandle, CONST_EVENT_KEY_PROPERTIES);
            if (event_obj.getSubscribeCount(this->get(), event_key) > 0)
            {
                CommandManager::getInstance().getProperty(ndevhandle, &old_property);
            }
            // set properties here
            CAMERA_PROPERTIES_T oParams = objsetproperties.rGetCameraProperties();
            PLOGI("ndevhandle %d\n", ndevhandle);
            err_id = CommandManager::getInstance().setProperty(ndevhandle, &oParams);
            if (DEVICE_OK != err_id)
            {
                PLOGD("err_id != DEVICE_OK\n");
            }
            else
            {
                PLOGD("err_id == DEVICE_OK\n");
                // check if new properties are different from saved properties
                auto *p_olddata = static_cast<void *>(&old_property);
                createEventMessage(EventType::EVENT_TYPE_PROPERTIES, p_olddata, ndevhandle,
                                   std::move(event_key));
            }
        }
    }

    objsetproperties.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));
    // create json string now for reply
    std::string output_reply = objsetproperties.createSetPropertiesObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::setFormat(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    SetFormatMethod objsetformat;
    objsetformat.getSetFormatObject(payload, setFormatSchema);

    int ndevhandle = objsetformat.getDeviceHandle();

    err_id = validateClient(&message, ndevhandle);

    if (err_id == DEVICE_OK)
    {
        // get saved format of the device
        CAMERA_FORMAT savedformat;
        std::string event_key = event_obj.getEventKeyWithId(ndevhandle, CONST_EVENT_KEY_FORMAT);

        if (event_obj.getSubscribeCount(this->get(), event_key) > 0)
        {
            CommandManager::getInstance().getFormat(ndevhandle, &savedformat);
        }
        // setformat here
        PLOGI("ndevhandle %d\n", ndevhandle);
        CAMERA_FORMAT sformat = objsetformat.rGetCameraFormat();
        err_id                = CommandManager::getInstance().setFormat(ndevhandle, sformat);
        if (DEVICE_OK != err_id)
        {
            PLOGD("err_id != DEVICE_OK\n");
        }
        else
        {
            PLOGD("err_id == DEVICE_OK\n");
            // check if new format settings are different from saved format settings
            // get saved format of the device
            auto *p_olddata = static_cast<void *>(&savedformat);
            createEventMessage(EventType::EVENT_TYPE_FORMAT, p_olddata, ndevhandle,
                               std::move(event_key));
        }
    }

    objsetformat.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));
    // create json string now for reply
    std::string output_reply = objsetformat.createSetFormatObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::getEventNotification(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    EventNotificationMethod obj_jsonparser;
    obj_jsonparser.getEventObject(payload, getEventNotificationSchema);

    if (obj_jsonparser.getIsErrorFromParam())
    {
        PLOGI("DEVICE_ERROR_WRONG_PARAM");
        err_id = DEVICE_ERROR_WRONG_PARAM;
        obj_jsonparser.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        bool return_val =
            event_obj.addSubscription(this->get(), CONST_EVENT_KEY_STREAMING_FAULT, message);
        return_val = event_obj.addSubscription(this->get(), CONST_EVENT_KEY_CAPTURE_FAULT, message);

        obj_jsonparser.setSubcribed(return_val);
        obj_jsonparser.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
    }
    std::string output_reply = obj_jsonparser.createObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

bool CameraService::getFd(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);
    PLOGI("payload %s", payload);

    LS::Message request(&message);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    int shmfd = -1;
    GetFdMethod obj_getfd;
    obj_getfd.getObject(payload, getFdSchema);

    int ndevhandle = obj_getfd.getDeviceHandle();

    // handle validation check
    if (ndevhandle == n_invalid_id)
        err_id = DEVICE_ERROR_JSON_PARSING;

    if (err_id == DEVICE_OK)
    {
        std::string type = obj_getfd.getType();
        PLOGI("ndevhandle %d type %s", ndevhandle, type.c_str());
        err_id = CommandManager::getInstance().getFd(ndevhandle, type, &shmfd);

        if (err_id == DEVICE_OK)
        {
            obj_getfd.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));

            // create json string now for reply
            std::string output_reply = obj_getfd.createObjectJsonString();
            PLOGI("output_reply %s\n", output_reply.c_str());

            LS::Payload response_payload(output_reply.c_str());
            response_payload.attachFd(shmfd); // attach a fd here
            request.respond(std::move(response_payload));
            return true;
        }
    }
    obj_getfd.setMethodReply(err_id == DEVICE_OK, (int)err_id, getErrorString(err_id));
    // create json string now for reply
    std::string output_reply = obj_getfd.createObjectJsonString();
    PLOGI("output_reply %s\n", output_reply.c_str());

    request.respond(output_reply.c_str());
    return true;
}

bool CameraService::addClientWatcher(LSHandle *handle, LSMessage *message, int ndevice_handle)
{
    const char *clientName       = LSMessageGetSenderServiceName(message);
    const char *unique_client_id = LSMessageGetSender(message);

    if (clientName != NULL)
    {
        PLOGI("clientName: %s\n", clientName);
        if (strstr(clientName, "com.webos.lunasend-") != NULL)
        {
            PLOGE("can not add: %s\n", clientName);
            return false;
        }
    }

    PLOGI("ndevice_handle: %d", ndevice_handle);
    PLOGI("unique_client_id: %s\n", unique_client_id);

    if (!CommandManager::getInstance().setClientDevice(ndevice_handle, unique_client_id))
    {
        PLOGI("setClientDevice fail! : %d", ndevice_handle);
        return false;
    }

    if (clientCookieMap_.find(unique_client_id) != clientCookieMap_.end())
    {
        PLOGI("already watched: %s \n", unique_client_id);
        return false;
    }

    const auto [clientCookie, success] =
        clientCookieMap_.insert(std::make_pair(unique_client_id, nullptr));

    if (clientCookie == clientCookieMap_.end())
    {
        PLOGE("clientCookie is invalid");
        return false;
    }

    auto func = [](LSHandle *input_handle, const char *service_name, bool connected,
                   void *ctx) -> bool
    {
        CameraService *self = static_cast<CameraService *>(ctx);
        auto cookie         = self->clientCookieMap_.find(service_name);

        if (cookie == self->clientCookieMap_.end())
        {
            PLOGE("can not find service_name: %s \n", service_name);
            return true;
        }

        if (!connected)
        {
            PLOGI("disconnect:%s\n", service_name);

            CommandManager::getInstance().closeClientDevice(service_name);

            if (!LSCancelServerStatus(input_handle, cookie->second, nullptr))
            {
                PLOGE("LSCancelServerStatus fail");
            }

            self->clientCookieMap_.erase(service_name);
        }
        else
        {
            PLOGI("connect:%s\n", service_name);
        }
        return true;
    };

    if (!LSRegisterServerStatusEx(handle, unique_client_id, func, static_cast<void *>(this),
                                  &clientCookie->second, nullptr))
    {
        PLOGE("error LSRegisterServerStatusEx\n");
    }

    return true;
}

DEVICE_RETURN_CODE_T CameraService::validateClient(LSMessage *message, int ndevice_handle)
{
    auto client_id = LSMessageGetSender(message);

    // camera id validation check
    if (ndevice_handle == n_invalid_id)
        return DEVICE_ERROR_JSON_PARSING;

    return CommandManager::getInstance().checkDeviceClient(ndevice_handle, client_id);
}

//[Camera Solution Manager] NEW APIs for Solution Manager - start
bool CameraService::getSolutions(LSMessage &message)
{
    PLOGI(" E \n");
    auto *payload               = LSMessageGetPayload(&message);
    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;
    std::vector<std::string> supportedSolutionList;
    std::vector<std::string> enabledSolutionList;

    GetSolutionsMethod obj_getsolutions;
    obj_getsolutions.getObject(payload, getSolutionsSchema);
    int ndevhandle = obj_getsolutions.getDeviceHandle();
    int ncamId     = getId(obj_getsolutions.getCameraId());

    PLOGI("ndevhandle (%d) \n", ndevhandle);
    PLOGI("ncamId (%d) \n", ncamId);
    // check if device handle value and camera id are set property.
    //       device handle         cameraId        result
    //  1.        set                 set           error
    //  2.        set               not set         OK
    //  3.      not set               set           OK
    //  4.      not set             not set         error

    if (n_invalid_id != ndevhandle && n_invalid_id != ncamId)
    {
        err_id = DEVICE_ERROR_WRONG_PARAM;
    }
    else if (n_invalid_id == ndevhandle && n_invalid_id == ncamId)
    {
        err_id = DEVICE_ERROR_PARAM_IS_MISSING;
    }

    if (err_id == DEVICE_OK && ndevhandle != n_invalid_id)
        err_id = validateClient(&message, ndevhandle);

    if (err_id != DEVICE_OK)
    {
        PLOGI("err_id(%d)\n", err_id);
        obj_getsolutions.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                        getErrorString(err_id));
    }
    else
    {
        if (n_invalid_id == ndevhandle)
        {
            ndevhandle = CommandManager::getInstance().getCameraHandle(ncamId);
            PLOGI("devhandel by camera(%d) is (%d)\n", ncamId, ndevhandle);
        }
        PLOGI("DEVICE_OK\n");
        obj_getsolutions.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id,
                                        getErrorString(err_id));

        err_id = CommandManager::getInstance().getSupportedCameraSolutionInfo(
            ndevhandle, supportedSolutionList);
        if (DEVICE_OK != err_id)
        {
            PLOGI("error happens on getting supported solution list by err_id(%d)\n", err_id);
            obj_getsolutions.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                            getErrorString(err_id));
        }

        err_id = CommandManager::getInstance().getEnabledCameraSolutionInfo(ndevhandle,
                                                                            enabledSolutionList);
        if (DEVICE_OK != err_id)
        {
            PLOGI("error happens on getting enabled solution list by err_id(%d)\n", err_id);
            obj_getsolutions.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                            getErrorString(err_id));
        }
    }

    std::string output_reply =
        obj_getsolutions.createObjectJsonString(supportedSolutionList, enabledSolutionList);

    LS::Message request(&message);
    request.respond(output_reply.c_str());
    PLOGI(" output_reply(%s) \n", output_reply.c_str());

    PLOGI(" X \n");
    return true;
}

bool CameraService::setSolutions(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);

    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    SetSolutionsMethod obj_setSolutions;
    obj_setSolutions.getObject(payload, setSolutionsSchema);
    int ndevhandle = obj_setSolutions.getDeviceHandle();
    int ncamId     = getId(obj_setSolutions.getCameraId());
    PLOGI("ndevhandle (%d) \n", ndevhandle);
    PLOGI("ncamId (%d) \n", ncamId);

    // check if device handle value and camera id are set property.
    //       device handle         cameraId        result
    //  1.        set                 set           error
    //  2.        set               not set         OK
    //  3.      not set               set           OK
    //  4.      not set             not set         error

    if (n_invalid_id != ndevhandle && n_invalid_id != ncamId)
    {
        err_id = DEVICE_ERROR_WRONG_PARAM;
    }
    else if (n_invalid_id == ndevhandle && n_invalid_id == ncamId)
    {
        err_id = DEVICE_ERROR_PARAM_IS_MISSING;
    }

    if (err_id == DEVICE_OK && ndevhandle != n_invalid_id)
        err_id = validateClient(&message, ndevhandle);

    if (err_id != DEVICE_OK)
    {
        PLOGI("err_id(%d)\n", err_id);

        obj_setSolutions.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                        getErrorString(err_id));
    }
    else
    {
        // check solutions is empty or not
        if (obj_setSolutions.isEmpty())
        {
            PLOGI("solutions is empty\n");
            err_id = DEVICE_ERROR_WRONG_PARAM;
            obj_setSolutions.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                            getErrorString(err_id));
        }
        else
        {
            if (n_invalid_id == ndevhandle)
            {
                ndevhandle = CommandManager::getInstance().getCameraHandle(ncamId);
                PLOGI("devhandel by camera(%d) is (%d)\n", ncamId, ndevhandle);
            }

            // check if the requested solution parameter is valid or not. If not valid at least one
            // of them, this method didn't do anything
            std::vector<std::string> supportedSolutionList;
            unsigned int candidateSolutionCnt = 0;
            err_id = CommandManager::getInstance().getSupportedCameraSolutionInfo(
                ndevhandle, supportedSolutionList);

            std::vector<std::string> str_solutions = obj_setSolutions.getEnableSolutionList();

            for (auto &s : str_solutions)
            {
                for (auto &i : supportedSolutionList)
                {
                    if (s == i)
                    {
                        if (candidateSolutionCnt < UINT_MAX)
                        {
                            candidateSolutionCnt++;
                        }
                        PLOGI("candidate enabled solutionName %s", s.c_str());
                    }
                }
            }

            // check if the parameters from client are all valid by comparing candidateSolutionCnt
            // number and parameters number.
            if (str_solutions.size() != candidateSolutionCnt)
            {
                PLOGE("%zd invalid parameter existed\n",
                      str_solutions.size() - candidateSolutionCnt);
                err_id = DEVICE_ERROR_WRONG_PARAM;
                obj_setSolutions.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                                getErrorString(err_id));
            }

            str_solutions        = obj_setSolutions.getDisableSolutionList();
            candidateSolutionCnt = 0;
            for (auto &s : str_solutions)
            {
                for (auto &i : supportedSolutionList)
                {
                    if (s == i)
                    {
                        if (candidateSolutionCnt < UINT_MAX)
                        {
                            candidateSolutionCnt++;
                        }
                        PLOGI("candidate enabled solutionName %s", s.c_str());
                    }
                }
            }

            // check if the parameters from client are all valid by comparing candidateSolutionCnt
            // number and parameters number.
            if (str_solutions.size() != candidateSolutionCnt)
            {
                PLOGE("%zd invalid parameter existed\n",
                      str_solutions.size() - candidateSolutionCnt);
                err_id = DEVICE_ERROR_WRONG_PARAM;
                obj_setSolutions.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                                getErrorString(err_id));
            }

            if (DEVICE_OK == err_id)
            {
                err_id = CommandManager::getInstance().enableCameraSolution(
                    ndevhandle, obj_setSolutions.getEnableSolutionList());
                err_id = CommandManager::getInstance().disableCameraSolution(
                    ndevhandle, obj_setSolutions.getDisableSolutionList());
            }

            if (DEVICE_OK != err_id)
            {
                PLOGI("DEVICE_NOT_OK err_id(%d)\n", err_id);
                obj_setSolutions.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                                getErrorString(err_id));
            }
            else
            {
                PLOGI("DEVICE_OK\n");
                obj_setSolutions.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id,
                                                getErrorString(err_id));
            }
        }
    }

    std::string output_reply = obj_setSolutions.createObjectJsonString();
    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

//[Camera Solution Manager] NEW APIs for Solution Manager - end

bool CameraService::getFormat(LSMessage &message)
{
    auto *payload = LSMessageGetPayload(&message);

    DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

    GetFormatMethod obj_getFormat;
    obj_getFormat.getObject(payload, getFormatSchema);

    int ndevhandle = n_invalid_id;
    int ncamId     = getId(obj_getFormat.getCameraId());

    PLOGI("ncamId (%d) \n", ncamId);

    if (n_invalid_id == ncamId)
    {
        err_id = DEVICE_ERROR_WRONG_PARAM;
        PLOGI("err_id(%d)\n", err_id);
        obj_getFormat.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
        // add subscription
        std::string event_key = CONST_EVENT_KEY_FORMAT;
        event_key += "_";
        event_key += obj_getFormat.getCameraId();
        bool bsubscribed = event_obj.addSubscription(this->get(), std::move(event_key), message);
        PLOGI("bsubscribed (%d) \n", bsubscribed);
        obj_getFormat.setSubcribed(bsubscribed);

        ndevhandle = CommandManager::getInstance().getCameraHandle(ncamId);
        PLOGI("devhandel by camera(%d) is (%d)\n", ncamId, ndevhandle);
        if (n_invalid_id != ndevhandle)
        {
            CAMERA_FORMAT output_format;
            err_id = CommandManager::getInstance().getFormat(ndevhandle, &output_format);

            if (DEVICE_OK != err_id)
            {
                PLOGI("DEVICE_NOT_OK err_id(%d)\n", err_id);
                obj_getFormat.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                             getErrorString(err_id));
            }
            else
            {
                PLOGI("DEVICE_OK\n");
                obj_getFormat.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id,
                                             getErrorString(err_id));
                obj_getFormat.setCameraFormat(output_format);
            }
        }
        else
        {
            err_id = DEVICE_ERROR_DEVICE_IS_NOT_OPENED;
            obj_getFormat.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                         getErrorString(err_id));
        }
    }

    std::string output_reply = obj_getFormat.createObjectJsonString();
    LS::Message request(&message);
    request.respond(output_reply.c_str());

    return true;
}

int main(int argc, char *argv[])
{
    try
    {
        if (!install_handler_service_crash())
        {
            return 1;
        }

        CameraService camerasrv;
    }
    catch (LS::Error &err)
    {
        LSErrorPrint(err, stdout);
        return 1;
    }
    catch (std::bad_cast &err)
    {
        std::cerr << err.what() << std::endl;
        return 1;
    }
    catch (const std::logic_error &err)
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

extern "C" void signal_handler_service_crash(int sig)
{
    PLOGI("signal(%d) received !!\n", sig);

    CommandManager::getInstance().handleCrash();
    exit(1);
}

bool install_handler_service_crash() noexcept
{
    bool ret                        = true;
    sigact_service_crash.sa_handler = signal_handler_service_crash;
    sigfillset(&sigact_service_crash.sa_mask);
    sigact_service_crash.sa_flags = SA_RESTART;
    for (int i = SIGHUP; i <= SIGSYS; i++)
    {
        if (i == SIGKILL || i == SIGSTOP || i == SIGCHLD || i == SIGURG || i == SIGCONT)
        {
            continue;
        }
        if (sigaction(i, &sigact_service_crash, NULL) == -1)
        {
            PLOGE("install signal handler for signal %d :: FAIL\n", i);
            ret = false;
        }
    }
    return ret;
}
