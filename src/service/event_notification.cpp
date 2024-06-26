// Copyright (c) 2019-2023 LG Electronics, Inc.
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

#include "constants.h"
#include "camera_types.h"
#include "command_manager.h"
#include "event_notification.h"
#include "json_parser.h"

#define CONST_MODULE_EVENTMANGER "EventManager"

bool EventNotification::addSubscription(LSHandle *lsHandle, std::string key, LSMessage &message)
{
    LSError error;
    LSErrorInit(&error);

    if (LSMessageIsSubscription(&message))
    {
        PMLOG_INFO(CONST_MODULE_EVENTMANGER,"LSMessageIsSubscription success");
        if (!LSSubscriptionAdd(lsHandle, key.c_str(), &message, &error))
        {
            PMLOG_ERROR(CONST_MODULE_EVENTMANGER,"LSSubscriptionAdd failed");
            LSErrorPrint(&error, stderr);
            LSErrorFree(&error);
            return false;
        }
        (void)getSubscribeCount(lsHandle, std::move(key));
        LSErrorFree(&error);
        return true;
    }
    else
    {
        (void)getSubscribeCount(lsHandle, std::move(key));
    }

    LSErrorFree(&error);
    return false;
}

void EventNotification::subscriptionReply(LSHandle *lsHandle, std::string key,
                                          std::string output_reply)
{
    LSError error;
    LSErrorInit(&error);
    if (!LSSubscriptionReply(lsHandle, key.c_str(), output_reply.c_str(), &error))
    {
        PMLOG_ERROR(CONST_MODULE_EVENTMANGER,"LSSubscriptionReply failed");
        LSErrorPrint(&error, stderr);
    }

    LSErrorFree(&error);
    PMLOG_DEBUG("end");
}

int EventNotification::getSubscribeCount(LSHandle *lsHandle, std::string key)
{
    int ret = LSSubscriptionGetHandleSubscribersCount(lsHandle, key.c_str());
    PMLOG_INFO(CONST_MODULE_EVENTMANGER,"cnt:%d, key:%s", ret, key.c_str());
    return ret;
}

bool EventNotification::getJsonString(json &json_outobj, std::string key, EventType etype,
                                      void *p_cur_data, void *p_old_data)
{
    bool result_val = true;

    json_outobj[CONST_PARAM_NAME_SUBSCRIBED] = true;

    switch (etype)
    {
    case EventType::EVENT_TYPE_FORMAT:
    {
        // camera id
        json_outobj[CONST_PARAM_NAME_ID] = getCameraIdFromKey(key);

        if (p_cur_data != nullptr && p_old_data != nullptr)
        {
            json json_outobjparams;
            CAMERA_FORMAT *cur_format = static_cast<CAMERA_FORMAT *>(p_cur_data);
            CAMERA_FORMAT *old_format = static_cast<CAMERA_FORMAT *>(p_old_data);

            if (old_format != cur_format)
            {
                json_outobjparams[CONST_PARAM_NAME_WIDTH]  = cur_format->nWidth;
                json_outobjparams[CONST_PARAM_NAME_HEIGHT] = cur_format->nHeight;
                json_outobjparams[CONST_PARAM_NAME_FPS]    = cur_format->nFps;
                json_outobjparams[CONST_PARAM_NAME_FORMAT] =
                    getFormatStringFromCode(cur_format->eFormat);
                json_outobj[CONST_PARAM_NAME_PARAMS] = std::move(json_outobjparams);
            }
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_EVENTMANGER,"event: %d pdata is null", (int)etype);
            result_val = false;
        }
        break;
    }

    case EventType::EVENT_TYPE_PROPERTIES:
    {
        // camera id
        json_outobj[CONST_PARAM_NAME_ID] = getCameraIdFromKey(std::move(key));

        if (p_cur_data != nullptr && p_old_data != nullptr)
        {
            json json_outobjparams;
            CAMERA_PROPERTIES_T *cur_properties = static_cast<CAMERA_PROPERTIES_T *>(p_cur_data);
            CAMERA_PROPERTIES_T *old_properties = static_cast<CAMERA_PROPERTIES_T *>(p_old_data);

            if (cur_properties != old_properties)
            {
                auto json_propertyobj = json::object();
                for (int i = 0; i < PROPERTY_END; i++)
                {
                    if (cur_properties->stGetData.data[i][QUERY_VALUE] != CONST_PARAM_DEFAULT_VALUE)
                    {
                        json_propertyobj[CONST_PARAM_NAME_MIN] =
                            cur_properties->stGetData.data[i][QUERY_MIN];
                        json_propertyobj[CONST_PARAM_NAME_MAX] =
                            cur_properties->stGetData.data[i][QUERY_MAX];
                        json_propertyobj[CONST_PARAM_NAME_STEP] =
                            cur_properties->stGetData.data[i][QUERY_STEP];
                        json_propertyobj[CONST_PARAM_NAME_DEFAULT_VALUE] =
                            cur_properties->stGetData.data[i][QUERY_DEFAULT];
                        json_propertyobj[CONST_PARAM_NAME_VALUE] =
                            cur_properties->stGetData.data[i][QUERY_VALUE];
                        json_outobjparams[getParamString(i)] = json_propertyobj;
                    }
                }
                json_outobj[CONST_PARAM_NAME_PARAMS] = std::move(json_outobjparams);
            }
        }
        else
        {
            PMLOG_ERROR(CONST_MODULE_EVENTMANGER,"event: %d pdata is null", (int)etype);
            result_val = false;
        }

        break;
    }

    case EventType::EVENT_TYPE_CONNECT:
    case EventType::EVENT_TYPE_DISCONNECT:
    {
        std::vector<int> idList;
        if (DEVICE_OK != CommandManager::getInstance().getDeviceList(idList))
        {
            PMLOG_ERROR(CONST_MODULE_EVENTMANGER,"getDeviceList returns not OK");
            result_val = false;
            break;
        }

        auto deviceListArr = json::array();
        auto deviceListObj = json::object();

        for (const auto &it : idList)
        {
            deviceListObj[CONST_PARAM_NAME_ID] = CONST_DEVICE_NAME_CAMERA + std::to_string(it);
            deviceListArr.push_back(deviceListObj);
        }
        json_outobj[CONST_PARAM_NAME_DEVICE_LIST] = std::move(deviceListArr);

        break;
    }

    default:
    {
        result_val = false;
        break;
    }
    }

    return result_val;
}

void EventNotification::eventReply(LSHandle *lsHandle, std::string key, EventType etype,
                                   void *p_cur_data, void *p_old_data)
{
    if (getSubscribeCount(lsHandle, key) > 0)
    {
        json json_outobj;
        std::string str_reply;

        json_outobj[CONST_PARAM_NAME_RETURNVALUE] =
            getJsonString(json_outobj, key, etype, p_cur_data, p_old_data);
        str_reply = json_outobj.dump();

        subscriptionReply(lsHandle, std::move(key), str_reply);
        PMLOG_INFO(CONST_MODULE_EVENTMANGER,"str_reply %s", str_reply.c_str());
    }
}

std::string EventNotification::getEventKeyWithId(int dev_handle, std::string key)
{
    std::string str_reply;
    int deviceid         = CommandManager::getInstance().getCameraId(dev_handle);
    std::string cameraid = "_camera";
    cameraid += std::to_string(deviceid);
    str_reply = key + cameraid;
    return str_reply;
}

std::string EventNotification::getCameraIdFromKey(std::string key)
{
    std::string str_reply;
    int split_pos = key.find("_");
    str_reply     = key.substr(split_pos + 1);
    return str_reply;
}

void EventNotification::removeSubscription(LSHandle *lsHandle, int camera_id)
{
    LSError error;
    LSErrorInit(&error);

    std::string output_reply;
    json json_outobj;
    LSSubscriptionIter *LSiter = NULL;

    std::string key_camera     = "_camera";
    std::string key_format     = CONST_EVENT_KEY_PROPERTIES;
    std::string key_properties = CONST_EVENT_KEY_FORMAT;

    key_camera += std::to_string(camera_id);
    key_format += key_camera;
    key_properties += key_camera;

    std::vector<std::string> key_list{std::move(key_format), std::move(key_properties)};

    json_outobj[CONST_PARAM_NAME_RETURNVALUE] = false;
    json_outobj[CONST_PARAM_NAME_SUBSCRIBED]  = false;
    json_outobj[CONST_PARAM_NAME_ID]          = getCameraIdFromKey(std::move(key_camera));
    json_outobj[CONST_PARAM_NAME_ERROR_CODE]  = DEVICE_ERROR_SUBSCIRPTION_FAIL_DEVICE_DISCONNETED;
    json_outobj[CONST_PARAM_NAME_ERROR_TEXT] =
        getErrorString(DEVICE_ERROR_SUBSCIRPTION_FAIL_DEVICE_DISCONNETED);

    output_reply = json_outobj.dump();

    for (const auto &it : key_list)
    {
        if (getSubscribeCount(lsHandle, it) > 0)
        {
            if (!LSSubscriptionReply(lsHandle, it.c_str(), output_reply.c_str(), &error))
            {
                PMLOG_ERROR(CONST_MODULE_EVENTMANGER,"LSSubscriptionReply failed");
                LSErrorPrint(&error, stderr);
            }

            if (LSSubscriptionAcquire(lsHandle, it.c_str(), &LSiter, &error))
            {
                // remove all subscription
                LSMessage *subscriber_message;
                while (LSSubscriptionHasNext(LSiter))
                {
                    subscriber_message = LSSubscriptionNext(LSiter);
                    if (LSMessageIsSubscription(subscriber_message))
                    {
                        LSSubscriptionRemove(LSiter);
                    }
                }
                LSSubscriptionRelease(LSiter);
            }
            else
            {
                PMLOG_ERROR(CONST_MODULE_EVENTMANGER,"LSSubscriptionAcquire failed");
                LSErrorPrint(&error, stderr);
            }
            // check the subscription count 0
            if (getSubscribeCount(lsHandle, it) > 0)
            {
                PMLOG_ERROR(CONST_MODULE_EVENTMANGER,"Remove failed!! need check");
            }
        }
    }

    LSErrorFree(&error);
}





