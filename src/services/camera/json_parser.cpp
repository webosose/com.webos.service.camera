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
#include "addon.h"
#include "camera_constants.h"
#include "json_utils.h"
#include <iostream>
#include <signal.h>

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
    jvalue_ref json_outobj             = jobject_create();
    jvalue_ref json_outdevicelistarray = jarray_create(NULL);

    std::string str_reply;

    MethodReply objreply = getMethodReply();

    if (objreply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                    jboolean_create(objreply.bGetReturnValue()));

        std::size_t count = getCameraCount();
        for (std::size_t i = 0; i < count; i++)
        {
            jvalue_ref json_outdevicelistitem = jobject_create();
            jobject_put(json_outdevicelistitem, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ID),
                        jstring_create(strGetCameraList(i).c_str()));
            jarray_append(json_outdevicelistarray, json_outdevicelistitem);
        }
        if (b_issubscribed_ == true)
        {
            jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SUBSCRIBED),
                        jboolean_create(b_issubscribed_));
        }
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEVICE_LIST),
                    json_outdevicelistarray);
    }
    else
    {
        createJsonStringFailure(objreply, json_outobj);
    }

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
    j_release(&json_outobj);

    return str_reply;
}

void OpenMethod::getOpenObject(const char *input, const char *schemapath)
{
    jvalue_ref j_obj;
    int retVal = deSerialize(input, schemapath, j_obj);

    if (0 == retVal)
    {
        raw_buffer str_appid =
            jstring_get_fast(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_APPID)));
        setAppId(str_appid.m_str);
        raw_buffer str_id =
            jstring_get_fast(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_ID)));
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

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
    j_release(&json_outobj);

    return str_reply;
}

void StartCameraMethod::getStartCameraObject(const char *input, const char *schemapath)
{
    jvalue_ref j_obj;
    int retval = deSerialize(input, schemapath, j_obj);

    if (0 == retval)
    {
        int n_devicehandle = n_invalid_id;
        jvalue_ref jnum    = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE));
        jnumber_get_i32(jnum, &n_devicehandle);
        setDeviceHandle(n_devicehandle);
    }
    else
    {
        setDeviceHandle(n_invalid_id);
    }
    j_release(&j_obj);
}

std::string StartCameraMethod::createStartCameraObjectJsonString() const
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

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
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
        jvalue_ref jnum    = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE));
        jnumber_get_i32(jnum, &n_devicehandle);
        setDeviceHandle(n_devicehandle);

        raw_buffer display =
            jstring_get_fast(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_WINDOW_ID)));

        camera_display_source_t r_dpy_source;
        if (display.m_str)
            r_dpy_source.str_window_id = display.m_str;
        setDpyParams(std::move(r_dpy_source));
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
    }
    else
    {
        createJsonStringFailure(obj_reply, json_outobj);
    }

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
    j_release(&json_outobj);

    return str_reply;
}

void StopCameraPreviewCaptureCloseMethod::getObject(const char *input, const char *schemapath)
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

std::string StopCameraPreviewCaptureCloseMethod::createObjectJsonString() const
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

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
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
        jvalue_ref jnum  = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE));
        jnumber_get_i32(jnum, &devicehandle);
        setDeviceHandle(devicehandle);

        CAMERA_FORMAT rcamera_params;
        int nWidth = 0, nHeight = 0;

        jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
        jvalue_ref j_params    = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WIDTH));
        jnumber_get_i32(j_params, &nWidth);
        rcamera_params.nWidth = (nWidth > 0) ? nWidth : 0;
        j_params              = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HEIGHT));
        jnumber_get_i32(j_params, &nHeight);
        rcamera_params.nHeight = (nHeight > 0) ? nHeight : 0;

        raw_buffer strformat =
            jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FORMAT)));
        std::string format = strformat.m_str;

        camera_format_t nformat;
        convertFormatToCode(std::move(format), &nformat);
        rcamera_params.eFormat = nformat;

        setCameraParams(rcamera_params);

        raw_buffer str_mode =
            jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MODE)));
        str_mode_ = str_mode.m_str == nullptr ? "" : str_mode.m_str;

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
        str_path_ = strpath.m_str ? strpath.m_str : "";
#ifdef DAC_ENABLED
        if (str_path_.empty())
        {
            str_path_ = cstr_capturedir;
        }
#endif
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

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
    j_release(&json_outobj);

    return str_reply;
}

CaptureMethod::CaptureMethod() : n_devicehandle_(n_invalid_id), n_image_(0), str_path_(cstr_empty)
{
}

void CaptureMethod::getCaptureObject(const char *input, const char *schemapath)
{
    jvalue_ref j_obj;
    int retval = deSerialize(input, schemapath, j_obj);

    if (0 == retval)
    {
        int devicehandle = n_invalid_id;
        jvalue_ref jnum  = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE));
        jnumber_get_i32(jnum, &devicehandle);
        setDeviceHandle(devicehandle);

        jvalue_ref j_nimages = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEFAULT_NIMAGE));
        jnumber_get_i32(j_nimages, &n_image_);

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

std::string
CaptureMethod::createCaptureObjectJsonString(std::vector<std::string> &capturedFiles) const
{
    jvalue_ref json_outobj = jobject_create();
    std::string str_reply;

    MethodReply objreply = getMethodReply();

    if (objreply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                    jboolean_create(objreply.bGetReturnValue()));

        if (capturedFiles.size() > 0)
        {
            jvalue_ref json_captured_files_array = jarray_create(0);
            for (const auto &capturedFile : capturedFiles)
            {
                jarray_append(json_captured_files_array, jstring_create(capturedFile.c_str()));
            }

            jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_IMAGE_PATH),
                        json_captured_files_array);
        }
    }
    else
    {
        createJsonStringFailure(objreply, json_outobj);
    }

    const char *strvalue = jvalue_stringify(json_outobj);
    str_reply            = (strvalue) ? strvalue : "";
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

std::string GetInfoMethod::createInfoObjectJsonString(bool supported) const
{
    jvalue_ref json_outobj   = jobject_create();
    jvalue_ref json_info_obj = jobject_create();
    std::string strreply;

    MethodReply objreply = getMethodReply();

    if (objreply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                    jboolean_create(objreply.bGetReturnValue()));

        jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_NAME),
                    jstring_create(rGetCameraInfo().str_devicename.c_str()));
        jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TYPE),
                    jstring_create(getTypeString(rGetCameraInfo().n_devicetype)));
        jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BUILTIN),
                    jboolean_create(rGetCameraInfo().b_builtin));
        jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SUPPORTED),
                    jboolean_create(supported));

        jvalue_ref json_resolutionobj = jobject_create();
        for (auto const &v : rGetCameraInfo().stResolution)
        {
            jvalue_ref json_resolutionarray = jarray_create(0);
            for (auto const &res : v.c_res)
            {
                jarray_append(json_resolutionarray, jstring_create(res.c_str()));
            }
            jobject_put(json_resolutionobj, jstring_create(getResolutionString(v.e_format).c_str()),
                        json_resolutionarray);
        }
        jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RESOLUTION), json_resolutionobj);
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_INFO), json_info_obj);
    }
    else
    {
        createJsonStringFailure(objreply, json_outobj);
    }

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        strreply = str;
    j_release(&json_outobj);

    return strreply;
}

GetSetPropertiesMethod::GetSetPropertiesMethod()
    : n_devicehandle_(n_invalid_id), ro_camproperties_(), str_params_(), str_devid_(cstr_empty),
      b_issubscribed_(false)
{
}

bool GetSetPropertiesMethod::isParamsEmpty(const char *input, const char *schemapath)
{
    jvalue_ref j_obj;
    bool paramEmpty = false;
    int retval      = deSerialize(input, schemapath, j_obj);
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
    jvalue_ref j_name_id_obj;
    int retval = deSerialize(input, schemapath, j_obj);

    if (0 == retval)
    {
        j_name_id_obj     = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_ID));
        raw_buffer str_id = jstring_get_fast(j_name_id_obj);

        // set camera id
        if (strstr(str_id.m_str, "camera") == NULL)
        {
            setCameraId(cstr_invaliddeviceid);
        }
        else
        {
            setCameraId(str_id.m_str);
        }

        // set params
        jvalue_ref params = jobject_get(j_obj, J_CSTR_TO_BUF("params"));
        for (ssize_t i = 0; i != jarray_size(params); i++)
        {
            raw_buffer strid = jstring_get_fast(jarray_get(params, i));
            setParams(strid.m_str);
        }
    }
    else
    {
        setCameraId(cstr_invaliddeviceid);
    }

    j_release(&j_obj);
}

std::string GetSetPropertiesMethod::createGetPropertiesObjectJsonString() const
{
    jvalue_ref json_outobj       = jobject_create();
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

            for (int i = 0; i < PROPERTY_END; i++)
            {
                if (obj.stGetData.data[i][QUERY_VALUE] != CONST_PARAM_DEFAULT_VALUE)
                {
                    jvalue_ref json_propertyobj = jobject_create();
                    jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN),
                                jnumber_create_i32(obj.stGetData.data[i][QUERY_MIN]));
                    jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX),
                                jnumber_create_i32(obj.stGetData.data[i][QUERY_MAX]));
                    jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),
                                jnumber_create_i32(obj.stGetData.data[i][QUERY_STEP]));
                    jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE),
                                jnumber_create_i32(obj.stGetData.data[i][QUERY_DEFAULT]));
                    jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE),
                                jnumber_create_i32(obj.stGetData.data[i][QUERY_VALUE]));
                    jobject_put(json_outobjparams, jstring_create(getParamString(i).c_str()),
                                json_propertyobj);
                }
            }
        }
        else
        {
            CAMERA_PROPERTIES_T obj = rGetCameraProperties();
            for (auto const &it : str_params_)
            {
                int param_enum = getParamNumFromString(it);
                if (param_enum >= 0)
                {
                    if (obj.stGetData.data[param_enum][QUERY_VALUE] != CONST_PARAM_DEFAULT_VALUE)
                    {
                        jvalue_ref json_propertyobj = jobject_create();
                        jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIN),
                                    jnumber_create_i32(obj.stGetData.data[param_enum][QUERY_MIN]));
                        jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAX),
                                    jnumber_create_i32(obj.stGetData.data[param_enum][QUERY_MAX]));
                        jobject_put(json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_STEP),
                                    jnumber_create_i32(obj.stGetData.data[param_enum][QUERY_STEP]));
                        jobject_put(
                            json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEFAULT_VALUE),
                            jnumber_create_i32(obj.stGetData.data[param_enum][QUERY_DEFAULT]));
                        jobject_put(
                            json_propertyobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VALUE),
                            jnumber_create_i32(obj.stGetData.data[param_enum][QUERY_VALUE]));
                        jobject_put(json_outobjparams, jstring_create((it).c_str()),
                                    json_propertyobj);
                    }
                }
            }
        }
        if (b_issubscribed_ == true)
        {
            jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SUBSCRIBED),
                        jboolean_create(b_issubscribed_));
        }
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PARAMS), json_outobjparams);
    }
    else
    {
        createJsonStringFailure(objreply, json_outobj);

        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PARAMS), json_outobjparams);

        if (b_issubscribed_ == true)
        {
            jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SUBSCRIBED),
                        jboolean_create(b_issubscribed_));
        }
    }

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        strreply = str;
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
        jvalue_ref jparams;
        for (int i = 0; i < PROPERTY_END; i++)
        {
            jparams = jobject_get(jobj_params, j_cstr_to_buffer(getParamString(i).c_str()));
            jnumber_get_i32(jparams, &r_camproperties.stGetData.data[i][QUERY_VALUE]);
        }

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

    const char *str = jvalue_stringify(json_outbj);
    if (str)
        strreply = str;
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
        int nWidth = 0, nHeight = 0;

        jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
        jvalue_ref jparams     = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WIDTH));
        jnumber_get_i32(jparams, &nWidth);
        rcameraparams.nWidth = (nWidth > 0) ? nWidth : 0;
        jparams              = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HEIGHT));
        jnumber_get_i32(jparams, &nHeight);
        rcameraparams.nHeight = (nHeight > 0) ? nHeight : 0;
        jparams               = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FPS));
        jnumber_get_i32(jparams, &rcameraparams.nFps);
        raw_buffer strformat =
            jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FORMAT)));
        std::string format = strformat.m_str;

        camera_format_t eformat;
        convertFormatToCode(std::move(format), &eformat);
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

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        strreply = str;
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

void GetFdMethod::getObject(const char *input, const char *schemapath)
{
    jvalue_ref j_obj;
    int retval = deSerialize(input, schemapath, j_obj);

    if (0 == retval)
    {
        int devicehandle = n_invalid_id;
        jnumber_get_i32(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE)), &devicehandle);
        setDeviceHandle(devicehandle);

        raw_buffer str_type =
            jstring_get_fast(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_TYPE)));
        setType(str_type.m_str);
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

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
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
GetSolutionsMethod::createObjectJsonString(std::vector<std::string> &supportedSolutionList,
                                           std::vector<std::string> &enabledSolutionList) const
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
            for (const auto &supportedSolution : supportedSolutionList)
            {
                bool isEnabled               = false;
                jvalue_ref json_solution_obj = jobject_create();

                jobject_put(json_solution_obj, J_CSTR_TO_JVAL("name"),
                            jstring_create(supportedSolution.c_str()));

                for (const auto &enabledSolution : enabledSolutionList)
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
                jobject_put(json_paramsobj, J_CSTR_TO_JVAL("Key_1"), jstring_create(string_value));
                jobject_put(json_paramsobj, J_CSTR_TO_JVAL("Key_2"), jnumber_create_i32(num_value));
                */
                jobject_put(json_solution_obj, J_CSTR_TO_JVAL("params"), json_paramsobj);

                jarray_append(json_supported_solutions_array, json_solution_obj);
            }

            jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SOLUTIONS),
                        json_supported_solutions_array);
        }
    }
    else
    {
        createJsonStringFailure(obj_reply, json_outobj);
    }

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
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

        j_solutions_obj = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_SOLUTIONS));

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
            jboolean_get(jobject_get(j_param_obj, J_CSTR_TO_BUF("Key1")), &enable);
            jboolean_get(jobject_get(j_param_obj, J_CSTR_TO_BUF("Key2")), &enable);
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

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
    j_release(&json_outobj);

    return str_reply;
}

GetFormatMethod::GetFormatMethod() : str_devid_(cstr_empty), ro_params_(), b_issubscribed_(false) {}

void GetFormatMethod::getObject(const char *input, const char *schemapath)
{
    jvalue_ref j_obj;
    jvalue_ref j_param_obj;
    int retval = deSerialize(input, schemapath, j_obj);

    if (0 == retval)
    {
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
        setCameraId(cstr_invaliddeviceid);
    }
    j_release(&j_obj);
}

std::string GetFormatMethod::createObjectJsonString() const
{
    jvalue_ref json_outobj       = jobject_create();
    jvalue_ref json_outobjparams = jobject_create();

    std::string str_reply;

    MethodReply obj_reply = getMethodReply();

    if (obj_reply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                    jboolean_create(obj_reply.bGetReturnValue()));

        std::string strformat = getFormatStringFromCode(rGetCameraFormat().eFormat);
        unsigned int w        = rGetCameraFormat().nWidth;
        unsigned int h        = rGetCameraFormat().nHeight;
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT),
                    jstring_create(strformat.c_str()));
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WIDTH),
                    jnumber_create_i32((w <= INT_MAX) ? w : 0));
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HEIGHT),
                    jnumber_create_i32((h <= INT_MAX) ? h : 0));
        jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FPS),
                    jnumber_create_i32(rGetCameraFormat().nFps));
        if (b_issubscribed_ == true)
        {
            jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SUBSCRIBED),
                        jboolean_create(b_issubscribed_));
        }
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PARAMS), json_outobjparams);
    }
    else
    {
        createJsonStringFailure(obj_reply, json_outobj);

        if (b_issubscribed_ == true)
        {
            jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SUBSCRIBED),
                        jboolean_create(b_issubscribed_));
        }
    }

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
    j_release(&json_outobj);

    return str_reply;
}

void EventNotificationMethod::getEventObject(const char *input, const char *schemapath)
{
    jvalue_ref j_obj = jobject_create();
    int retval       = deSerialize(input, schemapath, j_obj);

    if (retval == 0)
    {
        setIsErrorParam(false);
    }

    j_release(&j_obj);
}

std::string EventNotificationMethod::createObjectJsonString() const
{
    jvalue_ref json_outobj = jobject_create();
    std::string str_reply;
    MethodReply obj_reply = getMethodReply();

    if (obj_reply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE),
                    jboolean_create(obj_reply.bGetReturnValue()));

        if (b_issubscribed_ == true)
        {
            jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SUBSCRIBED),
                        jboolean_create(b_issubscribed_));
        }
    }
    else
    {
        createJsonStringFailure(obj_reply, json_outobj);
    }

    const char *str = jvalue_stringify(json_outobj);
    if (str)
        str_reply = str;
    j_release(&json_outobj);

    return str_reply;
}
