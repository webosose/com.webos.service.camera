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

#include <stdio.h>
#include <iostream>
#include <string>
#include "json_utils.h"
#include "service_types.h"
#include "constants.h"

class MethodReply
{
  public:
    MethodReply() {}
    ~MethodReply() {}

    void setReturnValue(bool retval) { b_retvalue_ = retval; }
    bool bGetReturnValue() { return b_retvalue_; }

    void setErrorCode(int errorcode) { n_errorcode_ = errorcode; }
    int getErrorCode() { return n_errorcode_; }

    void setErrorText(std::string errortext) { str_errortext_ = errortext; }
    std::string strGetErrorText() { return str_errortext_; }

  private:
    bool b_retvalue_;
    int n_errorcode_;
    std::string str_errortext_;
};

class GetCameraListMethod
{
  public:
    GetCameraListMethod() {}
    ~GetCameraListMethod() {}

    void setCameraList(std::string str_id, int count) { str_list_[count] = str_id; }
    std::string strGetCameraList(int count) { return str_list_[count]; }

    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() { return objreply_; }

    void setCameraCount(int count) { n_camcount_ = count; }
    int getCameraCount() { return n_camcount_; }

    bool getCameraListObject(const char *, const char *);
    std::string createCameraListObjectJsonString();

  private:
    std::string str_list_[CONST_MAX_DEVICE_COUNT];
    MethodReply objreply_;
    int n_camcount_;
};

class OpenMethod
{
  public:
    OpenMethod() {}
    ~OpenMethod() {}

    void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
    int getDeviceHandle() { return n_devicehandle_; }

    void setCameraId(std::string devid) { str_devid_ = devid; }
    std::string getCameraId() { return str_devid_; }

    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() { return objreply_; }

    void getOpenObject(const char *, const char *);
    std::string createOpenObjectJsonString();

  private:
    int n_devicehandle_;
    std::string str_devid_;
    MethodReply objreply_;
};

class StartPreviewMethod
{
  public:
    StartPreviewMethod() {}
    ~StartPreviewMethod() {}

    void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
    int getDeviceHandle() { return n_devicehandle_; }

    void setParams(camera_memory_source_t rin_params)
    {
        ro_params_.str_memorysource = rin_params.str_memorysource;
        ro_params_.str_memorytype = rin_params.str_memorytype;
    }
    camera_memory_source_t rGetParams() { return ro_params_; }

    void setKeyValue(int key) { n_keyvalue_ = key; }
    int getKeyValue() { return n_keyvalue_; }

    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() { return objreply_; }

    void getStartPreviewObject(const char *, const char *);
    std::string createStartPreviewObjectJsonString();

  private:
    int n_devicehandle_;
    camera_memory_source_t ro_params_;
    int n_keyvalue_;
    MethodReply objreply_;
};

class StartCaptureMethod
{
  public:
    StartCaptureMethod() {}
    ~StartCaptureMethod() {}

    void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
    int getDeviceHandle() { return n_devicehandle_; }

    void setCameraParams(camera_format_t r_inparams)
    {
        r_cameraparams_.n_width = r_inparams.n_width;
        r_cameraparams_.n_height = r_inparams.n_height;
        r_cameraparams_.e_format = r_inparams.e_format;
    }
    camera_format_t rGetParams() { return r_cameraparams_; }

    void setnImage(int nimage) { n_image_ = nimage; }
    int getnImage() { return n_image_; }

    void setCaptureMode(std::string capturemode) { str_mode_ = capturemode; }
    std::string strGetCaptureMode() { return str_mode_; }

    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() { return objreply_; }

    void getStartCaptureObject(const char *, const char *);
    std::string createStartCaptureObjectJsonString();

  private:
    int n_devicehandle_;
    camera_format_t r_cameraparams_;
    int n_image_;
    std::string str_mode_;
    MethodReply objreply_;
};

class StopPreviewCaptureCloseMethod
{
  public:
    StopPreviewCaptureCloseMethod() {}
    ~StopPreviewCaptureCloseMethod() {}

    void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
    int getDeviceHandle() { return n_devicehandle_; }

    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() { return objreply_; }

    void getObject(const char *, const char *);
    std::string createObjectJsonString();

  private:
    int n_devicehandle_;
    MethodReply objreply_;
};

class GetInfoMethod
{
  public:
    GetInfoMethod() {}
    ~GetInfoMethod() {}

    void setDeviceId(std::string devid) { str_deviceid_ = devid; }
    std::string strGetDeviceId() { return str_deviceid_; }

    void setCameraInfo(camera_info_t r_ininfo)
    {
        ro_info_.str_name = r_ininfo.str_name;
        ro_info_.b_builtin = r_ininfo.b_builtin;
        ro_info_.n_codec = r_ininfo.n_codec;
        ro_info_.n_format = r_ininfo.n_format;
        ro_info_.e_type = r_ininfo.e_type;
        ro_info_.n_maxpictureheight = r_ininfo.n_maxpictureheight;
        ro_info_.n_maxpicturewidth = r_ininfo.n_maxpicturewidth;
        ro_info_.n_maxvideoheight = r_ininfo.n_maxvideoheight;
        ro_info_.n_maxvideowidth = r_ininfo.n_maxvideowidth;
        ro_info_.n_samplingrate = r_ininfo.n_samplingrate;
    }
    camera_info_t rGetCameraInfo() { return ro_info_; }

    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() { return objreply_; }

    void getInfoObject(const char *, const char *);
    std::string createInfoObjectJsonString();

  private:
    std::string str_deviceid_;
    camera_info_t ro_info_;
    MethodReply objreply_;
};

class GetSetPropertiesMethod
{
  public:
    GetSetPropertiesMethod() {}
    ~GetSetPropertiesMethod() {}

    void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
    int getDeviceHandle() { return n_devicehandle_; }

    void setCameraProperties(camera_properties_t rin_info)
    {
        ro_camproperties_.n_zoom = rin_info.n_zoom;
        ro_camproperties_.n_gridzoom_x = rin_info.n_gridzoom_x;
        ro_camproperties_.n_gridzoom_y = rin_info.n_gridzoom_y;
        ro_camproperties_.n_pan = rin_info.n_pan;
        ro_camproperties_.n_tilt = rin_info.n_tilt;
        ro_camproperties_.n_contrast = rin_info.n_contrast;
        ro_camproperties_.n_brightness = rin_info.n_brightness;
        ro_camproperties_.n_saturation = rin_info.n_saturation;
        ro_camproperties_.n_sharpness = rin_info.n_sharpness;
        ro_camproperties_.n_hue = rin_info.n_hue;
        ro_camproperties_.n_whitebalancetemperature = rin_info.n_whitebalancetemperature;
        ro_camproperties_.n_gain = rin_info.n_gain;
        ro_camproperties_.n_gamma = rin_info.n_gamma;
        ro_camproperties_.n_frequency = rin_info.n_frequency;
        ro_camproperties_.b_mirror = rin_info.b_mirror;
        ro_camproperties_.n_exposure = rin_info.n_exposure;
        ro_camproperties_.b_autoexposure = rin_info.b_autoexposure;
        ro_camproperties_.b_autowhitebalance = rin_info.b_autowhitebalance;
        ro_camproperties_.n_bitrate = rin_info.n_bitrate;
        ro_camproperties_.n_framerate = rin_info.n_framerate;
        ro_camproperties_.ngop_length = rin_info.ngop_length;
        ro_camproperties_.b_led = rin_info.b_led;
        ro_camproperties_.b_yuvmode = rin_info.b_yuvmode;
        ro_camproperties_.n_illumination = rin_info.n_illumination;
        ro_camproperties_.b_backlightcompensation = rin_info.b_backlightcompensation;
        ro_camproperties_.n_mic_maxgain = rin_info.n_mic_maxgain;
        ro_camproperties_.n_mic_mingain = rin_info.n_mic_mingain;
        ro_camproperties_.n_micgain = rin_info.n_micgain;
        ro_camproperties_.b_micmute = rin_info.b_micmute;
    }
    camera_properties_t rGetCameraProperties() { return ro_camproperties_; }

    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() { return objreply_; }

    void getPropertiesObject(const char *, const char *);
    void getSetPropertiesObject(const char *, const char *);
    std::string createGetPropertiesObjectJsonString();
    std::string createSetPropertiesObjectJsonString();

  private:
    int n_devicehandle_;
    camera_properties_t ro_camproperties_;
    MethodReply objreply_;
};

class SetFormatMethod
{
  public:
    SetFormatMethod() {}
    ~SetFormatMethod() {}

    void setDeviceHandle(int devhandle) { n_devicehandle_ = devhandle; }
    int getDeviceHandle() { return n_devicehandle_; }

    void setCameraFormat(camera_format_t rin_params)
    {
        ro_params_.n_width = rin_params.n_width;
        ro_params_.n_height = rin_params.n_height;
        ro_params_.e_format = rin_params.e_format;
    }
    camera_format_t rGetCameraFormat() { return ro_params_; }

    void setMethodReply(bool returnvalue, int errorcode, std::string errortext)
    {
        objreply_.setReturnValue(returnvalue);
        objreply_.setErrorCode(errorcode);
        objreply_.setErrorText(errortext);
    }
    MethodReply getMethodReply() { return objreply_; }

    void getSetFormatObject(const char *, const char *);
    std::string createSetFormatObjectJsonString();

  private:
    int n_devicehandle_;
    camera_format_t ro_params_;
    MethodReply objreply_;
};

void createJsonStringFailure(MethodReply, jvalue_ref &);

#endif /*SRC_SERVICE_JSON_PARSER_H_*/
