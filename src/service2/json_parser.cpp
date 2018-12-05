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

std::string GetCameraListMethod::createCameraListObjectJsonString()
{
    jvalue_ref json_outobj = jobject_create();
    jvalue_ref json_outdevicelist = jobject_create();
    std::string str_reply;

    MethodReply objreply = getMethodReply();

    if (objreply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(objreply.bGetReturnValue()));

        int count = getCameraCount();
        for (int i = 0; i < count; i++)
        {
            jobject_put(json_outdevicelist, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ID), jstring_create(strGetCameraList(i).c_str()));
        }

        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DEVICE_LIST), json_outdevicelist);
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
    }
    else
    {
        setCameraId(invalid_device_id);
    }
    j_release(&j_obj);
}

std::string OpenMethod::createOpenObjectJsonString()
{
    jvalue_ref json_outobj = jobject_create();
    std::string str_reply;

    MethodReply obj_reply = getMethodReply();

    if (obj_reply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(obj_reply.bGetReturnValue()));
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_DEVICE_HANDLE), jnumber_create_i32(getDeviceHandle()));
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
        raw_buffer type = jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_TYPE)));
        raw_buffer source = jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_SOURCE)));

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

std::string StartPreviewMethod::createStartPreviewObjectJsonString()
{
    jvalue_ref json_outobj = jobject_create();
    std::string str_reply;

    MethodReply obj_reply = getMethodReply();

    if (obj_reply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(obj_reply.bGetReturnValue()));
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

std::string StopPreviewCaptureCloseMethod::createObjectJsonString()
{
    jvalue_ref json_outobj = jobject_create();
    std::string str_reply;

    MethodReply objreply = getMethodReply();

    if (objreply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(objreply.bGetReturnValue()));
    }
    else
    {
        createJsonStringFailure(objreply, json_outobj);
    }

    str_reply = jvalue_stringify(json_outobj);
    j_release(&json_outobj);

    return str_reply;
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

        camera_format_t rcamera_params;

        jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
        jvalue_ref j_params = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WIDTH));
        jnumber_get_i32(j_params, &rcamera_params.n_width);

        j_params = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HEIGHT));
        jnumber_get_i32(j_params, &rcamera_params.n_height);

        raw_buffer strformat = jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FORMAT)));
        std::string format = strformat.m_str;

        CameraFormat nformat;
        convertFormatToCode(format, &nformat);
        rcamera_params.e_format = nformat;

        setCameraParams(rcamera_params);

        raw_buffer strimg_mode = jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MODE)));
        str_mode_ = strimg_mode.m_str;

        if (0 == strcmp(str_mode_.c_str(), "MODE_BURST"))
        {
            j_params = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_NIMAGE));
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

std::string StartCaptureMethod::createStartCaptureObjectJsonString()
{
    jvalue_ref json_outobj = jobject_create();
    std::string str_reply;

    MethodReply objreply = getMethodReply();

    if (objreply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(objreply.bGetReturnValue()));
    }
    else
    {
        createJsonStringFailure(objreply, json_outobj);
    }

    str_reply = jvalue_stringify(json_outobj);
    j_release(&json_outobj);

    return str_reply;
}

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
        setDeviceId(invalid_device_id);
    }
    j_release(&j_obj);
}

std::string GetInfoMethod::createInfoObjectJsonString()
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
        std::string strformat;
        getFormatString(rGetCameraInfo().n_format, strformat);
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(objreply.bGetReturnValue()));

        jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_NAME), jstring_create(rGetCameraInfo().str_name.c_str()));
        jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TYPE), jstring_create(getTypeString(rGetCameraInfo().e_type).c_str()));
        jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BUILTIN), jboolean_create(rGetCameraInfo().b_builtin));

        jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXWIDTH), jnumber_create_i32(rGetCameraInfo().n_maxvideowidth));
        jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXHEIGHT), jnumber_create_i32(rGetCameraInfo().n_maxvideoheight));
        jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FRAMERATE), jnumber_create_i32(30));
        jobject_put(json_video_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT), jstring_create(strformat.c_str()));

        jobject_put(json_pictureobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXWIDTH), jnumber_create_i32(rGetCameraInfo().n_maxpicturewidth));
        jobject_put(json_pictureobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MAXHEIGHT), jnumber_create_i32(rGetCameraInfo().n_maxpictureheight));
        jobject_put(json_pictureobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FORMAT), jstring_create(strformat.c_str()));

        jobject_put(json_detailsobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_VIDEO), json_video_obj);
        jobject_put(json_detailsobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PICTURE), json_pictureobj);

        jobject_put(json_info_obj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_DETAILS), json_detailsobj);
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_INFO), json_detailsobj);
    }
    else
    {
        createJsonStringFailure(objreply, json_outobj);
    }

    strreply = jvalue_stringify(json_outobj);
    j_release(&json_outobj);

    return strreply;
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

std::string GetSetPropertiesMethod::createGetPropertiesObjectJsonString()
{
    jvalue_ref json_outobj = jobject_create();
    jvalue_ref json_outobjparams = jobject_create();
    std::string strreply;

    MethodReply objreply = getMethodReply();

    if (objreply.bGetReturnValue())
    {
        json_outobj = jobject_create();
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(objreply.bGetReturnValue()));
        json_outobjparams = jobject_create();

        if (rGetCameraProperties().n_zoom != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ZOOM), jnumber_create_i32(rGetCameraProperties().n_zoom));
        if (rGetCameraProperties().n_gridzoom_x != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_COL), jnumber_create_i32(rGetCameraProperties().n_gridzoom_x));
        if (rGetCameraProperties().n_gridzoom_y != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ROW), jnumber_create_i32(rGetCameraProperties().n_gridzoom_y));
        if (rGetCameraProperties().n_pan != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_PAN), jnumber_create_i32(rGetCameraProperties().n_pan));
        if (rGetCameraProperties().n_tilt != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_TILT), jnumber_create_i32(rGetCameraProperties().n_tilt));
        if (rGetCameraProperties().n_contrast != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_CONTRAST), jnumber_create_i32(rGetCameraProperties().n_contrast));
        if (rGetCameraProperties().n_brightness != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BIRGHTNESS), jnumber_create_i32(rGetCameraProperties().n_brightness));
        if (rGetCameraProperties().n_saturation != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SATURATION), jnumber_create_i32(rGetCameraProperties().n_saturation));
        if (rGetCameraProperties().n_sharpness != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_SHARPNESS), jnumber_create_i32(rGetCameraProperties().n_sharpness));
        if (rGetCameraProperties().n_hue != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_HUE), jnumber_create_i32(rGetCameraProperties().n_hue));
        if (rGetCameraProperties().n_whitebalancetemperature != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE), jnumber_create_i32(rGetCameraProperties().n_whitebalancetemperature));
        if (rGetCameraProperties().n_gain != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAIN), jnumber_create_i32(rGetCameraProperties().n_gain));
        if (rGetCameraProperties().n_gamma != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GAMMA), jnumber_create_i32(rGetCameraProperties().n_gamma));
        if (rGetCameraProperties().n_frequency != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FREQUENCY), jnumber_create_i32(rGetCameraProperties().n_frequency));
        if (rGetCameraProperties().b_mirror != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MIRROR), jboolean_create(rGetCameraProperties().b_mirror));
        if (rGetCameraProperties().n_exposure != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_EXPOSURE), jnumber_create_i32(rGetCameraProperties().n_exposure));
        if (rGetCameraProperties().b_autoexposure != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOEXPOSURE), jboolean_create(rGetCameraProperties().b_autoexposure));
        if (rGetCameraProperties().b_autowhitebalance != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_AUTOWHITEBALANCE), jboolean_create(rGetCameraProperties().b_autowhitebalance));
        if (rGetCameraProperties().n_bitrate != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BITRATE), jnumber_create_i32(rGetCameraProperties().n_bitrate));
        if (rGetCameraProperties().n_framerate != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_FRAMERATE), jnumber_create_i32(rGetCameraProperties().n_framerate));
        if (rGetCameraProperties().ngop_length != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_GOPLENGTH), jnumber_create_i32(rGetCameraProperties().ngop_length));
        if (rGetCameraProperties().b_led != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_LED), jboolean_create(rGetCameraProperties().b_led));
        if (rGetCameraProperties().b_yuvmode != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_YUVMODE), jboolean_create(rGetCameraProperties().b_yuvmode));
        if (rGetCameraProperties().n_illumination != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ILLUMINATION), jnumber_create_i32(rGetCameraProperties().n_illumination));
        if (rGetCameraProperties().b_backlightcompensation != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION), jboolean_create(rGetCameraProperties().b_backlightcompensation));
        if (rGetCameraProperties().n_mic_maxgain != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMAXGAIN), jnumber_create_i32(rGetCameraProperties().n_mic_maxgain));
        if (rGetCameraProperties().n_mic_mingain != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMINGAIN), jnumber_create_i32(rGetCameraProperties().n_mic_mingain));
        if (rGetCameraProperties().n_micgain != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICGAIN), jnumber_create_i32(rGetCameraProperties().n_micgain));
        if (rGetCameraProperties().b_micmute != CONST_VARIABLE_INITIALIZE)
            jobject_put(json_outobjparams, J_CSTR_TO_JVAL(CONST_PARAM_NAME_MICMUTE), jboolean_create(rGetCameraProperties().b_micmute));

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

        _camera_properties r_camproperties;

        jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
        jvalue_ref jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_ZOOM));
        jnumber_get_i32(jparams, &r_camproperties.n_zoom);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_COL));
        jnumber_get_i32(jparams, &r_camproperties.n_gridzoom_x);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_ROW));
        jnumber_get_i32(jparams, &r_camproperties.n_gridzoom_y);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_PAN));
        jnumber_get_i32(jparams, &r_camproperties.n_pan);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_TILT));
        jnumber_get_i32(jparams, &r_camproperties.n_tilt);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_CONTRAST));
        jnumber_get_i32(jparams, &r_camproperties.n_contrast);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_BIRGHTNESS));
        jnumber_get_i32(jparams, &r_camproperties.n_brightness);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_SATURATION));
        jnumber_get_i32(jparams, &r_camproperties.n_saturation);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_SHARPNESS));
        jnumber_get_i32(jparams, &r_camproperties.n_sharpness);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HUE));
        jnumber_get_i32(jparams, &r_camproperties.n_hue);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WHITEBALANCETEMPERATURE));
        jnumber_get_i32(jparams, &r_camproperties.n_whitebalancetemperature);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_GAIN));
        jnumber_get_i32(jparams, &r_camproperties.n_gain);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_GAMMA));
        jnumber_get_i32(jparams, &r_camproperties.n_gamma);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FREQUENCY));
        jnumber_get_i32(jparams, &r_camproperties.n_frequency);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MIRROR));
        jboolean_get(jparams, (bool *)&r_camproperties.b_mirror);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_EXPOSURE));
        jnumber_get_i32(jparams, &r_camproperties.n_exposure);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_AUTOEXPOSURE));
        jboolean_get(jparams, (bool *)&r_camproperties.b_autoexposure);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_AUTOWHITEBALANCE));
        jboolean_get(jparams, (bool *)&r_camproperties.b_autowhitebalance);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_BITRATE));
        jnumber_get_i32(jparams, &r_camproperties.n_bitrate);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FRAMERATE));
        jnumber_get_i32(jparams, &r_camproperties.n_framerate);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_GOPLENGTH));
        jnumber_get_i32(jparams, &r_camproperties.ngop_length);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_LED));
        jboolean_get(jparams, (bool *)&r_camproperties.b_led);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_YUVMODE));
        jboolean_get(jparams, (bool *)&r_camproperties.b_yuvmode);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_ILLUMINATION));
        jnumber_get_i32(jparams, &r_camproperties.n_illumination);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_BACKLIGHT_COMPENSATION));
        jboolean_get(jparams, (bool *)&r_camproperties.b_backlightcompensation);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MICMAXGAIN));
        jnumber_get_i32(jparams, &r_camproperties.n_mic_maxgain);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MICMINGAIN));
        jnumber_get_i32(jparams, &r_camproperties.n_mic_mingain);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MICGAIN));
        jnumber_get_i32(jparams, &r_camproperties.n_micgain);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_MICMUTE));
        jboolean_get(jparams, (bool *)&r_camproperties.b_micmute);

        setCameraProperties(r_camproperties);
    }
    else
    {
        setDeviceHandle(n_invalid_id);
    }
    j_release(&j_obj);
}

std::string GetSetPropertiesMethod::createSetPropertiesObjectJsonString()
{
    jvalue_ref json_outbj = jobject_create();
    std::string strreply;

    MethodReply objreply = getMethodReply();

    if (objreply.bGetReturnValue())
    {
        jobject_put(json_outbj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(objreply.bGetReturnValue()));
    }
    else
    {
        createJsonStringFailure(objreply, json_outbj);
    }

    strreply = jvalue_stringify(json_outbj);
    j_release(&json_outbj);

    return strreply;
}

void SetFormatMethod::getSetFormatObject(const char *input, const char *schemapath)
{
    jvalue_ref j_obj;
    int retval = deSerialize(input, schemapath, j_obj);

    if (0 == retval)
    {
        int devicehandle = n_invalid_id;
        jnumber_get_i32(jobject_get(j_obj, J_CSTR_TO_BUF(CONST_DEVICE_HANDLE)), &devicehandle);
        setDeviceHandle(devicehandle);

        camera_format_t rcameraparams;

        jvalue_ref jobj_params = jobject_get(j_obj, J_CSTR_TO_BUF(CONST_PARAM_NAME_PARAMS));
        jvalue_ref jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_WIDTH));
        jnumber_get_i32(jparams, &rcameraparams.n_width);
        jparams = jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_HEIGHT));
        jnumber_get_i32(jparams, &rcameraparams.n_height);
        raw_buffer strformat = jstring_get_fast(jobject_get(jobj_params, J_CSTR_TO_BUF(CONST_PARAM_NAME_FORMAT)));
        std::string format = strformat.m_str;

        CameraFormat eformat;
        convertFormatToCode(format, &eformat);
        rcameraparams.e_format = eformat;

        setCameraFormat(rcameraparams);
    }
    else
    {
        setDeviceHandle(n_invalid_id);
    }
    j_release(&j_obj);
}

std::string SetFormatMethod::createSetFormatObjectJsonString()
{
    jvalue_ref json_outobj = jobject_create();
    std::string strreply;

    MethodReply objreply = getMethodReply();

    if (objreply.bGetReturnValue())
    {
        jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(objreply.bGetReturnValue()));
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
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_RETURNVALUE), jboolean_create(obj_reply.bGetReturnValue()));
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_CODE), jnumber_create_i32(obj_reply.getErrorCode()));
    jobject_put(json_outobj, J_CSTR_TO_JVAL(CONST_PARAM_NAME_ERROR_TEXT), jstring_create(obj_reply.strGetErrorText().c_str()));

    return;
}
