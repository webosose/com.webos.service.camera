// Copyright (c) 2019 LG Electronics, Inc.
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
  GetCameraListMethod() { n_camcount_ = 0; }
  ~GetCameraListMethod() {}

  void setCameraList(const std::string& str_id, int count) { str_list_[count] = str_id; }
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

private:
  std::string str_list_[CONST_MAX_DEVICE_COUNT];
  MethodReply objreply_;
  int n_camcount_;
};

class OpenMethod
{
public:
  OpenMethod() { n_devicehandle_ = -1; }
  ~OpenMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  void setCameraId(const std::string& devid) { str_devid_ = devid; }
  std::string getCameraId() const { return str_devid_; }

  void setAppPriority(const std::string& priority) { str_priority_ = priority; }
  std::string getAppPriority() const { return str_priority_; }

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
  MethodReply objreply_;
};

class StartPreviewMethod
{
public:
  StartPreviewMethod()
  {
    n_devicehandle_ = -1;
    n_keyvalue_ = 0;
  }
  ~StartPreviewMethod() {}

  void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
  int getDeviceHandle() const { return n_devicehandle_; }

  void setParams(camera_memory_source_t rin_params)
  {
    ro_params_.str_memorysource = rin_params.str_memorysource;
    ro_params_.str_memorytype = rin_params.str_memorytype;
  }
  camera_memory_source_t rGetParams() const { return ro_params_; }

  void setKeyValue(int key) { n_keyvalue_ = key; }
  int getKeyValue() const { return n_keyvalue_; }

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
  camera_memory_source_t ro_params_;
  int n_keyvalue_;
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

  void setnImage(int nimage) { n_image_ = nimage; }
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

class StopPreviewCaptureCloseMethod
{
public:
  StopPreviewCaptureCloseMethod() { n_devicehandle_ = -1; }
  ~StopPreviewCaptureCloseMethod() {}

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

class GetInfoMethod
{
public:
  GetInfoMethod();
  ~GetInfoMethod() {}

  void setDeviceId(const std::string& devid) { str_deviceid_ = devid; }
  std::string strGetDeviceId() const { return str_deviceid_; }

  void setCameraInfo(camera_device_info_t r_ininfo)
  {
    strncpy(ro_info_.str_devicename, r_ininfo.str_devicename, 32);
    ro_info_.b_builtin = r_ininfo.b_builtin;
    ro_info_.n_codec = r_ininfo.n_codec;
    ro_info_.n_format = r_ininfo.n_format;
    ro_info_.n_devicetype = r_ininfo.n_devicetype;
    ro_info_.n_maxpictureheight = r_ininfo.n_maxpictureheight;
    ro_info_.n_maxpicturewidth = r_ininfo.n_maxpicturewidth;
    ro_info_.n_maxvideoheight = r_ininfo.n_maxvideoheight;
    ro_info_.n_maxvideowidth = r_ininfo.n_maxvideowidth;
    ro_info_.n_samplingrate = r_ininfo.n_samplingrate;
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
  std::string createInfoObjectJsonString() const;

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

  void setCameraProperties(CAMERA_PROPERTIES_T rin_info)
  {
    ro_camproperties_.nZoom = rin_info.nZoom;
    ro_camproperties_.nGridZoomX = rin_info.nGridZoomX;
    ro_camproperties_.nGridZoomY = rin_info.nGridZoomY;
    ro_camproperties_.nPan = rin_info.nPan;
    ro_camproperties_.nTilt = rin_info.nTilt;
    ro_camproperties_.nContrast = rin_info.nContrast;
    ro_camproperties_.nBrightness = rin_info.nBrightness;
    ro_camproperties_.nSaturation = rin_info.nSaturation;
    ro_camproperties_.nSharpness = rin_info.nSharpness;
    ro_camproperties_.nHue = rin_info.nHue;
    ro_camproperties_.nWhiteBalanceTemperature = rin_info.nWhiteBalanceTemperature;
    ro_camproperties_.nGain = rin_info.nGain;
    ro_camproperties_.nGamma = rin_info.nGamma;
    ro_camproperties_.nFrequency = rin_info.nFrequency;
    ro_camproperties_.bMirror = rin_info.bMirror;
    ro_camproperties_.nExposure = rin_info.nExposure;
    ro_camproperties_.bAutoExposure = rin_info.bAutoExposure;
    ro_camproperties_.bAutoWhiteBalance = rin_info.bAutoWhiteBalance;
    ro_camproperties_.nBitrate = rin_info.nBitrate;
    ro_camproperties_.nFramerate = rin_info.nFramerate;
    ro_camproperties_.ngopLength = rin_info.ngopLength;
    ro_camproperties_.bLed = rin_info.bLed;
    ro_camproperties_.bYuvMode = rin_info.bYuvMode;
    ro_camproperties_.nIllumination = rin_info.nIllumination;
    ro_camproperties_.bBacklightCompensation = rin_info.bBacklightCompensation;
    ro_camproperties_.nMicMaxGain = rin_info.nMicMaxGain;
    ro_camproperties_.nMicMinGain = rin_info.nMicMinGain;
    ro_camproperties_.nMicGain = rin_info.nMicGain;
    ro_camproperties_.bMicMute = rin_info.bMicMute;
    // update resolution structure
    ro_camproperties_.st_resolution.n_formatindex = rin_info.st_resolution.n_formatindex;
    for (int n = 0; n < rin_info.st_resolution.n_formatindex; n++)
    {
      ro_camproperties_.st_resolution.e_format[n] = rin_info.st_resolution.e_format[n];
      ro_camproperties_.st_resolution.n_frameindex[n] = rin_info.st_resolution.n_frameindex[n];
      for (int count = 0; count < rin_info.st_resolution.n_frameindex[n]; count++)
      {
        ro_camproperties_.st_resolution.n_height[n][count] =
            rin_info.st_resolution.n_height[n][count];
        ro_camproperties_.st_resolution.n_width[n][count] =
            rin_info.st_resolution.n_width[n][count];
        memset(ro_camproperties_.st_resolution.c_res[count], '\0',
               sizeof(ro_camproperties_.st_resolution.c_res[count]));
        strncpy(ro_camproperties_.st_resolution.c_res[count], rin_info.st_resolution.c_res[count],
                sizeof(ro_camproperties_.st_resolution.c_res[count]));
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
  void getSetPropertiesObject(const char *, const char *);
  std::string createGetPropertiesObjectJsonString() const;
  std::string createSetPropertiesObjectJsonString() const;

private:
  int n_devicehandle_;
  CAMERA_PROPERTIES_T ro_camproperties_;
  std::vector<std::string> str_params_;
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

class EventNotification
{
public:
  EventNotification()
  {
    etype_ = EventType::EVENT_TYPE_NONE;
    pdata_ = nullptr;
    b_issubscribed_ = false;
  }
  ~EventNotification() {}

  void getEventObject(const char *, const char *);
  std::string createEventObjectJsonString(void *) const;
  void setEventType(EventType etype) { etype_ = etype; }
  void setEventData(void *data) { pdata_ = data; }
  void setCameraId(const std::string& camid) { strcamid_ = camid; }

private:
  EventType etype_;
  void *pdata_;
  bool b_issubscribed_;
  std::string strcamid_;
};

void createJsonStringFailure(MethodReply, jvalue_ref &);
void createGetPropertiesJsonString(CAMERA_PROPERTIES_T *, void *, jvalue_ref &);
void createGetPropertiesOutputParamJsonString(const std::string, CAMERA_PROPERTIES_T *,
                                              jvalue_ref &);

#endif /*SRC_SERVICE_JSON_PARSER_H_*/
