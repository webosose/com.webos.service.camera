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

    raw_buffer str_mode =
        jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MODE)));
    str_mode_ = str_mode.m_str;

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
                jboolean_create(rGetCameraInfo().b_builtin));

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
    : n_devicehandle_(n_invalid_id), ro_camproperties_(), str_params_()
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

    jvalue_ref params = jobject_get(j_obj, J_CSTR_TO_BUF("params"));
    for (ssize_t i = 0; i != jarray_size(params); i++)
    {
      raw_buffer strid = jstring_get_fast(jarray_get(params, i));
      setParams(strid.m_str);
    }
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

    if (0 == str_params_.size())
    {
      CAMERA_PROPERTIES_T obj = rGetCameraProperties();
      CAMERA_PROPERTIES_T default_obj;
      void *data = (void *)&default_obj;
      createGetPropertiesJsonString(&obj, data, json_outobjparams);
      // add resolution structure
      jvalue_ref json_resolutionobj = jobject_create();
      for (int nformat = 0; nformat < rGetCameraProperties().stResolution.n_formatindex; nformat++)
      {
        jvalue_ref json_resolutionarray = jarray_create(0);
        for (int count = 0; count < rGetCameraProperties().stResolution.n_frameindex[nformat];
             count++)
        {
          jarray_append(json_resolutionarray,
                        jstring_create(rGetCameraProperties().stResolution.c_res[count]));
        }
        jobject_put(json_resolutionobj,
                    jstring_create(
                        getResolutionString(rGetCameraProperties().stResolution.e_format[nformat])
                            .c_str()),
                    json_resolutionarray);
      }
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RESOLUTION),
                  json_resolutionobj);
    }
    else
    {
      CAMERA_PROPERTIES_T obj = rGetCameraProperties();
      for (int nelementcount = 0; nelementcount < str_params_.size(); nelementcount++)
      {
        createGetPropertiesOutputParamJsonString(str_params_.at(nelementcount), &obj,
                                                 json_outobjparams);
      }
    }
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
    jvalue_ref jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_PAN));
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
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_EXPOSURE));
    jnumber_get_i32(jparams, &r_camproperties.nExposure);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_AUTOEXPOSURE));
    jnumber_get_i32(jparams, &r_camproperties.nAutoExposure);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_AUTOWHITEBALANCE));
    jnumber_get_i32(jparams, &r_camproperties.nAutoWhiteBalance);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION));
    jnumber_get_i32(jparams, &r_camproperties.nBacklightCompensation);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_ZOOM_ABSOLUTE));
    jnumber_get_i32(jparams, &r_camproperties.nZoomAbsolute);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_AUTOFOCUS));
    jnumber_get_i32(jparams, &r_camproperties.nAutoFocus);
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FOCUS_ABSOLUTE));
    jnumber_get_i32(jparams, &r_camproperties.nFocusAbsolute);
    r_camproperties.stResolution.n_formatindex = 0;
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

void EventNotification::outputObjectFormat(CAMERA_FORMAT *pFormat, jvalue_ref &json_outobj)const
{
  jvalue_ref json_outobjformat = jobject_create();

  jobject_put(json_outobjformat, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WIDTH),
                    jnumber_create_i32(pFormat->nWidth));

  jobject_put(json_outobjformat, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HEIGHT),
                    jnumber_create_i32(pFormat->nHeight));

  jobject_put(json_outobjformat, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FPS),
                    jnumber_create_i32(pFormat->nFps));

  std::string strformat = getFormatStringFromCode(pFormat->eFormat);
  jobject_put(json_outobjformat, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT),
                    jstring_create(strformat.c_str()));

  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMATINFO), json_outobjformat);
}

void EventNotification::outputObjectProperties(CAMERA_PROPERTIES_T *pProperties, jvalue_ref &json_outobj)const
{
  jvalue_ref json_outobjproperties = jobject_create();

  std::map<std::string,int> gPropertyMap;

  mappingPropertieswithConstValues(gPropertyMap,pProperties);

  std::map<std::string,int>::iterator it;

  std::string sProperty = "";
  for (it = gPropertyMap.begin(); it != gPropertyMap.end(); ++it)
  {
    sProperty = it->first;
    jobject_put(json_outobjproperties, J_CSTR_TO_JVAL(sProperty.c_str()),
                jnumber_create_i32(it->second));
  }

  jvalue_ref json_resolutionobj = jobject_create();
  for (int nformat = 0; nformat < pProperties->stResolution.n_formatindex; nformat++)
  {
    jvalue_ref json_resolutionarray = jarray_create(0);
    for (int count = 0; count < pProperties->stResolution.n_frameindex[nformat]; count++)
    {
      jarray_append(json_resolutionarray,
                    jstring_create(pProperties->stResolution.c_res[count]));
    }
    jobject_put(
        json_resolutionobj,
        jstring_create(
            getResolutionString(pProperties->stResolution.e_format[nformat]).c_str()),
        json_resolutionarray);
  }
  jobject_put(json_outobjproperties, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RESOLUTION),
              json_resolutionobj);
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PROPERTIESINFO),
              json_outobjproperties);
}

std::string
EventNotification::createEventObjectSubscriptionJsonString(CAMERA_FORMAT *pFormat,
                                                           CAMERA_PROPERTIES_T *pProperties) const
{
  jvalue_ref json_outobj = jobject_create();
  std::string strreply;

  // return value
  jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
              jboolean_create(b_issubscribed_));

  if (b_issubscribed_)
  {
    if (!strcamid_.empty())
    {
      // camera id
      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ID),
                  jstring_create(strcamid_.c_str()));

      if (nullptr != pFormat)
        outputObjectFormat(pFormat, json_outobj);

      if (nullptr != pProperties)
        outputObjectProperties(pProperties, json_outobj);
    }
  }
  strreply = jvalue_stringify(json_outobj);
  PMLOG_INFO(CONST_MODULE_LUNA, "createEventObjectSubscriptionJsonString strreply %s\n",
             strreply.c_str());
  j_release(&json_outobj);

  return strreply;
}

void createGetPropertiesJsonString(CAMERA_PROPERTIES_T *properties, void *pdata,
                                   jvalue_ref &json_outobjparams)
{
  CAMERA_PROPERTIES_T *old_property = static_cast<CAMERA_PROPERTIES_T *>(pdata);

  if (nullptr != old_property)
  {
    if (properties->nAutoFocus != old_property->nAutoFocus)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOFOCUS),
                  jnumber_create_i32(properties->nAutoFocus));
    if (properties->nFocusAbsolute != old_property->nFocusAbsolute)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FOCUS_ABSOLUTE),
                  jnumber_create_i32(properties->nFocusAbsolute));
    if (properties->nZoomAbsolute != old_property->nZoomAbsolute)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM_ABSOLUTE),
                  jnumber_create_i32(properties->nZoomAbsolute));
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
    if (properties->nExposure != old_property->nExposure)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE),
                  jnumber_create_i32(properties->nExposure));
    if (properties->nAutoExposure != old_property->nAutoExposure)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE),
                  jnumber_create_i32(properties->nAutoExposure));
    if (properties->nAutoWhiteBalance != old_property->nAutoWhiteBalance)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE),
                  jnumber_create_i32(properties->nAutoWhiteBalance));
    if (properties->nBacklightCompensation != old_property->nBacklightCompensation)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION),
                  jnumber_create_i32(properties->nBacklightCompensation));
  }
  return;
}

void mappingPropertieswithConstValues(std::map<std::string,int> &gPropertyMap, CAMERA_PROPERTIES_T *properties)
{
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_PAN,properties->nPan));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_TILT,properties->nTilt));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_CONTRAST,properties->nContrast));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_BIRGHTNESS,properties->nBrightness));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_SATURATION,properties->nSaturation));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_SHARPNESS,properties->nSharpness));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_HUE,properties->nHue));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE,properties->nWhiteBalanceTemperature));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_GAIN,properties->nGain));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_GAMMA,properties->nGamma));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_FREQUENCY,properties->nFrequency));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_EXPOSURE,properties->nExposure));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_AUTOEXPOSURE,properties->nAutoExposure));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_AUTOWHITEBALANCE,properties->nAutoWhiteBalance));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION,properties->nBacklightCompensation));
}

void createGetPropertiesOutputJsonString(const std::string propertyName,
                                   int propertyValue, jvalue_ref &json_outobjparams)
{
  if (propertyName == CONST_PARAM_NAME_AUTOFOCUS)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOFOCUS),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_FOCUS_ABSOLUTE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FOCUS_ABSOLUTE),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_ZOOM_ABSOLUTE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM_ABSOLUTE),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_PAN)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PAN),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_TILT)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TILT),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_CONTRAST)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_BIRGHTNESS)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_SATURATION)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SATURATION),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_SHARPNESS)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHARPNESS),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_HUE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HUE),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_WHITEBALANCETEMPERATURE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_GAIN)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAIN),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_GAMMA)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAMMA),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_FREQUENCY)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FREQUENCY),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_EXPOSURE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_AUTOEXPOSURE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_AUTOWHITEBALANCE)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE),
                  jnumber_create_i32(propertyValue));
  if (propertyName == CONST_PARAM_NAME_BACKLIGHT_COMPENSATION)
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION),
                  jnumber_create_i32(propertyValue));
}

void createGetPropertiesOutputParamJsonString(const std::string strparam,
                                              CAMERA_PROPERTIES_T *properties,
                                              jvalue_ref &json_outobjparams)
{
  std::map<std::string,int> gPropertyMap;

  mappingPropertieswithConstValues(gPropertyMap,properties);

  std::map<std::string,int>::iterator it = gPropertyMap.find(strparam);

  if (it != gPropertyMap.end())
  {
     createGetPropertiesOutputJsonString(strparam, it->second, json_outobjparams);
  }
  else if (CONST_PARAM_NAME_RESOLUTION == strparam)
  {
    jvalue_ref json_resolutionobj = jobject_create();
    for (int nformat = 0; nformat < properties->stResolution.n_formatindex; nformat++)
    {
      jvalue_ref json_resolutionarray = jarray_create(0);
      for (int count = 0; count < properties->stResolution.n_frameindex[nformat]; count++)
      {
        jarray_append(json_resolutionarray, jstring_create(properties->stResolution.c_res[count]));
      }
      jobject_put(
          json_resolutionobj,
          jstring_create(getResolutionString(properties->stResolution.e_format[nformat]).c_str()),
          json_resolutionarray);
    }
    jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RESOLUTION), json_resolutionobj);
  }
  else
  {
    // do nothing
  }

  return;
}

void GetFdMethod::getObject(const char *input, const char *schemapath)
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

std::string GetFdMethod::createObjectJsonString() const
{
  jvalue_ref json_outobj = jobject_create();
  std::string str_reply;

  MethodReply obj_reply = getMethodReply();

  if (obj_reply.bGetReturnValue())
  {
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(obj_reply.bGetReturnValue()));
  }
  else
  {
    createJsonStringFailure(obj_reply, json_outobj);
  }

  str_reply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return str_reply;
}
