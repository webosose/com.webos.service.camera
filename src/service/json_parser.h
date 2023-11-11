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

#ifndef JSON_PARSER_H_
#define JSON_PARSER_H_

#include "camera_types.h"
#include "constants.h"
#include "json_utils.h"
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <map>

class MethodReply
{
public:
  MethodReply()
  {
    b_retvalue_ = false;
    n_errorcode_ = 0;
  }
  ~MethodReply() {}

  void setReturnValue(bool retval) { b_retvalue_ = retval; }
  bool bGetReturnValue() const { return b_retvalue_; }

  void setErrorCode(int errorcode) { n_errorcode_ = errorcode; }
  int getErrorCode() const { return n_errorcode_; }

  void setErrorText(const std::string& errortext) { str_errortext_ = errortext; }
  std::string strGetErrorText() const { return str_errortext_; }

private:
  bool b_retvalue_;
  int n_errorcode_;
  std::string str_errortext_;
};

class GetCameraListMethod
{
public:
    GetCameraListMethod()
    {
        n_camcount_     = 0;
        b_issubscribed_ = false;
    }
    ~GetCameraListMethod() {}

  void setCameraList(const std::string& str_id, unsigned int count) { str_list_[count] = str_id; }
  std::string strGetCameraList(int count) const { return str_list_[count]; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void setCameraCount(int count) { n_camcount_ = count; }
  int getCameraCount() const { return n_camcount_; }

  static bool getCameraListObject(const char *, const char *);
  std::string createCameraListObjectJsonString() const;
  void setSubcribed(bool subscribed) { b_issubscribed_ = subscribed; }

private:
  std::string str_list_[CONST_MAX_DEVICE_COUNT];
  MethodReply objreply_;
  int n_camcount_;
  bool b_issubscribed_;
};

class OpenMethod
{
public:
  OpenMethod() {
    n_devicehandle_ = -1;
    n_client_pid_ = -1;
    n_client_sig_ = -1;
  }
  ~OpenMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  void setCameraId(const std::string& devid) { str_devid_ = devid; }
  std::string getCameraId() const { return str_devid_; }

  void setAppPriority(const std::string& priority) { str_priority_ = priority; }
  std::string getAppPriority() const { return str_priority_; }

  void setClientProcessId(int pid) { n_client_pid_ = pid; }
  int getClientProcessId() const { return n_client_pid_; }

  void setClientSignal(int sig) { n_client_sig_ = sig; }
  int getClientSignal() const { return n_client_sig_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getOpenObject(const char *, const char *);
  std::string createOpenObjectJsonString() const;

private:
  int n_devicehandle_;
  std::string str_devid_;
  std::string str_priority_;
  int n_client_pid_;
  int n_client_sig_;
  MethodReply objreply_;
};

class StartCameraMethod
{
public:
  StartCameraMethod()
  {
    n_devicehandle_ = -1;
    n_keyvalue_ = 0;
  }
  ~StartCameraMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  void setMemParams(camera_memory_source_t rin_params)
  {
    ro_mem_params_.str_memorysource = rin_params.str_memorysource;
    ro_mem_params_.str_memorytype = rin_params.str_memorytype;
  }
  camera_memory_source_t rGetMemParams() const { return ro_mem_params_; }

  void setKeyValue(int key) { n_keyvalue_ = key; }
  int getKeyValue() const { return n_keyvalue_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getStartCameraObject(const char *, const char *);
  std::string createStartCameraObjectJsonString() const;

private:
  int n_devicehandle_;
  camera_memory_source_t ro_mem_params_;
  int n_keyvalue_;
  MethodReply objreply_;
};

class StartPreviewMethod
{
public:
  StartPreviewMethod()
  {
    n_devicehandle_ = -1;
    n_keyvalue_ = 0;
    window_id_ = "";
    media_id_ = "";
  }
  ~StartPreviewMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  void setMemParams(camera_memory_source_t rin_params)
  {
    ro_mem_params_.str_memorysource = rin_params.str_memorysource;
    ro_mem_params_.str_memorytype = rin_params.str_memorytype;
  }
  camera_memory_source_t rGetMemParams() const { return ro_mem_params_; }

  void setDpyParams(camera_display_source_t rin_params)
  {
    ro_dpy_params_.str_window_id = rin_params.str_window_id;
  }
  camera_display_source_t rGetDpyParams() const { return ro_dpy_params_; }

  void setKeyValue(int key) { n_keyvalue_ = key; }
  int getKeyValue() const { return n_keyvalue_; }

  void setMediaIdValue(std::string media_id) { media_id_ = std::move(media_id); }
  std::string getMediaIdValue() const { return media_id_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getStartPreviewObject(const char *, const char *);
  std::string createStartPreviewObjectJsonString() const;

private:
  int n_devicehandle_;
  camera_memory_source_t ro_mem_params_;
  camera_display_source_t ro_dpy_params_;
  int n_keyvalue_;
  std::string window_id_;
  std::string media_id_;
  MethodReply objreply_;
};

class StartCaptureMethod
{
public:
  StartCaptureMethod();
  ~StartCaptureMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  void setCameraParams(CAMERA_FORMAT r_inparams)
  {
    r_cameraparams_.nWidth = r_inparams.nWidth;
    r_cameraparams_.nHeight = r_inparams.nHeight;
    r_cameraparams_.eFormat = r_inparams.eFormat;
  }
  CAMERA_FORMAT rGetParams() const { return r_cameraparams_; }

  int getnImage() const { return n_image_; }

  void setImagePath(const std::string& path) { str_path_ = path; }
  std::string getImagePath() const { return str_path_; }

  void setCaptureMode(const std::string& capturemode) { str_mode_ = capturemode; }
  std::string strGetCaptureMode() const { return str_mode_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getStartCaptureObject(const char *, const char *);
  std::string createStartCaptureObjectJsonString() const;

private:
  int n_devicehandle_;
  CAMERA_FORMAT r_cameraparams_;
  int n_image_;
  std::string str_mode_;
  std::string str_path_;
  MethodReply objreply_;
};

class CaptureMethod
{
public:
  CaptureMethod();
  ~CaptureMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  int getnImage() const { return n_image_; }

  void setImagePath(const std::string& path) { str_path_ = path; }
  std::string getImagePath() const { return str_path_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getCaptureObject(const char *, const char *);
  std::string createCaptureObjectJsonString(std::vector<std::string> &capturedFiles) const;

private:
  int n_devicehandle_;
  int n_image_;
  std::string str_path_;
  MethodReply objreply_;
};

class StopCameraPreviewCaptureCloseMethod
{
public:
  StopCameraPreviewCaptureCloseMethod()
  {
    n_devicehandle_ = -1;
    n_client_pid_ = -1;
  }
  ~StopCameraPreviewCaptureCloseMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  void setClientProcessId(int pid) { n_client_pid_ = pid; }
  int getClientProcessId() const { return n_client_pid_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getObject(const char *, const char *);
  std::string createObjectJsonString() const;

private:
  int n_devicehandle_;
  int n_client_pid_;
  MethodReply objreply_;
};

class GetInfoMethod
{
public:
  GetInfoMethod();
  ~GetInfoMethod() {}

  void setDeviceId(const std::string& devid) { str_deviceid_ = devid; }
  std::string strGetDeviceId() const { return str_deviceid_; }

  void setCameraInfo(const camera_device_info_t& r_ininfo)
  {
      ro_info_.str_devicename = r_ininfo.str_devicename;
      ro_info_.str_vendorid   = r_ininfo.str_vendorid;
      ro_info_.str_productid  = r_ininfo.str_productid;
      ro_info_.b_builtin      = r_ininfo.b_builtin;
      ro_info_.n_devicetype   = r_ininfo.n_devicetype;

      // update resolution structure
      for (auto const &v : r_ininfo.stResolution)
      {
          std::vector<std::string> c_res;
          c_res.clear();
          c_res.assign(v.c_res.begin(), v.c_res.end());
          ro_info_.stResolution.emplace_back(c_res, v.e_format);
      }
  }

  camera_device_info_t rGetCameraInfo() const { return ro_info_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getInfoObject(const char *, const char *);
  std::string createInfoObjectJsonString(bool supported) const;

private:
  std::string str_deviceid_;
  camera_device_info_t ro_info_;
  MethodReply objreply_;
};

class GetSetPropertiesMethod
{
public:
  GetSetPropertiesMethod();
  ~GetSetPropertiesMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }
  void setCameraId(const std::string &devid) { str_devid_ = devid; }
  std::string getCameraId() const { return str_devid_; }

  void setCameraProperties(const CAMERA_PROPERTIES_T& rin_info)
  {
    //update query data
    for (int i = 0; i < PROPERTY_END; i++)
    {
      for (int j = 0; j < QUERY_END; j++)
      {
        ro_camproperties_.stGetData.data[i][j] = rin_info.stGetData.data[i][j];
      }
    }
  }
  CAMERA_PROPERTIES_T rGetCameraProperties() const { return ro_camproperties_; }

  void setParams(const std::string& param) { str_params_.push_back(param); }
  std::vector<std::string> getParams() { return str_params_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getPropertiesObject(const char *, const char *);
  bool isParamsEmpty(const char *, const char *);
  void getSetPropertiesObject(const char *, const char *);
  std::string createGetPropertiesObjectJsonString() const;
  std::string createSetPropertiesObjectJsonString() const;
  void setSubcribed(bool subscribed) { b_issubscribed_ = subscribed; }

private:
  int n_devicehandle_;
  CAMERA_PROPERTIES_T ro_camproperties_;
  std::vector<std::string> str_params_;
  std::string str_devid_;
  bool b_issubscribed_;
  MethodReply objreply_;
};

class SetFormatMethod
{
public:
  SetFormatMethod();
  ~SetFormatMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  void setCameraFormat(CAMERA_FORMAT rin_params)
  {
    ro_params_.nWidth = rin_params.nWidth;
    ro_params_.nHeight = rin_params.nHeight;
    ro_params_.eFormat = rin_params.eFormat;
    ro_params_.nFps = rin_params.nFps;
  }
  CAMERA_FORMAT rGetCameraFormat() const { return ro_params_; }

  void setMethodReply(bool returnvalue, int errorcode, const std::string& errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getSetFormatObject(const char *, const char *);
  std::string createSetFormatObjectJsonString() const;

private:
  int n_devicehandle_;
  CAMERA_FORMAT ro_params_;
  MethodReply objreply_;
};

class GetFdMethod
{
public:
  GetFdMethod() { n_devicehandle_ = -1; };
  ~GetFdMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getObject(const char *, const char *);
  std::string createObjectJsonString() const;

private:
  int n_devicehandle_;
  MethodReply objreply_;
};


class GetSolutionsMethod
{
public:
  GetSolutionsMethod() { n_devicehandle_ = -1; };
  ~GetSolutionsMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }
  void setCameraId(const std::string &devid) { str_devid_ = devid; }
  std::string getCameraId() const { return str_devid_; }
  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getObject(const char *, const char *);
  std::string createObjectJsonString(std::vector<std::string> supportedSolutionList,
                                     std::vector<std::string> enabledSolutionList) const;

private:
  int n_devicehandle_;
  std::string str_devid_;
  MethodReply objreply_;
};

class SetSolutionsMethod
{
public:
  SetSolutionsMethod();
  ~SetSolutionsMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }
  void setCameraId(const std::string &devid) { str_devid_ = devid; }
  std::string getCameraId() const { return str_devid_; }
  void setEnableSolutionList(const std::string &solution)
  {
    str_enable_solutions_.push_back(solution);
  }
  std::vector<std::string> getEnableSolutionList() { return str_enable_solutions_; }
  void setDisbleSolutionList(const std::string &solution)
  {
    str_disable_solutions_.push_back(solution);
  }
  std::vector<std::string> getDisableSolutionList() { return str_disable_solutions_; }
  bool isEmpty();
  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
  {
    objreply_.setReturnValue(returnvalue);
    objreply_.setErrorCode(errorcode);
    objreply_.setErrorText(errortext);
  }
  MethodReply getMethodReply() const { return objreply_; }

  void getObject(const char *, const char *);
  std::string createObjectJsonString() const;

private:
  int n_devicehandle_;
  std::string str_devid_;
  std::vector<std::string> str_enable_solutions_;
  std::vector<std::string> str_disable_solutions_;
  MethodReply objreply_;
};

class GetFormatMethod
{
public:
    GetFormatMethod();
    ~GetFormatMethod() {}

    void setCameraId(const std::string &devid) { str_devid_ = devid; }
    std::string getCameraId() const { return str_devid_; }
    void setSubcribed(bool subscribed) { b_issubscribed_ = subscribed; }

    void setCameraFormat(CAMERA_FORMAT rin_params)
    {
        ro_params_.nWidth  = rin_params.nWidth;
        ro_params_.nHeight = rin_params.nHeight;
        ro_params_.eFormat = rin_params.eFormat;
        ro_params_.nFps    = rin_params.nFps;
    }
    CAMERA_FORMAT rGetCameraFormat() const { return ro_params_; }

    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() const { return objreply_; }

    void getObject(const char *, const char *);
    std::string createObjectJsonString() const;

private:
    std::string str_devid_;
    CAMERA_FORMAT ro_params_;
    bool b_issubscribed_;
    MethodReply objreply_;
};

class EventNotificationMethod
{
public:
    EventNotificationMethod()
    {
        b_iserror_      = true;
        b_issubscribed_ = false;
    }
    ~EventNotificationMethod() {}

    void getEventObject(const char *, const char *);
    void setSubcribed(bool subscribed) { b_issubscribed_ = subscribed; }
    bool getIsErrorFromParam() { return b_iserror_; }
    void setIsErrorParam(bool error) { b_iserror_ = error; }
    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() const { return objreply_; }
    std::string createObjectJsonString() const;

private:
    bool b_iserror_;
    bool b_issubscribed_;
    MethodReply objreply_;
};

void createJsonStringFailure(MethodReply, jvalue_ref &);

#endif /*SRC_SERVICE_JSON_PARSER_H_*/
