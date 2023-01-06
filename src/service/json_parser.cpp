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

#include "json_parser.h"
#include "constants.h"
#include <iostream>
#include <signal.h>
#include "whitelist_checker.h"

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

  const char* strvalue = jvalue_stringify(json_outobj);
  str_reply = (strvalue) ? strvalue : "";
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

    int n_client_pid = n_invalid_pid;
    jvalue_ref jnum = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_CLIENT_PROCESS_ID));
    jnumber_get_i32(jnum, &n_client_pid);
    if (n_client_pid > 0)
    {
      setClientProcessId(n_client_pid);

      int n_client_sig = n_invalid_sig;
      jnum = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_CLIENT_SIGNAL_NUM));
      jnumber_get_i32(jnum, &n_client_sig);
      if ((SIGHUP <= n_client_sig && n_client_sig <= SIGSYS) &&
	      (n_client_sig != SIGKILL && n_client_sig != SIGSTOP))

      {
        setClientSignal(n_client_sig);
      }
      else
      {
        setClientSignal(n_invalid_sig);
      }
    }
    else
    {
      setClientProcessId(n_invalid_pid);
    }
  }
  else
  {
    setCameraId(cstr_invaliddeviceid);
    setClientProcessId(n_invalid_pid);
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

    int n_client_pid = getClientProcessId();
    if (n_client_pid > 0)
    {
      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_CLIENT_PROCESS_ID),
                  jnumber_create_i32(n_client_pid));

      int n_client_sig = getClientSignal();
      if (n_client_sig != -1)
      {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_CLIENT_SIGNAL_NUM),
                    jnumber_create_i32(n_client_sig));
      }
    }
  }
  else
  {
    createJsonStringFailure(obj_reply, json_outobj);
  }

  const char* strvalue = jvalue_stringify(json_outobj);
  str_reply = (strvalue) ? strvalue : "";
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
    r_cams_source.str_memorysource = (source.m_str) ? source.m_str : "";
    r_cams_source.str_memorytype = (type.m_str) ? type.m_str : "";
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

  const char* strvalue = jvalue_stringify(json_outobj);
  str_reply = (strvalue) ? strvalue : "";
  j_release(&json_outobj);

  return str_reply;
}

void StopPreviewCaptureCloseMethod::getObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retVal = deSerialize(input, schemapath, j_obj);
  int n_client_pid = n_invalid_pid;

  if (0 == retVal)
  {
    int n_devicehandle = n_invalid_id;
    jnumber_get_i32(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE)), &n_devicehandle);
    setDeviceHandle(n_devicehandle);

    jnumber_get_i32(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_CLIENT_PROCESS_ID)), &n_client_pid);
    setClientProcessId(n_client_pid);
  }
  else
  {
    setDeviceHandle(n_invalid_id);
    setClientProcessId(n_client_pid);
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

    int n_client_id = getClientProcessId();
    if (n_client_id > 0)
    {
      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_CLIENT_PROCESS_ID),
                  jnumber_create_i32(n_client_id));
    }
  }
  else
  {
    createJsonStringFailure(objreply, json_outobj);
  }

  const char* strvalue = jvalue_stringify(json_outobj);
  str_reply = (strvalue) ? strvalue : "";
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
    int nWidth=0, nHeight=0;

    jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
    jvalue_ref j_params = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WIDTH));
    jnumber_get_i32(j_params, &nWidth);
    rcamera_params.nWidth = nWidth;
    j_params = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HEIGHT));
    jnumber_get_i32(j_params, &nHeight);
    rcamera_params.nHeight = nHeight;

    raw_buffer strformat =
        jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FORMAT)));
    std::string format = strformat.m_str;

    camera_format_t nformat;
    convertFormatToCode(format, &nformat);
    rcamera_params.eFormat = nformat;

    setCameraParams(rcamera_params);

    raw_buffer str_mode =
        jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MODE)));
    str_mode_ = (str_mode.m_str) ? str_mode.m_str : "";

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
    str_path_ = (strpath.m_str) ? strpath.m_str : "";
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

  const char* strvalue = jvalue_stringify(json_outobj);
  str_reply = (strvalue) ? strvalue : "";
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
    bool supported = WhitelistChecker::getInstance().isSupportedCamera(rGetCameraInfo().str_vendorid, rGetCameraInfo().str_productid);

    getFormatString(rGetCameraInfo().n_format, strformat);
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(objreply.bGetReturnValue()));

    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_NAME),
                jstring_create(rGetCameraInfo().str_devicename));
    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TYPE),
                jstring_create(getTypeString(rGetCameraInfo().n_devicetype)));
    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BUILTIN),
                jboolean_create(rGetCameraInfo().b_builtin));
    jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SUPPORTED),
                jboolean_create(supported));

    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXWIDTH),
                jnumber_create_i32(rGetCameraInfo().n_maxvideowidth));
    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXHEIGHT),
                jnumber_create_i32(rGetCameraInfo().n_maxvideoheight));
    jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FRAMERATE),
                jnumber_create_i32(rGetCameraInfo().n_cur_fps));
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

  const char* strvalue = jvalue_stringify(json_outobj);
  strreply = (strvalue) ? strvalue : "";
  j_release(&json_outobj);

  return strreply;
}

GetSetPropertiesMethod::GetSetPropertiesMethod()
    : n_devicehandle_(n_invalid_id), ro_camproperties_(), str_params_()
{
}

bool GetSetPropertiesMethod::isParamsEmpty(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  bool paramEmpty = false;
  int retval = deSerialize(input, schemapath, j_obj);
  if (0 == retval)
  {
     jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
     if (jobject_size(jobj_params) == 0)
         paramEmpty = true;
  }
  else
  {
    setDeviceHandle(n_invalid_id);
  }
  j_release(&j_obj);
  return paramEmpty;
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

      createGetPropertiesJsonString(&obj, &default_obj, json_outobjparams);

      // add resolution structure
      jvalue_ref json_resolutionobj = jobject_create();
      for (int nformat = 0; nformat < rGetCameraProperties().stResolution.n_formatindex; nformat++)
      {
        jvalue_ref json_resolutionarray = jarray_create(0);
        for (int count = 0; count < rGetCameraProperties().stResolution.n_framecount[nformat];
             count++)
        {
          jarray_append(json_resolutionarray,
                        jstring_create(rGetCameraProperties().stResolution.c_res[nformat][count]));
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
      for (size_t nelementcount = 0; nelementcount < str_params_.size(); nelementcount++)
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

  const char* strvalue = jvalue_stringify(json_outobj);
  strreply = (strvalue) ? strvalue : "";
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

  const char* strvalue = jvalue_stringify(json_outbj);
  strreply = (strvalue) ? strvalue : "";
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
    int nWidth=0, nHeight=0;

    jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
    jvalue_ref jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WIDTH));
    jnumber_get_i32(jparams, &nWidth);
    rcameraparams.nWidth = nWidth;
    jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HEIGHT));
    jnumber_get_i32(jparams, &nHeight);
    rcameraparams.nHeight = nHeight;
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

  const char* strvalue = jvalue_stringify(json_outobj);
  strreply = (strvalue) ? strvalue : "";
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

void createGetPropertiesJsonString(CAMERA_PROPERTIES_T *properties, CAMERA_PROPERTIES_T *old_property, jvalue_ref &json_outobjparams)
{
  if (nullptr != old_property)
  {
    if (properties->nContrast != old_property->nContrast)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nContrast));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST), json_propertyobj);
    } else if(properties->nContrast == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nBrightness != old_property->nBrightness)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nBrightness));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS), json_propertyobj);
    } else if(properties->nBrightness == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nAutoFocus != old_property->nAutoFocus)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOFOCUS][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOFOCUS][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOFOCUS][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOFOCUS][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nAutoFocus));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOFOCUS), json_propertyobj);
    } else if(properties->nAutoFocus == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOFOCUS), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nFocusAbsolute != old_property->nFocusAbsolute)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_FOCUSABSOLUTE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_FOCUSABSOLUTE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_FOCUSABSOLUTE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_FOCUSABSOLUTE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nFocusAbsolute));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FOCUS_ABSOLUTE), json_propertyobj);
    } else if(properties->nFocusAbsolute == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FOCUS_ABSOLUTE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nZoomAbsolute != old_property->nZoomAbsolute)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_ZOOMABSOLUTE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_ZOOMABSOLUTE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_ZOOMABSOLUTE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_ZOOMABSOLUTE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nZoomAbsolute));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM_ABSOLUTE), json_propertyobj);
    } else if(properties->nZoomAbsolute == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM_ABSOLUTE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }

    if (properties->nPan != old_property->nPan)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_PAN][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_PAN][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_PAN][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_PAN][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nPan));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PAN), json_propertyobj);
    } else if(properties->nPan == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PAN), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nTilt != old_property->nTilt)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_TILT][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_TILT][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_TILT][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_TILT][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nTilt));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TILT), json_propertyobj);
    } else if(properties->nTilt == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TILT), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nContrast != old_property->nContrast)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nContrast));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST), json_propertyobj);
    } else if(properties->nContrast == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nBrightness != old_property->nBrightness)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nBrightness));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS), json_propertyobj);
    } else if(properties->nBrightness == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nSaturation != old_property->nSaturation)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_SATURATION][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_SATURATION][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_SATURATION][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_SATURATION][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nSaturation));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SATURATION), json_propertyobj);
    } else if(properties->nSaturation == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SATURATION), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }

    if (properties->nSharpness != old_property->nSharpness)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_SHARPNESS][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_SHARPNESS][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_SHARPNESS][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_SHARPNESS][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nSharpness));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHARPNESS), json_propertyobj);
    } else if(properties->nSharpness == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHARPNESS), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nHue != old_property->nHue)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_HUE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_HUE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_HUE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_HUE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nHue));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HUE), json_propertyobj);
    } else if(properties->nHue == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HUE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nWhiteBalanceTemperature != old_property->nWhiteBalanceTemperature)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_WHITEBALANCETEMPERATURE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_WHITEBALANCETEMPERATURE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_WHITEBALANCETEMPERATURE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_WHITEBALANCETEMPERATURE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nWhiteBalanceTemperature));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE), json_propertyobj);
    } else if(properties->nWhiteBalanceTemperature == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nGain != old_property->nGain)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAIN][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAIN][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_GAIN][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAIN][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nGain));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAIN), json_propertyobj);
    } else if(properties->nGain == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAIN), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nGamma != old_property->nGamma)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAMMA][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAMMA][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_GAMMA][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAMMA][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nGamma));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAMMA), json_propertyobj);
    } else if(properties->nGamma == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAMMA), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nFrequency != old_property->nFrequency)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_FREQUENCY][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_FREQUENCY][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_FREQUENCY][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_FREQUENCY][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nFrequency));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FREQUENCY), json_propertyobj);
    } else if(properties->nFrequency == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FREQUENCY), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nExposure != old_property->nExposure)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_EXPOSURE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_EXPOSURE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_EXPOSURE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_EXPOSURE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nExposure));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE), json_propertyobj);
    } else if(properties->nExposure == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nAutoExposure != old_property->nAutoExposure)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOEXPOSURE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOEXPOSURE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOEXPOSURE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOEXPOSURE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nAutoExposure));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE), json_propertyobj);
    } else if(properties->nAutoExposure == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nAutoWhiteBalance != old_property->nAutoWhiteBalance)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nAutoWhiteBalance));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE), json_propertyobj);
    } else if(properties->nAutoWhiteBalance == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    if (properties->nBacklightCompensation != old_property->nBacklightCompensation)
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_BACKLIGHTCOMPENSATION][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_BACKLIGHTCOMPENSATION][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_BACKLIGHTCOMPENSATION][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_BACKLIGHTCOMPENSATION][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nBacklightCompensation));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION), json_propertyobj);
    } else if(properties->nBacklightCompensation == CONST_PARAM_DEFAULT_VALUE) {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
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
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_ZOOM_ABSOLUTE,properties->nZoomAbsolute));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_FOCUS_ABSOLUTE,properties->nFocusAbsolute));
  gPropertyMap.insert(std::make_pair(CONST_PARAM_NAME_AUTOFOCUS,properties->nAutoFocus));

}

void createGetPropertiesOutputJsonString(const std::string propertyName,
                                                   CAMERA_PROPERTIES_T *properties,  jvalue_ref &json_outobjparams)
{

  if (propertyName == CONST_PARAM_NAME_AUTOFOCUS)
  {
    if (properties->nAutoFocus == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOFOCUS), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOFOCUS][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOFOCUS][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOFOCUS][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOFOCUS][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nAutoFocus));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOFOCUS), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_FOCUS_ABSOLUTE)
  {
    if (properties->nFocusAbsolute == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FOCUS_ABSOLUTE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_FOCUSABSOLUTE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_FOCUSABSOLUTE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_FOCUSABSOLUTE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_FOCUSABSOLUTE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nFocusAbsolute));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FOCUS_ABSOLUTE), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_ZOOM_ABSOLUTE)
  {
    if (properties->nZoomAbsolute == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM_ABSOLUTE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_ZOOMABSOLUTE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_ZOOMABSOLUTE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_ZOOMABSOLUTE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_ZOOMABSOLUTE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nZoomAbsolute));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM_ABSOLUTE), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_PAN)
  {
    if (properties->nPan == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PAN), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_PAN][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_PAN][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_PAN][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_PAN][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nPan));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PAN), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_TILT)
  {
    if (properties->nTilt == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TILT), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_TILT][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_TILT][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_TILT][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_TILT][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nTilt));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TILT), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_CONTRAST)
  {
    if (properties->nContrast == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_CONTRAST][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nContrast));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_BIRGHTNESS)
  {
    if (properties->nBrightness == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_BRIGHTNESS][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nBrightness));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_SATURATION)
  {
    if (properties->nSaturation == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SATURATION), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_SATURATION][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_SATURATION][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_SATURATION][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_SATURATION][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nSaturation));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SATURATION), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_SHARPNESS)
  {
    if (properties->nSharpness == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHARPNESS), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_SHARPNESS][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_SHARPNESS][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_SHARPNESS][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_SHARPNESS][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nSharpness));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHARPNESS), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_HUE)
  {
    if (properties->nHue == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HUE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_HUE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_HUE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_HUE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_HUE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nHue));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HUE), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_WHITEBALANCETEMPERATURE)
  {
    if (properties->nWhiteBalanceTemperature == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_WHITEBALANCETEMPERATURE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_WHITEBALANCETEMPERATURE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_WHITEBALANCETEMPERATURE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_WHITEBALANCETEMPERATURE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nWhiteBalanceTemperature));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_GAIN)
  {
    if (properties->nGain == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAIN), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAIN][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAIN][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_GAIN][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAIN][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nGain));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAIN), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_GAMMA)
  {
    if (properties->nGamma == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAMMA), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAMMA][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAMMA][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_GAMMA][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_GAMMA][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nGamma));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAMMA), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_FREQUENCY)
  {
    if (properties->nFrequency == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FREQUENCY), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_FREQUENCY][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_FREQUENCY][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_FREQUENCY][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_FREQUENCY][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nFrequency));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FREQUENCY), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_EXPOSURE)
  {
    if (properties->nExposure == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_EXPOSURE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_EXPOSURE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_EXPOSURE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_EXPOSURE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nExposure));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_AUTOEXPOSURE)
  {
    if (properties->nAutoExposure == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOEXPOSURE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOEXPOSURE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOEXPOSURE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOEXPOSURE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nAutoExposure));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_AUTOWHITEBALANCE)
  {
    if (properties->nAutoWhiteBalance == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_AUTOWHITEBALANCE][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nAutoWhiteBalance));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE), json_propertyobj);
    }
  }
  if (propertyName == CONST_PARAM_NAME_BACKLIGHT_COMPENSATION)
  {
    if (properties->nBacklightCompensation == CONST_PARAM_DEFAULT_VALUE)
    {
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION), jstring_create(CONST_PARAM_NAME_NOTSUPPORT));
    }
    else
    {
      jvalue_ref json_propertyobj = jobject_create();
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN), jnumber_create_i32(properties->stGetData.data[PROPERTY_BACKLIGHTCOMPENSATION][QUERY_MIN]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX), jnumber_create_i32(properties->stGetData.data[PROPERTY_BACKLIGHTCOMPENSATION][QUERY_MAX]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),jnumber_create_i32(properties->stGetData.data[PROPERTY_BACKLIGHTCOMPENSATION][QUERY_STEP]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE), jnumber_create_i32(properties->stGetData.data[PROPERTY_BACKLIGHTCOMPENSATION][QUERY_DEFAULT]));
      jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE), jnumber_create_i32(properties->nBacklightCompensation));
      jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION), json_propertyobj);
     }
  }
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
     createGetPropertiesOutputJsonString(strparam, properties, json_outobjparams);
  }
  else if (CONST_PARAM_NAME_RESOLUTION == strparam)
  {
    jvalue_ref json_resolutionobj = jobject_create();
    for (int nformat = 0; nformat < properties->stResolution.n_formatindex; nformat++)
    {
      jvalue_ref json_resolutionarray = jarray_create(0);
      for (int count = 0; count < properties->stResolution.n_framecount[nformat]; count++)
      {
        jarray_append(json_resolutionarray, jstring_create(properties->stResolution.c_res[nformat][count]));
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

  const char* strvalue = jvalue_stringify(json_outobj);
  str_reply = (strvalue) ? strvalue : "";
  j_release(&json_outobj);

  return str_reply;
}

void GetSolutionsMethod::getObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  int retval = deSerialize(input, schemapath, j_obj);

  if (0 == retval)
  {
    int devicehandle       = n_invalid_id;
    jvalue_ref j_param_obj = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE));
    jnumber_get_i32(j_param_obj, &devicehandle);
    if (devicehandle == 0)
    {
      devicehandle = n_invalid_id;
    }
    setDeviceHandle(devicehandle);
    j_param_obj       = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_ID));
    raw_buffer str_id = jstring_get_fast(j_param_obj);
    if (strstr(str_id.m_str, "camera") == NULL)
    {
      setCameraId(cstr_invaliddeviceid);
    }
    else
    {
      setCameraId(str_id.m_str);
    }
  }
  else
  {
    setDeviceHandle(n_invalid_id);
  }
  j_release(&j_obj);
}

std::string
GetSolutionsMethod::createObjectJsonString(std::vector<std::string> supportedSolutionList,
                                           std::vector<std::string> enabledSolutionList) const
{
  jvalue_ref json_outobj                    = jobject_create();
  jvalue_ref json_supported_solutions_array = jarray_create(0);

  std::string str_reply;

  MethodReply obj_reply = getMethodReply();

  if (obj_reply.bGetReturnValue())
  {
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                jboolean_create(obj_reply.bGetReturnValue()));

    if (supportedSolutionList.size() > 0)
    {
      for (auto supportedSolution : supportedSolutionList)
      {
        bool isEnabled               = false;
        jvalue_ref json_solution_obj = jobject_create();

        jobject_put(json_solution_obj, J_CSTR_TO_JVAL("name"),
                    jstring_create(supportedSolution.c_str()));

        for (auto enabledSolution : enabledSolutionList)
        {
          if (enabledSolution == supportedSolution)
          {
            isEnabled = true;
            break;
          }
        }

        jvalue_ref json_paramsobj = jobject_create();
        jobject_put(json_paramsobj, J_CSTR_TO_JVAL("enable"), jboolean_create(isEnabled));
        /* To do: TBD */
        /*
        jobject_put(json_paramsobj, J_CSTR_TO_JVAL("Key_1"),
        jstring_create(string_value));
        jobject_put(json_paramsobj, J_CSTR_TO_JVAL("Key_2"),
        jnumber_create_i32(num_value));
        */
        jobject_put(json_solution_obj, J_CSTR_TO_JVAL("params"), json_paramsobj);

        jarray_append(json_supported_solutions_array, json_solution_obj);
      }

      jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SOLUTION),
                  json_supported_solutions_array);
    }
  }
  else
  {
    createJsonStringFailure(obj_reply, json_outobj);
  }

  str_reply = jvalue_stringify(json_outobj);
  j_release(&json_outobj);

  return str_reply;
}

SetSolutionsMethod::SetSolutionsMethod()
    : n_devicehandle_(n_invalid_id), str_enable_solutions_(), str_disable_solutions_()
{
}

bool SetSolutionsMethod::isEmpty()
{
  bool isEmpty = false;
  if (0 == str_enable_solutions_.size() && 0 == str_disable_solutions_.size())
  {
    isEmpty = true;
  }
  return isEmpty;
}

void SetSolutionsMethod::getObject(const char *input, const char *schemapath)
{
  jvalue_ref j_obj;
  jvalue_ref j_solutions_obj;
  int retval = deSerialize(input, schemapath, j_obj);

  if (0 == retval)
  {
    int devicehandle       = n_invalid_id;
    jvalue_ref j_param_obj = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE));
    jnumber_get_i32(j_param_obj, &devicehandle);
    if (devicehandle == 0)
    {
      devicehandle = n_invalid_id;
    }
    setDeviceHandle(devicehandle);

    j_param_obj       = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_ID));
    raw_buffer str_id = jstring_get_fast(j_param_obj);
    if (strstr(str_id.m_str, "camera") == NULL)
    {
      setCameraId(cstr_invaliddeviceid);
    }
    else
    {
      setCameraId(str_id.m_str);
    }

    j_solutions_obj = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_SOLUTION));

    for (ssize_t i = 0; i != jarray_size(j_solutions_obj); i++)
    {
      raw_buffer strid = jstring_get_fast(
          jobject_get(jarray_get(j_solutions_obj, i), J_CSTR_TO_BUF("name")));

      bool enable = false;
      jvalue_ref j_param_obj =
          jobject_get(jarray_get(j_solutions_obj, i), J_CSTR_TO_BUF("params"));
      jboolean_get(jobject_get(j_param_obj, J_CSTR_TO_BUF("enable")), &enable);
      /* To do: TBD */
      /*
      jboolean_get(jobject_get(j_param_obj, J_CSTR_TO_BUF("Key1")),
      &enable);
      jboolean_get(jobject_get(j_param_obj, J_CSTR_TO_BUF("Key2")),
      &enable);
      */

      if (true == enable)
      {
        setEnableSolutionList(strid.m_str);
      }
      else
      {
        setDisbleSolutionList(strid.m_str);
      }
    }
  }
  else
  {
    setDeviceHandle(n_invalid_id);
  }
  j_release(&j_obj);
}

std::string SetSolutionsMethod::createObjectJsonString() const
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
