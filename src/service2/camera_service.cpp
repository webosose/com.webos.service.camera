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
#include "camera_service.h"
#include "camera_hal_types.h"
#include "camera_types.h"
#include "command_manager.h"
#include "json_parser.h"
#include "json_schema.h"
#include "notifier.h"
#include "service_types.h"
#include <pbnjson.hpp>

#include <sstream>
#include <string>

const std::string service = "com.webos.service.camera3";

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

  // attach to mainloop and run it
  attachToLoop(main_loop_ptr_.get());

  // sunscribe to pdm client
  Notifier notifier;
  notifier.setLSHandle(this->get());
  notifier.addNotifier(NotifierClient::NOTIFIER_CLIENT_PDM);

  // run the gmainloop
  g_main_loop_run(main_loop_ptr_.get());
}

int CameraService::getId(std::string cameraid)
{
  std::stringstream strStream;
  int num = n_invalid_id;
  int len = cameraid.length();
  for (int index = 0; index < len; index++)
  {
    if (isdigit(cameraid[index]))
    {
      strStream << cameraid[index];
      strStream >> num;
    }
  }
  return num;
}

bool CameraService::open(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);

  // create Open class object and read data from json object after schema validation
  OpenMethod open;
  open.getOpenObject(payload, openSchema);

  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  // camera id validation check
  if (invalid_device_id == open.getCameraId())
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    open.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    int ndev_id = getId(open.getCameraId());
    PMLOG_INFO(CONST_MODULE_LUNA, "device Id %d\n", ndev_id);
    int ndevice_handle = n_invalid_id;

    // open camera device and save fd
    err_id = CommandManager::getInstance().open(ndev_id, &ndevice_handle);
    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      open.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      open.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
      open.setDeviceHandle(ndevice_handle);
    }
  }

  // create json string now for LS reply
  std::string output_reply = open.createOpenObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::close(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);

  // create close class object and read data from json object after schema validation
  StopPreviewCaptureCloseMethod obj_close;
  obj_close.getObject(payload, stopCapturePreviewCloseSchema);

  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  int ndev_id = obj_close.getDeviceHandle();
  // camera id validation check
  if (n_invalid_id == ndev_id)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_close.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::close ndev_id : %d\n", ndev_id);
    // close device here
    err_id = CommandManager::getInstance().close(ndev_id);

    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      obj_close.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      obj_close.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
    }
  }

  // create json string now for reply
  std::string output_reply = obj_close.createObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::startPreview(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  StartPreviewMethod obj_startpreview;
  obj_startpreview.getStartPreviewObject(payload, startPreviewSchema);

  int ndevice_id = obj_startpreview.getDeviceHandle();
  if (n_invalid_id == ndevice_id)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_startpreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::startPreview DevID : %d\n");
    // start preview here
    int key = 0;
    err_id = CommandManager::getInstance().startPreview(ndevice_id, &key);

    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      obj_startpreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      obj_startpreview.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
      obj_startpreview.setKeyValue(key);
    }
  }

  // create json string now for reply
  std::string output_reply = obj_startpreview.createStartPreviewObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::stopPreview(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  StopPreviewCaptureCloseMethod obj_stoppreview;
  obj_stoppreview.getObject(payload, stopCapturePreviewCloseSchema);

  int ndevice_id = obj_stoppreview.getDeviceHandle();

  if (n_invalid_id == ndevice_id)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_stoppreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::stopPreview ndevice_id : %d\n", ndevice_id);
    // stop preview here
    err_id = CommandManager::getInstance().stopPreview(ndevice_id);

    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      obj_stoppreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      obj_stoppreview.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
    }
  }

  // create json string now for reply
  std::string output_reply = obj_stoppreview.createObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::startCapture(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  StartCaptureMethod obj_startcapture;
  obj_startcapture.getStartCaptureObject(payload, startCaptureSchema);

  int ndevice_id = obj_startcapture.getDeviceHandle();

  if (n_invalid_id == ndevice_id)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_startcapture.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    if ((CAMERA_FORMAT_JPEG != obj_startcapture.rGetParams().eFormat) &&
        (CAMERA_FORMAT_YUV != obj_startcapture.rGetParams().eFormat))
    {
      err_id = DEVICE_ERROR_UNSUPPORTED_FORMAT;
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::startCapture ndevice_id : %d\n", ndevice_id);
      PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::startCapture nImage : %d\n",
                 obj_startcapture.getnImage());

      // capture image here
      if (0 != obj_startcapture.getnImage())
        err_id = CommandManager::getInstance().captureImage(
            ndevice_id, obj_startcapture.getnImage(), obj_startcapture.rGetParams());
      else
        err_id =
            CommandManager::getInstance().startCapture(ndevice_id, obj_startcapture.rGetParams());
    }
    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      obj_startcapture.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      obj_startcapture.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
    }
  }

  // create json string now for reply
  std::string output_reply = obj_startcapture.createStartCaptureObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::stopCapture(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  StopPreviewCaptureCloseMethod obj_stopcapture;
  obj_stopcapture.getObject(payload, stopCapturePreviewCloseSchema);

  int ndevice_id = obj_stopcapture.getDeviceHandle();

  if (n_invalid_id == ndevice_id)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_stopcapture.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::stopPreview ndevice_id : %d\n", ndevice_id);
    // stop capture here
    err_id = CommandManager::getInstance().stopCapture(ndevice_id);

    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      obj_stopcapture.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      obj_stopcapture.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
    }
  }

  // create json string now for reply
  std::string output_reply = obj_stopcapture.createObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::getInfo(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  GetInfoMethod obj_getinfo;
  obj_getinfo.getInfoObject(payload, getInfoSchema);

  if (invalid_device_id == obj_getinfo.strGetDeviceId())
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_getinfo.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    // get info here
    CAMERA_INFO_T o_camerainfo;

    int ndev_id = getId(obj_getinfo.strGetDeviceId());
    PMLOG_INFO(CONST_MODULE_LUNA, "device Id %d\n", ndev_id);

    err_id = CommandManager::getInstance().getDeviceInfo(ndev_id, &o_camerainfo);

    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      obj_getinfo.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      obj_getinfo.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
      obj_getinfo.setCameraInfo(o_camerainfo);
    }
  }

  // create json string now for reply
  std::string output_reply = obj_getinfo.createInfoObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::getCameraList(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  // create CameraList class object and read data from json object after schema validation
  GetCameraListMethod obj_getcameralist;
  bool ret = obj_getcameralist.getCameraListObject(payload, getCameraListSchema);

  if (false == ret)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_getcameralist.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    // get camera list here
    int arr_camsupport[CONST_MAX_DEVICE_COUNT], arr_micsupport[CONST_MAX_DEVICE_COUNT];
    int arr_camdev[CONST_MAX_DEVICE_COUNT], arr_micdev[CONST_MAX_DEVICE_COUNT];

    for (int i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
    {
      arr_camdev[i] = arr_micdev[i] = CONST_VARIABLE_INITIALIZE;
      arr_camsupport[i] = arr_micsupport[i] = 0;
    }

    err_id = CommandManager::getInstance().getDeviceList(arr_camdev, arr_micdev, arr_camsupport,
                                                         arr_micsupport);

    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      obj_getcameralist.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                       getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      obj_getcameralist.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));

      char arrlist[20][CONST_MAX_STRING_LENGTH];
      int supportlist[20];
      int n_camcount = 0;

      for (int i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
      {
        if (CONST_VARIABLE_INITIALIZE == arr_camdev[i])
          break;
        snprintf(arrlist[n_camcount], CONST_MAX_STRING_LENGTH, "%s%d", CONST_DEVICE_NAME_CAMERA,
                 arr_camdev[i]);
        supportlist[n_camcount] = arr_camsupport[i];
        n_camcount++;
      }

      obj_getcameralist.setCameraCount(n_camcount);
      for (int i = 0; i < n_camcount; i++)
      {
        obj_getcameralist.setCameraList(arrlist[i], i);
      }
    }
  }

  // create json string now for reply
  std::string output_reply = obj_getcameralist.createCameraListObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::getProperties(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  GetSetPropertiesMethod obj_getproperties;
  obj_getproperties.getPropertiesObject(payload, getPropertiesSchema);

  int devid = obj_getproperties.getDeviceHandle();

  if (n_invalid_id == devid)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_getproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    // get properties here
    PMLOG_INFO(CONST_MODULE_LUNA, "devid %d\n", devid);
    CAMERA_PROPERTIES_T dev_property;
    err_id = CommandManager::getInstance().getProperty(devid, &dev_property);

    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      obj_getproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id,
                                       getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      obj_getproperties.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
      obj_getproperties.setCameraProperties(dev_property);
    }
  }

  // create json string now for reply
  std::string output_reply = obj_getproperties.createGetPropertiesObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::setProperties(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  GetSetPropertiesMethod objsetproperties;
  objsetproperties.getSetPropertiesObject(payload, setPropertiesSchema);

  int devid = objsetproperties.getDeviceHandle();

  if (n_invalid_id == devid)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    objsetproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    // set properties here
    CAMERA_PROPERTIES_T oParams = objsetproperties.rGetCameraProperties();
    PMLOG_INFO(CONST_MODULE_LUNA, "DevID %d\n", devid);
    err_id = CommandManager::getInstance().setProperty(devid, &oParams);
    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      objsetproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      objsetproperties.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
    }
  }

  // create json string now for reply
  std::string output_reply = objsetproperties.createSetPropertiesObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::setFormat(LSMessage &message)
{
  const char *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE err_id = DEVICE_OK;

  SetFormatMethod objsetformat;
  objsetformat.getSetFormatObject(payload, setFormatSchema);

  int devid = objsetformat.getDeviceHandle();
  // camera id validation check
  if (n_invalid_id == devid)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    objsetformat.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    // setformat here
    PMLOG_INFO(CONST_MODULE_LUNA, "setFormat DevID %d\n", devid);
    CAMERA_FORMAT sformat = objsetformat.rGetCameraFormat();
    err_id = CommandManager::getInstance().setFormat(devid, sformat);
    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      objsetformat.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      objsetformat.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
    }
  }

  // create json string now for reply
  std::string output_reply = objsetformat.createSetFormatObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

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

  const std::string responsesuccess = "{\"returnValue\": true}";
  const std::string responsefailure = "{\"returnValue\": false}";
  const std::string responsesubscribed = "{\"returnValue\": true,\"subscribed\": true}";

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
