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

#include "camera_types.h"
#include "json_utils.h"
#include "json_parser.h"
#include "command_manager.h"
#include "event_notification.h"

#define CONST_MODULE_EVENTMANGER "EventManager"


bool EventNotification::addSubscription(LSHandle * lsHandle, const char* key, LSMessage &message)
{
  LSError error;
  LSErrorInit(&error);
  int cnt = -1;

  if (LSMessageIsSubscription(&message))
  {
    PMLOG_INFO(CONST_MODULE_EVENTMANGER, "addSubscription LSMessageIsSubscription success\n");
    if (!LSSubscriptionAdd(lsHandle, key, &message, &error))
    {
      PMLOG_INFO(CONST_MODULE_EVENTMANGER, "addSubscription LSSubscriptionAdd failed\n");
      LSErrorPrint(&error, stderr);
      LSErrorFree(&error);
      return false;
    }
    cnt = getSubscripeCount(lsHandle, key);
    PMLOG_INFO(CONST_MODULE_EVENTMANGER, "addSubscription_cnt %d\n", cnt);
    LSErrorFree(&error);
    return true;
  }

  LSErrorFree(&error);
  return false;
}

void EventNotification::subscriptionReply(LSHandle *lsHandle, const char* key, jvalue_ref output_reply)
{
  LSError error;
  LSErrorInit(&error);
  if (!LSSubscriptionReply(lsHandle, key, jvalue_tostring_simple(output_reply), &error))
  {
    PMLOG_INFO(CONST_MODULE_EVENTMANGER, "subscriptionReply failed\n");
    LSErrorPrint(&error, stderr);
  }

  LSErrorFree(&error);
  PMLOG_INFO(CONST_MODULE_EVENTMANGER, "subscriptionReply end\n");
}

int EventNotification::getSubscripeCount(LSHandle * lsHandle, const char* key)
{
  int ret = -1;
  ret = LSSubscriptionGetHandleSubscribersCount(lsHandle, key);
  return ret;
}

bool EventNotification::getJsonString(jvalue_ref &json_outobj, void *p_cur_data, void *p_old_data, EventType etype)
{
  // event type
  std::string event = getEventNotificationString(etype);
  bool resultVal = true;

  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EVENT),
                  jstring_create(event.c_str()));

  switch (etype)
  {
    case EventType::EVENT_TYPE_FORMAT:
    {
      if (p_cur_data != nullptr && p_old_data != nullptr)
      {
        jvalue_ref json_outobjparams = jobject_create();
        CAMERA_FORMAT *cur_format = static_cast<CAMERA_FORMAT *>(p_cur_data);
        CAMERA_FORMAT *old_format = static_cast<CAMERA_FORMAT *>(p_old_data);

        if (old_format->nWidth != cur_format->nWidth)
          jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WIDTH),
                    jnumber_create_i32(cur_format->nWidth));
        if (old_format->nHeight != cur_format->nHeight)
          jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HEIGHT),
                    jnumber_create_i32(cur_format->nHeight));
        if (old_format->nFps != cur_format->nFps)
          jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FPS),
                    jnumber_create_i32(cur_format->nFps));

        if (old_format->eFormat != cur_format->eFormat)
        {
          std::string strformat = getFormatStringFromCode(cur_format->eFormat);
          jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT),
                    jstring_create(strformat.c_str()));
        }
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMATINFO), json_outobjparams);
      }
      else
      {
        PMLOG_INFO(CONST_MODULE_EVENTMANGER, "getJsonString event: %d pdata is null \n", etype);
        resultVal = false;
      }
      break;
    }

    case EventType::EVENT_TYPE_PROPERTIES:
    {
      // camera id
      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ID),
                  jstring_create(strcamid_.c_str()));
      if (p_cur_data != nullptr && p_old_data != nullptr)
      {
        jvalue_ref json_outobjparams = jobject_create();
        CAMERA_PROPERTIES_T *cur_properties = static_cast<CAMERA_PROPERTIES_T *>(p_cur_data);
        CAMERA_PROPERTIES_T *old_properties = static_cast<CAMERA_PROPERTIES_T *>(p_old_data);

        createGetPropertiesJsonString(cur_properties, old_properties, json_outobjparams);
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PROPERTIESINFO), json_outobjparams);
      }
      else
      {
        PMLOG_INFO(CONST_MODULE_EVENTMANGER, "getJsonString event: %d pdata is null \n", etype);
        resultVal = false;
      }
      break;
    }

    case EventType::EVENT_TYPE_CONNECT:
    case EventType::EVENT_TYPE_DISCONNECT:
    {
      int arr_camsupport[CONST_MAX_DEVICE_COUNT], arr_micsupport[CONST_MAX_DEVICE_COUNT];
      int arr_camdev[CONST_MAX_DEVICE_COUNT], arr_micdev[CONST_MAX_DEVICE_COUNT];

      for (int i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
      {
        arr_camdev[i] = arr_micdev[i] = CONST_PARAM_DEFAULT_VALUE;
        arr_camsupport[i] = arr_micsupport[i] = 0;
      }

      if (DEVICE_OK != CommandManager::getInstance().getDeviceList(arr_camdev, arr_micdev, arr_camsupport, arr_micsupport))
      {
          PMLOG_INFO(CONST_MODULE_EVENTMANGER, "getDeviceList returns not OK \n");
          resultVal = false;
          break;
      }

      jvalue_ref deviceListArr = jarray_create(NULL);

      for (int i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
      {
          if (arr_camdev[i] == CONST_PARAM_DEFAULT_VALUE)
              break;

          jvalue_ref deviceListObj = jobject_create();
          char buf[CONST_MAX_STRING_LENGTH] = {0, };
          snprintf(buf, CONST_MAX_STRING_LENGTH, "%s%d", CONST_DEVICE_NAME_CAMERA, arr_camdev[i]);
          jobject_put(deviceListObj, J_CSTR_TO_JVAL("id"), jstring_create(buf));
          jarray_append(deviceListArr, deviceListObj);
      }

      jobject_put(json_outobj, J_CSTR_TO_JVAL("deviceList"), deviceListArr);

      break;
    }

    default:
    {
      resultVal = false;
      break;
    }
  }

  return resultVal;
}

void EventNotification::eventReply(LSHandle * lsHandle, const char* key, void *p_cur_data, void *p_old_data, EventType etype)
{
  jvalue_ref json_outobj = jobject_create();
  std::string strreply;

  PMLOG_INFO(CONST_MODULE_EVENTMANGER, "eventReply_subsciptionCnt: %d\n", getSubscripeCount(lsHandle, key));

  bool rerunVal = getJsonString(json_outobj, p_cur_data, p_old_data, etype);

  // return value
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
              jboolean_create(rerunVal));

  subscriptionReply(lsHandle, key, json_outobj);

  const char* strvalue = jvalue_stringify(json_outobj);
  strreply = (strvalue) ? strvalue : "";
  PMLOG_INFO(CONST_MODULE_EVENTMANGER, "eventReply strreply %s\n", strreply.c_str());
  j_release(&json_outobj);

}

std::string
EventNotification::subscriptionJsonString(bool issubscribed)
{
  jvalue_ref json_outobj = jobject_create();
  std::string strreply;

  // return value
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
              jboolean_create(issubscribed));

  const char* strvalue = jvalue_stringify(json_outobj);
  strreply = (strvalue) ? strvalue : "";
  PMLOG_INFO(CONST_MODULE_EVENTMANGER, "subscriptionJsonString strreply %s\n",
             strreply.c_str());
  j_release(&json_outobj);

  return strreply;
}





