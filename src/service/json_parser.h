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

#ifndef JSON_PARSER_H_
#define JSON_PARSER_H_

#include "camera_types.h"
#include "constants.h"
#include "json_utils.h"
#include <iostream>
#include <stdio.h>
#include <string>

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

  void setErrorText(std::string errortext) { str_errortext_ = errortext; }
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

  void setCameraList(std::string str_id, int count) { str_list_[count] = str_id; }
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

  void setCameraId(std::string devid) { str_devid_ = devid; }
  std::string getCameraId() const { return str_devid_; }

  void setAppPriority(std::string priority) { str_priority_ = priority; }
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

  void setCaptureMode(std::string capturemode) { str_mode_ = capturemode; }
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

  void setDeviceId(std::string devid) { str_deviceid_ = devid; }
  std::string strGetDeviceId() const { return str_deviceid_; }

  void setCameraInfo(CAMERA_INFO_T r_ininfo)
  {
    strncpy(ro_info_.strName, r_ininfo.strName, sizeof(r_ininfo.strName));
    ro_info_.bBuiltin = r_ininfo.bBuiltin;
    ro_info_.nCodec = r_ininfo.nCodec;
    ro_info_.nFormat = r_ininfo.nFormat;
    ro_info_.nType = r_ininfo.nType;
    ro_info_.nMaxPictureHeight = r_ininfo.nMaxPictureHeight;
    ro_info_.nMaxPictureWidth = r_ininfo.nMaxPictureWidth;
    ro_info_.nMaxVideoHeight = r_ininfo.nMaxVideoHeight;
    ro_info_.nMaxVideoWidth = r_ininfo.nMaxVideoWidth;
    ro_info_.nSamplingRate = r_ininfo.nSamplingRate;
  }
  CAMERA_INFO_T rGetCameraInfo() const { return ro_info_; }

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
  CAMERA_INFO_T ro_info_;
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
  }
  CAMERA_PROPERTIES_T rGetCameraProperties() const { return ro_camproperties_; }

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
  }
  CAMERA_FORMAT rGetCameraFormat() const { return ro_params_; }

  void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
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

void createJsonStringFailure(MethodReply, jvalue_ref &);

#endif /*SRC_SERVICE_JSON_PARSER_H_*/
