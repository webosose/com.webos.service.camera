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
 #include
 (File Inclusions)
 -- ----------------------------------------------------------------------------*/
#include "camera_service.h"
#include "camera_hal_types.h"
#include "camera_types.h"
#include "command_manager.h"
#include "json_schema.h"
#include "notifier.h"
#include <pbnjson.hpp>
#include <sstream>
#include <string>

#include <signal.h>


struct sigaction sigact_service_crash;
void signal_handler_service_crash(int sig);
void install_handler_service_crash();


const std::string service = "com.webos.service.camera2";
const std::string camerastr  = "camera";

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
  LS_CATEGORY_METHOD(getEventNotification)
  LS_CATEGORY_METHOD(getFd)
  LS_CATEGORY_END;

  // attach to mainloop and run it
  attachToLoop(main_loop_ptr_.get());

  // subscribe to pdm client
  Notifier notifier;
  notifier.setLSHandle(this->get());
  notifier.addNotifier(NotifierClient::NOTIFIER_CLIENT_PDM, main_loop_ptr_.get());

  // run the gmainloop
  g_main_loop_run(main_loop_ptr_.get());
}

int CameraService::getId(std::string cameraid)
{
  std::stringstream strStream;
  int num = n_invalid_id;
  int len = 0;
  int ndeviceid = 0;

  if ((cameraid.compare(0, camerastr.length(), camerastr)) == 0)
  {
    cameraid.erase (0, camerastr.length());
  }
  else
  {
    return num;
  }

  len = cameraid.length();

  for (int index = 0; index < len; index++)
  {
    if (isdigit(cameraid[index]))
    {
      strStream << cameraid[index];
      strStream >> num;
      strStream.clear();
      ndeviceid = num + ndeviceid*10;
      num = ndeviceid;
    }
    else
    {
      return n_invalid_id;
    }
  }
  return num;
}

void CameraService::createEventMessage(EventType etype, void *pdata, int devhandle)
{
  objevent_.setEventType(etype);

  int deviceid = CommandManager::getInstance().getCameraId(devhandle);
  std::string cameraid = "camera" + std::to_string(deviceid);
  objevent_.setCameraId(cameraid);

  std::string output_reply = cstr_empty;

  if(EventType::EVENT_TYPE_FORMAT == etype)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "createEventMessage EVENT_TYPE_FORMAT Event received\n");
    CAMERA_FORMAT newformat;
    CommandManager::getInstance().getFormat(devhandle, &newformat);

    auto *p_old_format = static_cast<CAMERA_FORMAT *>(pdata);

    if(*p_old_format != newformat)
    {
      auto *p_outdata = static_cast<void *>(&newformat);
      objevent_.setEventData(p_outdata);
      output_reply = objevent_.createEventObjectJsonString(pdata);
    }
  }
  else if(EventType::EVENT_TYPE_PROPERTIES == etype)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "createEventMessage EVENT_TYPE_PROPERTIES Event received\n");
    CAMERA_PROPERTIES_T new_property;
    CommandManager::getInstance().getProperty(devhandle, &new_property);

    auto *p_old_property = static_cast<CAMERA_PROPERTIES_T *>(pdata);

    if(*p_old_property != new_property)
    {
      auto *p_outdata = static_cast<void *>(&new_property);
      objevent_.setEventData(p_outdata);
      output_reply = objevent_.createEventObjectJsonString(pdata);
    }
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "createEventMessage Unknown Event received\n");
  }

  if(cstr_empty != output_reply)
  {
    LSError error;
    LSErrorInit(&error);
    if (!LSSubscriptionReply(this->get(), CONST_EVENT_NOTIFICATION, output_reply.c_str(), &error))
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "createEventMessage LSSubscriptionReply failed\n");
      LSErrorPrint(&error, stderr);
    }
    LSErrorFree(&error);
  }
}

bool CameraService::open(LSMessage &message)
{
  auto *payload = LSMessageGetPayload(&message);

  // create Open class object and read data from json object after schema validation
  OpenMethod open;
  open.getOpenObject(payload, openSchema);

  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  // camera id validation check
  if (cstr_invaliddeviceid == open.getCameraId())
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    open.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    int ndev_id = getId(open.getCameraId());
    PMLOG_INFO(CONST_MODULE_LUNA, "device Id %d\n", ndev_id);
    std::string app_priority = open.getAppPriority();
    PMLOG_INFO(CONST_MODULE_LUNA, "priority : %s \n", app_priority.c_str());
    int ndevice_handle = n_invalid_id;

    // open camera device and save fd
    err_id = CommandManager::getInstance().open(ndev_id, &ndevice_handle, app_priority);
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

      // add the client process Id to client pid pool if the pid is valid
      int n_client_pid = open.getClientProcessId();
      if (n_client_pid > 0)
      {
        int n_client_sig = open.getClientSignal();
        if (n_client_sig != -1)
        {
          PMLOG_INFO(CONST_MODULE_LUNA, "Try register client process Id %d with signal number %d ...", n_client_pid, n_client_sig);
          std::string outmsg;
          bool bRetVal = CommandManager::getInstance().registerClientPid(ndevice_handle, n_client_pid, n_client_sig, outmsg);
          if (bRetVal == true)
          {
            PMLOG_INFO(CONST_MODULE_LUNA, outmsg.c_str());
            open.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id) + "\n<pid, sig> ::" + outmsg);
          }
          else // opened camera itself is valid! 
          {
            PMLOG_INFO(CONST_MODULE_LUNA, outmsg.c_str());
            open.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id) + "\n<pid, sig> :: " + outmsg);           
          }
        }
      }
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
  auto *payload = LSMessageGetPayload(&message);

  // create close class object and read data from json object after schema validation
  StopPreviewCaptureCloseMethod obj_close;
  obj_close.getObject(payload, stopCapturePreviewCloseSchema);

  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  int ndevhandle = obj_close.getDeviceHandle();
  // camera id validation check
  if (n_invalid_id == ndevhandle)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_close.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    int n_client_pid = obj_close.getClientProcessId();
    if (n_client_pid > 0)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "Try to unregister the client of pid %d\n", n_client_pid);
      
      // first try remove the client pid from the client pid pool if the pid is valid  
      std::string pid_msg;
      bool bRetVal;
      bRetVal = CommandManager::getInstance().unregisterClientPid(ndevhandle, n_client_pid, pid_msg);
      if (bRetVal == false)
      {
        PMLOG_INFO(CONST_MODULE_LUNA, pid_msg.c_str());
        obj_close.setMethodReply(CONST_PARAM_VALUE_FALSE, 0, pid_msg);
      } 
      else // if successul then close device here
      {
        PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::close ndevhandle : %d\n", ndevhandle);

        err_id = CommandManager::getInstance().close(ndevhandle);
        if (DEVICE_OK != err_id)
        {
          PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
          obj_close.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id) + "\npid :: " + pid_msg);           
        }
        else
        {
          PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
          obj_close.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id) + "\npid :: " + pid_msg);          
        }
      }
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::close ndevhandle : %d\n", ndevhandle);

      // close device here
      err_id = CommandManager::getInstance().close(ndevhandle);

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
  auto *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;
  camera_memory_source_t memType;

  StartPreviewMethod obj_startpreview;
  obj_startpreview.getStartPreviewObject(payload, startPreviewSchema);

  int ndevhandle = obj_startpreview.getDeviceHandle();
  if (n_invalid_id == ndevhandle)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_startpreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::startPreview ndevhandle : %d\n", ndevhandle);
    // start preview here
    int key = 0;

    memType = obj_startpreview.rGetParams();
    if (memType.str_memorytype == kMemtypeShmem)
    {
        err_id = CommandManager::getInstance().startPreview(ndevhandle, memType.str_memorytype, &key);

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
    else
    {
        PMLOG_INFO(CONST_MODULE_LUNA, "memory type is not supported\n");
        err_id = DEVICE_ERROR_UNSUPPORTED_MEMORYTYPE;
        obj_startpreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
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
  auto *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  StopPreviewCaptureCloseMethod obj_stoppreview;
  obj_stoppreview.getObject(payload, stopCapturePreviewCloseSchema);

  int ndevhandle = obj_stoppreview.getDeviceHandle();

  if (n_invalid_id == ndevhandle)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_stoppreview.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::stopPreview ndevhandle : %d\n", ndevhandle);
    // stop preview here
    err_id = CommandManager::getInstance().stopPreview(ndevhandle);

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
  auto *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  StartCaptureMethod obj_startcapture;
  obj_startcapture.getStartCaptureObject(payload, startCaptureSchema);

  int ndevhandle = obj_startcapture.getDeviceHandle();

  if (n_invalid_id == ndevhandle)
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
      PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::startCapture ndevhandle : %d\n", ndevhandle);
      PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::startCapture nImage : %d\n",
                 obj_startcapture.getnImage());
      PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::startCapture path: %s\n",
                 obj_startcapture.getImagePath().c_str());

      // capture image here
      if (0 != obj_startcapture.getnImage())
        err_id = CommandManager::getInstance().captureImage(
            ndevhandle, obj_startcapture.getnImage(), obj_startcapture.rGetParams(),
            obj_startcapture.getImagePath());
      else
        err_id = CommandManager::getInstance().startCapture(
            ndevhandle, obj_startcapture.rGetParams(), obj_startcapture.getImagePath());
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
  auto *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  StopPreviewCaptureCloseMethod obj_stopcapture;
  obj_stopcapture.getObject(payload, stopCapturePreviewCloseSchema);

  int ndevhandle = obj_stopcapture.getDeviceHandle();

  if (n_invalid_id == ndevhandle)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_stopcapture.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::stopPreview ndevhandle : %d\n", ndevhandle);
    // stop capture here
    err_id = CommandManager::getInstance().stopCapture(ndevhandle);

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
  auto *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  GetInfoMethod obj_getinfo;
  obj_getinfo.getInfoObject(payload, getInfoSchema);

  if (cstr_invaliddeviceid == obj_getinfo.strGetDeviceId())
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_getinfo.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    // get info here
    camera_device_info_t o_camerainfo;

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
  auto *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

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
      arr_camdev[i] = arr_micdev[i] = CONST_PARAM_DEFAULT_VALUE;
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
      int n_camcount = 0;

      for (int i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
      {
        if (arr_camdev[i] == CONST_PARAM_DEFAULT_VALUE)
          break;
        snprintf(arrlist[n_camcount], CONST_MAX_STRING_LENGTH, "%s%d", CONST_DEVICE_NAME_CAMERA,
                 arr_camdev[i]);
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
  auto *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  GetSetPropertiesMethod obj_getproperties;
  obj_getproperties.getPropertiesObject(payload, getPropertiesSchema);

  int ndevhandle = obj_getproperties.getDeviceHandle();

  if (n_invalid_id == ndevhandle)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_getproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    // get properties here
    PMLOG_INFO(CONST_MODULE_LUNA, "ndevhandle %d\n", ndevhandle);
    CAMERA_PROPERTIES_T dev_property;
    err_id = CommandManager::getInstance().getProperty(ndevhandle, &dev_property);

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
  auto *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  GetSetPropertiesMethod objsetproperties;
  objsetproperties.getSetPropertiesObject(payload, setPropertiesSchema);

  int ndevhandle = objsetproperties.getDeviceHandle();

  if (n_invalid_id == ndevhandle)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    objsetproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    // check params object is empty or not
    if (objsetproperties.isParamsEmpty(payload, setPropertiesSchema))
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "Params object is empty\n");
      err_id = DEVICE_ERROR_WRONG_PARAM;
      objsetproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      // get old properties before setting new
      CAMERA_PROPERTIES_T old_property;
      CommandManager::getInstance().getProperty(ndevhandle, &old_property);
      auto *p_olddata = static_cast<void *>(&old_property);
      // set properties here
      CAMERA_PROPERTIES_T oParams = objsetproperties.rGetCameraProperties();
      PMLOG_INFO(CONST_MODULE_LUNA, "ndevhandle %d\n", ndevhandle);
      err_id = CommandManager::getInstance().setProperty(ndevhandle, &oParams);
      if (DEVICE_OK != err_id)
      {
        PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
        objsetproperties.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
      }
      else
      {
        PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
        objsetproperties.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
        // check if new properties are different from saved properties
        createEventMessage(EventType::EVENT_TYPE_PROPERTIES, p_olddata, ndevhandle);
      }
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
  auto *payload = LSMessageGetPayload(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  SetFormatMethod objsetformat;
  objsetformat.getSetFormatObject(payload, setFormatSchema);

  int ndevhandle = objsetformat.getDeviceHandle();
  // camera id validation check
  if (n_invalid_id == ndevhandle)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    objsetformat.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    // get saved format of the device
    CAMERA_FORMAT savedformat;
    CommandManager::getInstance().getFormat(ndevhandle, &savedformat);
    auto *p_olddata = static_cast<void *>(&savedformat);
    // setformat here
    PMLOG_INFO(CONST_MODULE_LUNA, "setFormat ndevhandle %d\n", ndevhandle);
    CAMERA_FORMAT sformat = objsetformat.rGetCameraFormat();
    err_id = CommandManager::getInstance().setFormat(ndevhandle, sformat);
    if (DEVICE_OK != err_id)
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id != DEVICE_OK\n");
      objsetformat.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "err_id == DEVICE_OK\n");
      objsetformat.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));
      // check if new format settings are different from saved format settings
      // get saved format of the device
      createEventMessage(EventType::EVENT_TYPE_FORMAT, p_olddata, ndevhandle);
    }
  }

  // create json string now for reply
  std::string output_reply = objsetformat.createSetFormatObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  return true;
}

bool CameraService::getEventNotification(LSMessage &message)
{
  auto *payload = LSMessageGetPayload(&message);
  LSError error;
  LSErrorInit(&error);

  objevent_.getEventObject(payload, getEventNotificationSchema);
  int ndev_id = getId(objevent_.getCameraId());

  if (LSMessageIsSubscription(&message))
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "getEventNotification LSMessageIsSubscription success\n");
    if (!LSSubscriptionAdd(this->get(), CONST_EVENT_NOTIFICATION, &message, &error))
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "getEventNotification LSSubscriptionAdd failed\n");
      LSErrorPrint(&error, stderr);
    }
  }

  int ndevhandle = CommandManager::getInstance().getCameraHandle(ndev_id);
  CAMERA_FORMAT format;
  CommandManager::getInstance().getFormat(ndevhandle, &format);

  CAMERA_PROPERTIES_T property;
  CommandManager::getInstance().getProperty(ndevhandle, &property);

  // create json string now for reply
  std::string output_reply = objevent_.createEventObjectSubscriptionJsonString(&format, &property);
  PMLOG_INFO(CONST_MODULE_LUNA, "getEventNotification output_reply %s\n", output_reply.c_str());

  LS::Message request(&message);
  request.respond(output_reply.c_str());

  LSErrorFree(&error);

  return true;
}

bool CameraService::getFd(LSMessage &message)
{
  auto *payload = LSMessageGetPayload(&message);
  LS::Message request(&message);
  DEVICE_RETURN_CODE_T err_id = DEVICE_OK;

  int shmfd = -1;
  GetFdMethod obj_getfd;
  obj_getfd.getObject(payload, getFdSchema);

  int ndevhandle = obj_getfd.getDeviceHandle();

  if (n_invalid_id == ndevhandle)
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::DEVICE_ERROR_JSON_PARSING\n");
    err_id = DEVICE_ERROR_JSON_PARSING;
    obj_getfd.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
  }
  else
  {
    PMLOG_INFO(CONST_MODULE_LUNA, "CameraService::getFd ndevhandle %d\n", ndevhandle);

    err_id = CommandManager::getInstance().getFd(ndevhandle, &shmfd);

    if(err_id == DEVICE_OK)
    {
      obj_getfd.setMethodReply(CONST_PARAM_VALUE_TRUE, (int)err_id, getErrorString(err_id));

      // create json string now for reply
      std::string output_reply = obj_getfd.createObjectJsonString();
      PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

      LS::Payload response_payload(output_reply.c_str());
      response_payload.attachFd(shmfd);//attach a fd here
      request.respond(std::move(response_payload));
      return true;
    }
    else
    {
      PMLOG_INFO(CONST_MODULE_LUNA, "CameraService:: %s\n", getErrorString(err_id));
      obj_getfd.setMethodReply(CONST_PARAM_VALUE_FALSE, (int)err_id, getErrorString(err_id));
    }
  }
  // create json string now for reply
  std::string output_reply = obj_getfd.createObjectJsonString();
  PMLOG_INFO(CONST_MODULE_LUNA, "output_reply %s\n", output_reply.c_str());

  request.respond(output_reply.c_str());
  return true;
}

int main(int argc, char *argv[])
{
  install_handler_service_crash();
  
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


void signal_handler_service_crash(int sig)
{
    CommandManager::getInstance().handleCrash();
    exit(1);
}

void install_handler_service_crash()
{
    sigact_service_crash.sa_handler = signal_handler_service_crash;
    sigfillset(&sigact_service_crash.sa_mask);
    sigact_service_crash.sa_flags = SA_RESTART;
    for (int i = SIGHUP; i <= SIGSYS; i++)
    {
        if (i == SIGKILL || i == SIGSTOP)
        {
            continue;
        }
        if (sigaction(i, &sigact_service_crash, NULL) == -1)
        {
            PMLOG_ERROR(CONST_MODULE_LUNA, "install signal handler for signal %d :: FAIL\n", i);
        }
    }
}
