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
    : n_devicehandle_(n_invalid_id), r_cameraparams_(), n_image_(0), str_path_(cstr_empty)
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

    camera_format_t nformat;
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
    // get path for images to be saved
    raw_buffer strpath =
        jstring_get_fast(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_IMAGE_PATH)));
    str_path_ = strpath.m_str;
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
    getFormatString(rGetCameraInfo().n_format, strformat);
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(objreply.bGetReturnValue()));

    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_NAME),
                jstring_create(rGetCameraInfo().str_devicename));
    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TYPE),
                jstring_create(getTypeString(rGetCameraInfo().n_devicetype)));
    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BUILTIN),
                jboolean_create((bool *)rGetCameraInfo().b_builtin));

    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXWIDTH),
                jnumber_create_i32(rGetCameraInfo().n_maxvideowidth));
    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXHEIGHT),
                jnumber_create_i32(rGetCameraInfo().n_maxvideoheight));
    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FRAMERATE), jnumber_create_i32(30));
    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT), jstring_create(strformat));

    jobject_put(json_pictureobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXWIDTH),
                jnumber_create_i32(rGetCameraInfo().n_maxpicturewidth));
    jobject_put(json_pictureobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXHEIGHT),
                jnumber_create_i32(rGetCameraInfo().n_maxpictureheight));
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

    CAMERA_PROPERTIES_T obj = rGetCameraProperties();
    CAMERA_PROPERTIES_T default_obj;
    void *data = (void *)&default_obj;
    createGetPropertiesJsonString(&obj, data, json_outobjparams);

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
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FPS));
    jnumber_get_i32(jparams, &rcameraparams.nFps);
    raw_buffer strformat =
        jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FORMAT)));
    std::string format = strformat.m_str;

    camera_format_t eformat;
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

std::string EventNotification::createEventObjectJsonString(void *pdata) const
{
  jvalue_ref json_outobj = jobject_create();
  std::string strreply;

  // return value
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
              jboolean_create(b_issubscribed_));

  // event type
  std::string event = getEventNotificationString(etype_);

  if (cstr_format == event)
  {
    if (nullptr != pdata_)
    {
      // camera id
      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ID),
                  jstring_create(strcamid_.c_str()));
      // eventType
      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EVENT),
                  jstring_create(event.c_str()));

      CAMERA_FORMAT *format = static_cast<CAMERA_FORMAT *>(pdata_);
      jvalue_ref json_outobjparams = jobject_create();

      CAMERA_FORMAT *old_format = static_cast<CAMERA_FORMAT *>(pdata);

      if (old_format->nWidth != format->nWidth)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WIDTH),
                    jnumber_create_i32(format->nWidth));
      if (old_format->nHeight != format->nHeight)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HEIGHT),
                    jnumber_create_i32(format->nHeight));
      if (old_format->nFps != format->nFps)
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FPS),
                    jnumber_create_i32(format->nFps));

      if (old_format->eFormat != format->eFormat)
      {
        std::string strformat = getFormatStringFromCode(format->eFormat);
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT),
                    jstring_create(strformat.c_str()));
      }

      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMATINFO), json_outobjparams);
    }
  }
  else if (cstr_properties == event)
  {
    if (nullptr != pdata_)
    {
      // camera id
      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ID),
                  jstring_create(strcamid_.c_str()));
      // eventType
      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EVENT),
                  jstring_create(event.c_str()));

      jvalue_ref json_outobjparams = jobject_create();
      CAMERA_PROPERTIES_T *properties = static_cast<CAMERA_PROPERTIES_T *>(pdata_);

      createGetPropertiesJsonString(properties, pdata, json_outobjparams);

      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PROPERTIESINFO), json_outobjparams);
    }
  }

  strreply = jvalue_stringify(json_outobj);
  PMLOG_INFO(CONST_MODULE_LUNA, "createEventObjectJsonString strreply %s\n", strreply.c_str());
  j_release(&json_outobj);

  return strreply;
}

void createGetPropertiesJsonString(CAMERA_PROPERTIES_T *properties, void *pdata,
                                   jvalue_ref &json_outobjparams)
{
  CAMERA_PROPERTIES_T *old_property = static_cast<CAMERA_PROPERTIES_T *>(pdata);

  if (nullptr != old_property)
  {
    if (properties->nZoom != old_property->nZoom)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM),
                  jnumber_create_i32(properties->nZoom));
    if (properties->nGridZoomX != old_property->nGridZoomX)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_COL),
                  jnumber_create_i32(properties->nGridZoomX));
    if (properties->nGridZoomY != old_property->nGridZoomY)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ROW),
                  jnumber_create_i32(properties->nGridZoomY));
    if (properties->nPan != old_property->nPan)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PAN),
                  jnumber_create_i32(properties->nPan));
    if (properties->nTilt != old_property->nTilt)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TILT),
                  jnumber_create_i32(properties->nTilt));
    if (properties->nContrast != old_property->nContrast)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST),
                  jnumber_create_i32(properties->nContrast));
    if (properties->nBrightness != old_property->nBrightness)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS),
                  jnumber_create_i32(properties->nBrightness));
    if (properties->nSaturation != old_property->nSaturation)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SATURATION),
                  jnumber_create_i32(properties->nSaturation));
    if (properties->nSharpness != old_property->nSharpness)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHARPNESS),
                  jnumber_create_i32(properties->nSharpness));
    if (properties->nHue != old_property->nHue)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HUE),
                  jnumber_create_i32(properties->nHue));
    if (properties->nWhiteBalanceTemperature != old_property->nWhiteBalanceTemperature)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE),
                  jnumber_create_i32(properties->nWhiteBalanceTemperature));
    if (properties->nGain != old_property->nGain)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAIN),
                  jnumber_create_i32(properties->nGain));
    if (properties->nGamma != old_property->nGamma)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAMMA),
                  jnumber_create_i32(properties->nGamma));
    if (properties->nFrequency != old_property->nFrequency)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FREQUENCY),
                  jnumber_create_i32(properties->nFrequency));
    if (properties->bMirror != old_property->bMirror)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIRROR),
                  jboolean_create(properties->bMirror));
    if (properties->nExposure != old_property->nExposure)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE),
                  jnumber_create_i32(properties->nExposure));
    if (properties->bAutoExposure != old_property->bAutoExposure)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE),
                  jboolean_create(properties->bAutoExposure));
    if (properties->bAutoWhiteBalance != old_property->bAutoWhiteBalance)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE),
                  jboolean_create(properties->bAutoWhiteBalance));
    if (properties->nBitrate != old_property->nBitrate)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BITRATE),
                  jnumber_create_i32(properties->nBitrate));
    if (properties->nFramerate != old_property->nFramerate)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FRAMERATE),
                  jnumber_create_i32(properties->nFramerate));
    if (properties->ngopLength != old_property->ngopLength)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GOPLENGTH),
                  jnumber_create_i32(properties->ngopLength));
    if (properties->bLed != old_property->bLed)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_LED),
                  jboolean_create(properties->bLed));
    if (properties->bYuvMode != old_property->bYuvMode)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_YUVMODE),
                  jboolean_create(properties->bYuvMode));
    if (properties->nIllumination != old_property->nIllumination)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ILLUMINATION),
                  jnumber_create_i32(properties->nIllumination));
    if (properties->bBacklightCompensation != old_property->bBacklightCompensation)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION),
                  jboolean_create(properties->bBacklightCompensation));
    if (properties->nMicMaxGain != old_property->nMicMaxGain)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMAXGAIN),
                  jnumber_create_i32(properties->nMicMaxGain));
    if (properties->nMicMinGain != old_property->nMicMinGain)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMINGAIN),
                  jnumber_create_i32(properties->nMicMinGain));
    if (properties->nMicGain != old_property->nMicGain)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICGAIN),
                  jnumber_create_i32(properties->nMicGain));
    if (properties->bMicMute != old_property->bMicMute)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMUTE),
                  jboolean_create(properties->bMicMute));
  }
  return;
}
