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

#include "json_parser.h"
#include "constants.h"
#include <iostream>

bool GetCameraListMethod::getCameraListObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retval = deSerialize(input, schemapath, j_obj);
  j_release(&j_obj);

  if (0 == retval)
  {
    return true;
  }
  else
  {
    return false;
  }
}

std::string GetCameraListMethod::createCameraListObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  jvalue_ref json_outdevicelistarray = jarray_create(NULL);

  std::string str_reply;

  MethodReply objreply = getMethodReply();

  if (objreply.bGetReturnValue())
  {
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(objreply.bGetReturnValue()));

    int count = getCameraCount();
    for (int i = 0; i < count; i++)
    {
      jvalue_ref json_outdevicelistitem = jobject_create();
      jobject_put(json_outdevicelistitem, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ID),
                  jstring_create(strGetCameraList(i).c_str()));
      jarray_append(json_outdevicelistarray, json_outdevicelistitem);
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEVICE_LIST), json_outdevicelistarray);
  }
  else
  {
    createJsonStringFailure(objreply, json_outobj);
  }

  str_reply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return str_reply;
}

void OpenMethod::getOpenObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retVal = deSerialize(input, schemapath, j_obj);

  if (0 == retVal)
  {
    raw_buffer str_id = jstring_get_fast(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_ID)));
    setCameraId(str_id.m_str);
    str_id = jstring_get_fast(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_APP_PRIORITY)));
    std::string priority = str_id.m_str;
    // parsing of argumnets to open call. Only empty or primary or secondary are valid
    if ((0 == priority.length()) || (cstr_primary == priority) || (cstr_secondary == priority))
    {
      setAppPriority(str_id.m_str);
    }
    else
    {
      setCameraId(cstr_invaliddeviceid);
    }
  }
  else
  {
    setCameraId(cstr_invaliddeviceid);
  }
  j_release(&j_obj);
}

std::string OpenMethod::createOpenObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  std::string str_reply;

  MethodReply obj_reply = getMethodReply();

  if (obj_reply.bGetReturnValue())
  {
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(obj_reply.bGetReturnValue()));
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_DEVICE_HANDLE),
                jnumber_create_i32(getDeviceHandle()));
  }
  else
  {
    createJsonStringFailure(obj_reply, json_outobj);
  }

  str_reply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return str_reply;
}

void StartPreviewMethod::getStartPreviewObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retval = deSerialize(input, schemapath, j_obj);

  if (0 == retval)
  {
    int n_devicehandle = n_invalid_id;
    jvalue_ref jnum = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE));
    jnumber_get_i32(jnum, &n_devicehandle);
    setDeviceHandle(n_devicehandle);

    jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
    raw_buffer type =
        jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_TYPE)));
    raw_buffer source =
        jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_SOURCE)));

    camera_memory_source_t r_cams_source;
    r_cams_source.str_memorysource = source.m_str;
    r_cams_source.str_memorytype = type.m_str;
    setParams(r_cams_source);
  }
  else
  {
    setDeviceHandle(n_invalid_id);
  }
  j_release(&j_obj);
}

std::string StartPreviewMethod::createStartPreviewObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  std::string str_reply;

  MethodReply obj_reply = getMethodReply();

  if (obj_reply.bGetReturnValue())
  {
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(obj_reply.bGetReturnValue()));
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_DEVICE_KEY), jnumber_create_i32(getKeyValue()));
  }
  else
  {
    createJsonStringFailure(obj_reply, json_outobj);
  }

  str_reply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return str_reply;
}

void StopPreviewCaptureCloseMethod::getObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retVal = deSerialize(input, schemapath, j_obj);

  if (0 == retVal)
  {
    int n_devicehandle = n_invalid_id;
    jnumber_get_i32(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE)), &n_devicehandle);
    setDeviceHandle(n_devicehandle);
  }
  else
  {
    setDeviceHandle(n_invalid_id);
  }
  j_release(&j_obj);
}

std::string StopPreviewCaptureCloseMethod::createObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  std::string str_reply;

  MethodReply objreply = getMethodReply();

  if (objreply.bGetReturnValue())
  {
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(objreply.bGetReturnValue()));
  }
  else
  {
    createJsonStringFailure(objreply, json_outobj);
  }

  str_reply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return str_reply;
}

StartCaptureMethod::StartCaptureMethod()
    : n_devicehandle_(n_invalid_id), r_cameraparams_(), n_image_(0)
{
}

void StartCaptureMethod::getStartCaptureObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retval = deSerialize(input, schemapath, j_obj);

  if (0 == retval)
  {
    int devicehandle = n_invalid_id;
    jvalue_ref jnum = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE));
    jnumber_get_i32(jnum, &devicehandle);
    setDeviceHandle(devicehandle);

    CAMERA_FORMAT rcamera_params;

    jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
    jvalue_ref j_params = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WIDTH));
    jnumber_get_i32(j_params, &rcamera_params.nWidth);

    j_params = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HEIGHT));
    jnumber_get_i32(j_params, &rcamera_params.nHeight);

    raw_buffer strformat =
        jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FORMAT)));
    std::string format = strformat.m_str;

    CAMERA_FORMAT_T nformat;
    convertFormatToCode(format, &nformat);
    rcamera_params.eFormat = nformat;

    setCameraParams(rcamera_params);

    raw_buffer strimg_mode =
        jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MODE)));
    str_mode_ = strimg_mode.m_str;

    if (0 == strcmp(str_mode_.c_str(), "MODE_BURST"))
    {
      j_params = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_DEFAULT_NIMAGE));
      jnumber_get_i32(j_params, &n_image_);
    }
    else if (0 == strcmp(str_mode_.c_str(), "MODE_ONESHOT"))
    {
      n_image_ = 1;
    }
    else
    {
      n_image_ = 0;
    }
  }
  else
  {
    setDeviceHandle(n_invalid_id);
  }
  j_release(&j_obj);
}

std::string StartCaptureMethod::createStartCaptureObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  std::string str_reply;

  MethodReply objreply = getMethodReply();

  if (objreply.bGetReturnValue())
  {
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(objreply.bGetReturnValue()));
  }
  else
  {
    createJsonStringFailure(objreply, json_outobj);
  }

  str_reply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return str_reply;
}

GetInfoMethod::GetInfoMethod() : ro_info_() {}

void GetInfoMethod::getInfoObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retval = deSerialize(input, schemapath, j_obj);

  if (0 == retval)
  {
    raw_buffer strid = jstring_get_fast(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_ID)));
    setDeviceId(strid.m_str);
  }
  else
  {
    setDeviceId(cstr_invaliddeviceid);
  }
  j_release(&j_obj);
}

std::string GetInfoMethod::createInfoObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  jvalue_ref json_info_obj = jobject_create();
  jvalue_ref json_detailsobj = jobject_create();
  jvalue_ref json_video_obj = jobject_create();
  jvalue_ref json_pictureobj = jobject_create();
  std::string strreply;

  MethodReply objreply = getMethodReply();

  if (objreply.bGetReturnValue())
  {
    char strformat[CONST_MAX_STRING_LENGTH];
    getFormatString(rGetCameraInfo().nFormat, strformat);
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(objreply.bGetReturnValue()));

    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_NAME),
                jstring_create(rGetCameraInfo().strName));
    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TYPE),
                jstring_create(getTypeString(rGetCameraInfo().nType)));
    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BUILTIN),
                jboolean_create((bool *)rGetCameraInfo().bBuiltin));

    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXWIDTH),
                jnumber_create_i32(rGetCameraInfo().nMaxVideoWidth));
    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXHEIGHT),
                jnumber_create_i32(rGetCameraInfo().nMaxVideoHeight));
    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FRAMERATE), jnumber_create_i32(30));
    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT), jstring_create(strformat));

    jobject_put(json_pictureobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXWIDTH),
                jnumber_create_i32(rGetCameraInfo().nMaxPictureWidth));
    jobject_put(json_pictureobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXHEIGHT),
                jnumber_create_i32(rGetCameraInfo().nMaxPictureHeight));
    jobject_put(json_pictureobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT),
                jstring_create(strformat));

    jobject_put(json_detailsobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VIDEO), json_video_obj);
    jobject_put(json_detailsobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PICTURE), json_pictureobj);

    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DETAILS), json_detailsobj);
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_INFO), json_info_obj);
  }
  else
  {
    createJsonStringFailure(objreply, json_outobj);
  }

  strreply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return strreply;
}

GetSetPropertiesMethod::GetSetPropertiesMethod()
    : n_devicehandle_(n_invalid_id), ro_camproperties_()
{
}

void GetSetPropertiesMethod::getPropertiesObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retval = deSerialize(input, schemapath, j_obj);

  if (0 == retval)
  {
    int devicehandle = n_invalid_id;
    jnumber_get_i32(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE)), &devicehandle);
    setDeviceHandle(devicehandle);
  }
  else
  {
    setDeviceHandle(n_invalid_id);
  }
  j_release(&j_obj);
}

std::string GetSetPropertiesMethod::createGetPropertiesObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  jvalue_ref json_outobjparams = jobject_create();
  std::string strreply;

  MethodReply objreply = getMethodReply();

  if (objreply.bGetReturnValue())
  {
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(objreply.bGetReturnValue()));

    if (rGetCameraProperties().nZoom != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM),
                  jnumber_create_i32(rGetCameraProperties().nZoom));
    if (rGetCameraProperties().nGridZoomX != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_COL),
                  jnumber_create_i32(rGetCameraProperties().nGridZoomX));
    if (rGetCameraProperties().nGridZoomY != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ROW),
                  jnumber_create_i32(rGetCameraProperties().nGridZoomY));
    if (rGetCameraProperties().nPan != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PAN),
                  jnumber_create_i32(rGetCameraProperties().nPan));
    if (rGetCameraProperties().nTilt != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TILT),
                  jnumber_create_i32(rGetCameraProperties().nTilt));
    if (rGetCameraProperties().nContrast != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST),
                  jnumber_create_i32(rGetCameraProperties().nContrast));
    if (rGetCameraProperties().nBrightness != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS),
                  jnumber_create_i32(rGetCameraProperties().nBrightness));
    if (rGetCameraProperties().nSaturation != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SATURATION),
                  jnumber_create_i32(rGetCameraProperties().nSaturation));
    if (rGetCameraProperties().nSharpness != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHARPNESS),
                  jnumber_create_i32(rGetCameraProperties().nSharpness));
    if (rGetCameraProperties().nHue != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HUE),
                  jnumber_create_i32(rGetCameraProperties().nHue));
    if (rGetCameraProperties().nWhiteBalanceTemperature != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE),
                  jnumber_create_i32(rGetCameraProperties().nWhiteBalanceTemperature));
    if (rGetCameraProperties().nGain != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAIN),
                  jnumber_create_i32(rGetCameraProperties().nGain));
    if (rGetCameraProperties().nGamma != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAMMA),
                  jnumber_create_i32(rGetCameraProperties().nGamma));
    if (rGetCameraProperties().nFrequency != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FREQUENCY),
                  jnumber_create_i32(rGetCameraProperties().nFrequency));
    if (rGetCameraProperties().bMirror != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIRROR),
                  jboolean_create(rGetCameraProperties().bMirror));
    if (rGetCameraProperties().nExposure != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE),
                  jnumber_create_i32(rGetCameraProperties().nExposure));
    if (rGetCameraProperties().bAutoExposure != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE),
                  jboolean_create(rGetCameraProperties().bAutoExposure));
    if (rGetCameraProperties().bAutoWhiteBalance != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE),
                  jboolean_create(rGetCameraProperties().bAutoWhiteBalance));
    if (rGetCameraProperties().nBitrate != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BITRATE),
                  jnumber_create_i32(rGetCameraProperties().nBitrate));
    if (rGetCameraProperties().nFramerate != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FRAMERATE),
                  jnumber_create_i32(rGetCameraProperties().nFramerate));
    if (rGetCameraProperties().ngopLength != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GOPLENGTH),
                  jnumber_create_i32(rGetCameraProperties().ngopLength));
    if (rGetCameraProperties().bLed != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_LED),
                  jboolean_create(rGetCameraProperties().bLed));
    if (rGetCameraProperties().bYuvMode != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_YUVMODE),
                  jboolean_create(rGetCameraProperties().bYuvMode));
    if (rGetCameraProperties().nIllumination != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ILLUMINATION),
                  jnumber_create_i32(rGetCameraProperties().nIllumination));
    if (rGetCameraProperties().bBacklightCompensation != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION),
                  jboolean_create(rGetCameraProperties().bBacklightCompensation));
    if (rGetCameraProperties().nMicMaxGain != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMAXGAIN),
                  jnumber_create_i32(rGetCameraProperties().nMicMaxGain));
    if (rGetCameraProperties().nMicMinGain != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMINGAIN),
                  jnumber_create_i32(rGetCameraProperties().nMicMinGain));
    if (rGetCameraProperties().nMicGain != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICGAIN),
                  jnumber_create_i32(rGetCameraProperties().nMicGain));
    if (rGetCameraProperties().bMicMute != CONST_VARIABLE_INITIALIZE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMUTE),
                  jboolean_create(rGetCameraProperties().bMicMute));

    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PARAMS), json_outobjparams);
  }
  else
  {
    createJsonStringFailure(objreply, json_outobj);
  }

  strreply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return strreply;
}

void GetSetPropertiesMethod::getSetPropertiesObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retval = deSerialize(input, schemapath, j_obj);

  if (0 == retval)
  {
    int devicehandle = n_invalid_id;
    jnumber_get_i32(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE)), &devicehandle);
    setDeviceHandle(devicehandle);

    CAMERA_PROPERTIES_T r_camproperties;

    jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
    jvalue_ref jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_ZOOM));
    jnumber_get_i32(jparams, &r_camproperties.nZoom);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_COL));
    jnumber_get_i32(jparams, &r_camproperties.nGridZoomX);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_ROW));
    jnumber_get_i32(jparams, &r_camproperties.nGridZoomY);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_PAN));
    jnumber_get_i32(jparams, &r_camproperties.nPan);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_TILT));
    jnumber_get_i32(jparams, &r_camproperties.nTilt);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_CONTRAST));
    jnumber_get_i32(jparams, &r_camproperties.nContrast);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_BIRGHTNESS));
    jnumber_get_i32(jparams, &r_camproperties.nBrightness);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_SATURATION));
    jnumber_get_i32(jparams, &r_camproperties.nSaturation);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_SHARPNESS));
    jnumber_get_i32(jparams, &r_camproperties.nSharpness);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HUE));
    jnumber_get_i32(jparams, &r_camproperties.nHue);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE));
    jnumber_get_i32(jparams, &r_camproperties.nWhiteBalanceTemperature);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_GAIN));
    jnumber_get_i32(jparams, &r_camproperties.nGain);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_GAMMA));
    jnumber_get_i32(jparams, &r_camproperties.nGamma);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FREQUENCY));
    jnumber_get_i32(jparams, &r_camproperties.nFrequency);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MIRROR));
    jboolean_get(jparams, (bool *)&r_camproperties.bMirror);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_EXPOSURE));
    jnumber_get_i32(jparams, &r_camproperties.nExposure);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_AUTOEXPOSURE));
    jboolean_get(jparams, (bool *)&r_camproperties.bAutoExposure);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_AUTOWHITEBALANCE));
    jboolean_get(jparams, (bool *)&r_camproperties.bAutoWhiteBalance);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_BITRATE));
    jnumber_get_i32(jparams, &r_camproperties.nBitrate);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FRAMERATE));
    jnumber_get_i32(jparams, &r_camproperties.nFramerate);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_GOPLENGTH));
    jnumber_get_i32(jparams, &r_camproperties.ngopLength);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_LED));
    jboolean_get(jparams, (bool *)&r_camproperties.bLed);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_YUVMODE));
    jboolean_get(jparams, (bool *)&r_camproperties.bYuvMode);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_ILLUMINATION));
    jnumber_get_i32(jparams, &r_camproperties.nIllumination);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION));
    jboolean_get(jparams, (bool *)&r_camproperties.bBacklightCompensation);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MICMAXGAIN));
    jnumber_get_i32(jparams, &r_camproperties.nMicMaxGain);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MICMINGAIN));
    jnumber_get_i32(jparams, &r_camproperties.nMicMinGain);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MICGAIN));
    jnumber_get_i32(jparams, &r_camproperties.nMicGain);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MICMUTE));
    jboolean_get(jparams, (bool *)&r_camproperties.bMicMute);

    setCameraProperties(r_camproperties);
  }
  else
  {
    setDeviceHandle(n_invalid_id);
  }
  j_release(&j_obj);
}

std::string GetSetPropertiesMethod::createSetPropertiesObjectJsonString() const
{
  jvalue_ref json_outbj = jobject_create();
  std::string strreply;

  MethodReply objreply = getMethodReply();

  if (objreply.bGetReturnValue())
  {
    jobject_put(json_outbj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(objreply.bGetReturnValue()));
  }
  else
  {
    createJsonStringFailure(objreply, json_outbj);
  }

  strreply = jvalue_stringify(json_outbj);
  j_release(&json_outbj);

  return strreply;
}

SetFormatMethod::SetFormatMethod() : n_devicehandle_(n_invalid_id), ro_params_() {}

void SetFormatMethod::getSetFormatObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retval = deSerialize(input, schemapath, j_obj);

  if (0 == retval)
  {
    int devicehandle = n_invalid_id;
    jnumber_get_i32(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE)), &devicehandle);
    setDeviceHandle(devicehandle);

    CAMERA_FORMAT rcameraparams;

    jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
    jvalue_ref jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WIDTH));
    jnumber_get_i32(jparams, &rcameraparams.nWidth);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HEIGHT));
    jnumber_get_i32(jparams, &rcameraparams.nHeight);
    raw_buffer strformat =
        jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FORMAT)));
    std::string format = strformat.m_str;

    CAMERA_FORMAT_T eformat;
    convertFormatToCode(format, &eformat);
    rcameraparams.eFormat = eformat;

    setCameraFormat(rcameraparams);
  }
  else
  {
    setDeviceHandle(n_invalid_id);
  }
  j_release(&j_obj);
}

std::string SetFormatMethod::createSetFormatObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  std::string strreply;

  MethodReply objreply = getMethodReply();

  if (objreply.bGetReturnValue())
  {
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(objreply.bGetReturnValue()));
  }
  else
  {
    createJsonStringFailure(objreply, json_outobj);
  }

  strreply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return strreply;
}

void createJsonStringFailure(MethodReply obj_reply, jvalue_ref &json_outobj)
{
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
              jboolean_create(obj_reply.bGetReturnValue()));
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_CODE),
              jnumber_create_i32(obj_reply.getErrorCode()));
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_TEXT),
              jstring_create(obj_reply.strGetErrorText().c_str()));

  return;
}

void EventNotification::getEventObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retval = deSerialize(input, schemapath, j_obj);

  if (0 == retval)
  {
    jboolean_get(jobject_get(j_obj, J_CSTR_TO_BUF("subscribe")), (bool *)&b_issubscribed_);
  }
  else
  {
    b_issubscribed_ = false;
  }
  j_release(&j_obj);
}

std::string EventNotification::createEventObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  std::string strreply;

  // return value
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
              jboolean_create(b_issubscribed_));

  // event type
  std::string event = getEventNotificationString(etype_);
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EVENT), jstring_create(event.c_str()));

  if (cstr_format == event)
  {
    if (nullptr != pdata_)
    {
      CAMERA_FORMAT *format = static_cast<CAMERA_FORMAT *>(pdata_);
      jvalue_ref json_outobjparams = jobject_create();

      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WIDTH),
                  jnumber_create_i32(format->nWidth));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HEIGHT),
                  jnumber_create_i32(format->nHeight));

      std::string strformat = getFormatStringFromCode(format->eFormat);
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT),
                  jstring_create(strformat.c_str()));

      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PARAMS), json_outobjparams);
    }
  }
  else if (cstr_properties == event)
  {
    if (nullptr != pdata_)
    {
      CAMERA_PROPERTIES_T *properties = static_cast<CAMERA_PROPERTIES_T *>(pdata_);
      jvalue_ref json_outobjparams = jobject_create();

      if (properties->nZoom != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM),
                    jnumber_create_i32(properties->nZoom));
      if (properties->nGridZoomX != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_COL),
                    jnumber_create_i32(properties->nGridZoomX));
      if (properties->nGridZoomY != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ROW),
                    jnumber_create_i32(properties->nGridZoomY));
      if (properties->nPan != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PAN),
                    jnumber_create_i32(properties->nPan));
      if (properties->nTilt != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TILT),
                    jnumber_create_i32(properties->nTilt));
      if (properties->nContrast != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST),
                    jnumber_create_i32(properties->nContrast));
      if (properties->nBrightness != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS),
                    jnumber_create_i32(properties->nBrightness));
      if (properties->nSaturation != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SATURATION),
                    jnumber_create_i32(properties->nSaturation));
      if (properties->nSharpness != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHARPNESS),
                    jnumber_create_i32(properties->nSharpness));
      if (properties->nHue != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HUE),
                    jnumber_create_i32(properties->nHue));
      if (properties->nWhiteBalanceTemperature != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE),
                    jnumber_create_i32(properties->nWhiteBalanceTemperature));
      if (properties->nGain != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAIN),
                    jnumber_create_i32(properties->nGain));
      if (properties->nGamma != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAMMA),
                    jnumber_create_i32(properties->nGamma));
      if (properties->nFrequency != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FREQUENCY),
                    jnumber_create_i32(properties->nFrequency));
      if (properties->bMirror != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIRROR),
                    jboolean_create(properties->bMirror));
      if (properties->nExposure != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE),
                    jnumber_create_i32(properties->nExposure));
      if (properties->bAutoExposure != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE),
                    jboolean_create(properties->bAutoExposure));
      if (properties->bAutoWhiteBalance != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE),
                    jboolean_create(properties->bAutoWhiteBalance));
      if (properties->nBitrate != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BITRATE),
                    jnumber_create_i32(properties->nBitrate));
      if (properties->nFramerate != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FRAMERATE),
                    jnumber_create_i32(properties->nFramerate));
      if (properties->ngopLength != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GOPLENGTH),
                    jnumber_create_i32(properties->ngopLength));
      if (properties->bLed != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_LED),
                    jboolean_create(properties->bLed));
      if (properties->bYuvMode != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_YUVMODE),
                    jboolean_create(properties->bYuvMode));
      if (properties->nIllumination != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ILLUMINATION),
                    jnumber_create_i32(properties->nIllumination));
      if (properties->bBacklightCompensation != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION),
                    jboolean_create(properties->bBacklightCompensation));
      if (properties->nMicMaxGain != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMAXGAIN),
                    jnumber_create_i32(properties->nMicMaxGain));
      if (properties->nMicMinGain != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMINGAIN),
                    jnumber_create_i32(properties->nMicMinGain));
      if (properties->nMicGain != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICGAIN),
                    jnumber_create_i32(properties->nMicGain));
      if (properties->bMicMute != CONST_VARIABLE_INITIALIZE)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMUTE),
                    jboolean_create(properties->bMicMute));

      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PARAMS), json_outobjparams);
    }
  }

  strreply = jvalue_stringify(json_outobj);
  PMLOG_INFO(CONST_MODULE_LUNA, "createEventObjectJsonString strreply %s\n", strreply.c_str());
  j_release(&json_outobj);

  return strreply;
}
