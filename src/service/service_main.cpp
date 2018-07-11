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

/*-----------------------------------------------------------------------------
 #include
 (File Inclusions)
 -- ----------------------------------------------------------------------------*/

#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <pthread.h>

#include "command_manager.h"
#include "device_manager.h"
#include "device_controller.h"
#include "command_service_util.h"
#include "service_main.h"
#include "service_utils.h"

GMainLoop *g_mainLoop;
GMainLoop *g_Mainloop;

int fd;

CommandManager *CommandManager::cmdhandlerinstance = nullptr;
CommandManager *devCmd = CommandManager::getInstance();

LSMethod camera_methods[] =
{

{ "open", CameraService::open },
{ "close", CameraService::close },
{ "getInfo", CameraService::getInfo },
{ "getList", CameraService::getList },
{ "createHandle", CameraService::createHandle },
{ "getProperties", CameraService::getProperties },
{ "setProperties", CameraService::setProperties },
{ "startCapture", CameraService::startCapture },
{ "stopCapture", CameraService::stopCapture },
{ "startPreview", CameraService::startPreview },
{ "stopPreview", CameraService::stopPreview },
{ "captureNimage", CameraService::captureImage },
{ "loadPlugin", CameraService::loadPlugin }, };

LSHandle *_gpstLsHandle = NULL;

/*-----------------------------------------------------------------------------

 Function
 ------------------------------------------------------------------------------*/
static void *_ls2_handleloop(void *data)
{
    //g_main_loop_run(g_mainLoop);
    return NULL;
}

int parse_parameter(char *pID, char *pDevice, DEVICE_TYPE *pDevType, int *pDevID, int *pnID)
{
    char strServiceName[CONST_MAX_STRING_LENGTH];
    char strDevName[CONST_MAX_STRING_LENGTH];
    char strDevCount[CONST_MAX_STRING_LENGTH];
    int nDevID;
    int nID;
    DEVICE_TYPE_T nType;

    if (pID == NULL || strlen(pID) < 10 || strlen(pID) >= CONST_MAX_STRING_LENGTH)
        return DEVICE_ERROR_WRONG_PARAM;
    else
        sscanf(pID, "camera://%[^'/']/%[^'/']/%d", strServiceName, strDevName, &nID);

    PMLOG_INFO(CONST_MODULE_LUNA, "serviceName: %s, devname: %s ID %d", strServiceName, strDevName,
            nID);

    if (strncmp(strServiceName, CONST_SERVICE_NAME_CAMERA, strlen(CONST_SERVICE_NAME_CAMERA)) != 0)
        return DEVICE_ERROR_WRONG_PARAM;

    if (strncmp(strDevName, CONST_DEVICE_NAME_CAMERA, strlen(CONST_DEVICE_NAME_CAMERA)) == 0)
    {
        strncpy(strDevCount, strDevName + strlen(CONST_DEVICE_NAME_CAMERA),
                CONST_MAX_STRING_LENGTH);
        PMLOG_INFO(CONST_MODULE_LUNA, "strDevCount: %s", strDevCount);

        nDevID = atoi(strDevCount);
        nType = DEVICE_CAMERA;
    }
    else if (strncmp(strDevName, CONST_DEVICE_NAME_MIC, strlen(CONST_DEVICE_NAME_MIC)) == 0)
    {
        strncpy(strDevCount, strDevName + strlen(CONST_DEVICE_NAME_MIC), CONST_MAX_STRING_LENGTH);
        PMLOG_INFO(CONST_MODULE_LUNA, "strDevCount: %s", strDevCount);

        nDevID = atoi(strDevCount);
        nType = DEVICE_MICROPHONE;
    }
    else
        return DEVICE_ERROR_WRONG_PARAM;

    *pDevType = nType;
    *pDevID = nDevID;
    *pDevID = nID;

    return DEVICE_OK;
}

bool CameraService::open(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    int ret = -1;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL;
    struct json_object *pOutJson = NULL;
    DEVICE_RETURN_CODE nErrID;
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        int Id;
        DEVICE_TYPE DevType;
        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter %s %s\n", devID, devType);
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            PMLOG_INFO(CONST_MODULE_LUNA, "DevID %d nId %d\n", DevID, Id);
            ret = devCmd->open(DevID, DevType);

            if ((ret != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: open, Return: %s", json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    LSMessageUnref(message);

    if (pInJson)
        json_object_put(pInJson);
    pInJson = NULL;

    if (pOutJson)
        json_object_put(pOutJson);
    pOutJson = NULL;
    return TRUE;
}

bool CameraService::close(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    int ret = -1;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL;
    struct json_object *pOutJson = NULL;
    DEVICE_RETURN_CODE_T nErrID;
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        int Id;
        DEVICE_TYPE DevType;
        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            nErrID = devCmd->close(DevID, DevType);
            if ((nErrID != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                PMLOG_INFO(CONST_MODULE_LUNA, "DevID %d devtype %d\n", DevID, DevType);
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: close, Return: %s", json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "close success\n");

    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);
    return TRUE;
}

bool CameraService::startPreview(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    int ret;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL;
    struct json_object *pOutJson = NULL;
    DEVICE_RETURN_CODE nErrID;
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        int Id;
        DEVICE_TYPE DevType;
        int pKey;

        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;
        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            ret = devCmd->startPreview(DevID, DevType, pKey); //enable

            if ((ret != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(pKey));
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: start, Return: %s", json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "start success\n");

    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);
    return TRUE;
}

bool CameraService::stopPreview(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    int ret;
    DEVICE_RETURN_CODE nErrID;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL;
    struct json_object *pOutJson = NULL;

    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        int Id;
        DEVICE_TYPE DevType;
        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            ret = devCmd->stopPreview(DevID, DevType); //enable

            if ((ret != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: stop, Return: %s", json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
    PMLOG_INFO(CONST_MODULE_LUNA, "stop success\n");
    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);

    return TRUE;
}

bool CameraService::startCapture(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    int ret;
    DEVICE_RETURN_CODE nErrID;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL, *pInJsonChild2 = NULL;
    struct json_object *pOutJson = NULL;
    PMLOG_INFO(CONST_MODULE_LUNA, "entering function startcapture\n");
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    PMLOG_INFO(CONST_MODULE_LUNA, "pInJson\n");
    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        DEVICE_TYPE DevType;
        int nWidth = 0;
        int nHeight = 0;
        char *pFormat = NULL;
        char *pCodec = NULL;
        char *pSamplingRate = NULL;
        CAMERA_DATA_FORMAT nformat;
        FORMAT sFormat; // for camera
        int Id;

        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_PARAMS, &pInJsonChild1))
        {
            if (strncmp(devType, CONST_PARAM_NAME_CAMERA, 6) == 0)
            {
                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_WIDTH,
                                &pInJsonChild2))
                    nWidth = json_object_get_int(pInJsonChild2);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_HEIGHT,
                                &pInJsonChild2))
                    nHeight = json_object_get_int(pInJsonChild2);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_FORMAT,
                                &pInJsonChild2))
                    pFormat = (char *) json_object_get_string(pInJsonChild2);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;
            }
            else if (strncmp(devType, CONST_PARAM_NAME_MICROPHONE, 10) == 0)
            {
                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_SAMPLINGRATE,
                                &pInJsonChild2))
                    pSamplingRate = (char *) json_object_get_string(pInJsonChild2);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_CODEC,
                                &pInJsonChild2))
                    pCodec = (char *) json_object_get_string(pInJsonChild2);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;
            }
        }
        else
        {
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;
        }

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            if (DEVICE_CAMERA == DevType)
            {
                _ConvertFormatToCode(pFormat, &nformat);
                sFormat.eFormat = nformat;
                sFormat.nHeight = nHeight;
                sFormat.nWidth = nWidth;

                ret = devCmd->startCapture(DevID, DevType, sFormat); //enable

                if ((ret != DEVICE_OK))
                {
                    json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                            json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                    json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                            json_object_new_int(nErrID));
                    json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                            json_object_new_string(_GetErrorString(nErrID)));
                }
                else
                {
                    json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                            json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
                }
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: startcapture, Return: %s",
            json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
    PMLOG_INFO(CONST_MODULE_LUNA, "startcapture success\n");
    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);

    return TRUE;
}

bool CameraService::stopCapture(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    int ret;
    DEVICE_RETURN_CODE nErrID;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL;
    struct json_object *pOutJson = NULL;

    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        int Id;
        DEVICE_TYPE DevType;
        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            ret = devCmd->stopCapture(DevID, DevType); //enable

            if ((ret != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: stop, Return: %s", json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
    PMLOG_INFO(CONST_MODULE_LUNA, "stop success\n");
    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);

    return TRUE;
}

bool CameraService::captureImage(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    int ret;
    DEVICE_RETURN_CODE nErrID;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL, *pInJsonChild2 = NULL;
    struct json_object *pOutJson = NULL;

    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        int Id;
        DEVICE_TYPE DevType;
        int nCount;
        FORMAT sFormat;
        int nWidth = 0;
        int nHeight = 0;
        char *pFormat = NULL;
        CAMERA_DATA_FORMAT nformat;

        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_DEFAULT_NIMAGE, &pInJsonChild1))
            nCount = json_object_get_int(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;
        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_PARAMS, &pInJsonChild1))
        {
            if (strncmp(devType, CONST_PARAM_NAME_CAMERA, 6) == 0)
            {
                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_WIDTH,
                                &pInJsonChild2))
                    nWidth = json_object_get_int(pInJsonChild2);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_HEIGHT,
                                &pInJsonChild2))
                    nHeight = json_object_get_int(pInJsonChild2);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_FORMAT,
                                &pInJsonChild2))
                    pFormat = (char *) json_object_get_string(pInJsonChild2);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

            }
        }
        else
        {
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;
        }

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            _ConvertFormatToCode(pFormat, &nformat);
            sFormat.eFormat = nformat;
            sFormat.nHeight = nHeight;
            sFormat.nWidth = nWidth;
            ret = devCmd->captureImage(DevID, DevType, nCount, sFormat); //enable

            if ((ret != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: stop, Return: %s", json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
    PMLOG_INFO(CONST_MODULE_LUNA, "stop success\n");
    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);

    return TRUE;
}

bool CameraService::getInfo(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    bool ret = true;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL;
    struct json_object *pOutJson = NULL, *pOutJsonChild1 = NULL, *pOutJsonChild2 = NULL,
            *pOutJsonChild3 = NULL;
    DEVICE_RETURN_CODE nErrID;
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        int Id;
        DEVICE_TYPE DevType;
        CAMERA_INFO_T oInfo;
        char strFormat[CONST_MAX_STRING_LENGTH];

        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            ret = devCmd->getDeviceInfo(DevID, DevType, &oInfo);

            if ((ret != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));

                pOutJsonChild1 = json_object_new_object();
                json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_NAME,
                        json_object_new_string(oInfo.strName));
                json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_TYPE,
                        json_object_new_string(_GetTypeString(oInfo.nType)));
                json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_BUILTIN,
                        json_object_new_boolean(oInfo.bBuiltin));

                pOutJsonChild2 = json_object_new_object();

                if (oInfo.nType == DEVICE_CAMERA)
                {
                    pOutJsonChild3 = json_object_new_object();
                    json_object_object_add(pOutJsonChild3, CONST_PARAM_NAME_MAXWIDTH,
                            json_object_new_int(oInfo.nMaxVideoWidth));
                    json_object_object_add(pOutJsonChild3, CONST_PARAM_NAME_MAXHEIGHT,
                            json_object_new_int(oInfo.nMaxVideoHeight));
                    json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_VIDEO, pOutJsonChild3);

                    pOutJsonChild3 = json_object_new_object();
                    json_object_object_add(pOutJsonChild3, CONST_PARAM_NAME_MAXWIDTH,
                            json_object_new_int(oInfo.nMaxPictureWidth));
                    json_object_object_add(pOutJsonChild3, CONST_PARAM_NAME_MAXHEIGHT,
                            json_object_new_int(oInfo.nMaxPictureHeight));
                    json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_PICTURE,
                            pOutJsonChild3);

                    _GetFormatString(oInfo.nFormat, strFormat);
                    json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_FORMAT,
                            json_object_new_string(strFormat));
                }
                else if (oInfo.nType == DEVICE_MICROPHONE)
                {
                    //json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_SAMPLINGRATE, json_object_new_string(_GetSamplingRateString(oInfo.nSamplingRate)));
                    //json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_CODEC, json_object_new_string(_GetCodecString(oInfo.nCodec)));
                }

                json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_DETAILS, pOutJsonChild2);
                json_object_object_add(pOutJson, CONST_PARAM_NAME_INFO, pOutJsonChild1);
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: getInfo, Return: %s", json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "getInfo success\n");

    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);
    return TRUE;
}

bool CameraService::getList(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    bool ret = true;
    const char *payload;
    struct json_object *pInJson = NULL;
    struct json_object *pOutJson = NULL, *pOutJsonChild1 = NULL, *pOutJsonChild2 = NULL;
    DEVICE_RETURN_CODE nErrID;
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        int arrCamSupport[CONST_MAX_DEVICE_COUNT], arrMicSupport[CONST_MAX_DEVICE_COUNT];
        int arrCamDev[CONST_MAX_DEVICE_COUNT], arrMicDev[CONST_MAX_DEVICE_COUNT];
        char arrList[20][CONST_MAX_STRING_LENGTH];
        int supportList[20];
        int i;

        pOutJson = json_object_new_object();

        PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");

        int nCamCount;
        int nMicCount;
        for (i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
        {
            arrCamDev[i] = arrMicDev[i] = CONST_VARIABLE_INITIALIZE;
            arrCamSupport[i] = arrMicSupport[i] = 0;
        }

        ret = devCmd->getDeviceList(arrCamDev, arrMicDev, arrCamSupport, arrMicSupport);

        if ((ret != DEVICE_OK))
        {
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_TRUE));

            nCamCount = 0;
            for (i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
            {
                if (arrCamDev[i] == CONST_VARIABLE_INITIALIZE)
                    break;

                snprintf(arrList[nCamCount], CONST_MAX_STRING_LENGTH, "%s://%s/%s%d",
                        CONST_SERVICE_URI_NAME, CONST_SERVICE_NAME_CAMERA, CONST_DEVICE_NAME_CAMERA,
                        arrCamDev[i]);
                supportList[nCamCount] = arrCamSupport[i];
                nCamCount++;
            }

            nMicCount = 0;
            for (i = 0; i < CONST_MAX_DEVICE_COUNT; i++)
            {
                if (arrMicDev[i] == CONST_VARIABLE_INITIALIZE)
                    break;

                snprintf(arrList[nCamCount + nMicCount], CONST_MAX_STRING_LENGTH, "%s://%s/%s%d",
                        CONST_SERVICE_URI_NAME, CONST_SERVICE_NAME_CAMERA, CONST_DEVICE_NAME_MIC,
                        arrMicDev[i]);
                supportList[nCamCount + nMicCount] = arrMicSupport[i];
                nMicCount++;
            }

            pOutJsonChild1 = json_object_new_array();

            for (i = 0; i < nCamCount; i++)
            {
                pOutJsonChild2 = json_object_new_object();
                json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_URI,
                        json_object_new_string(arrList[i]));
                json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_TYPE,
                        json_object_new_string(_GetTypeString(DEVICE_CAMERA)));
                json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_SUPPORT,
                        json_object_new_boolean(supportList[i]));
                json_object_array_add(pOutJsonChild1, pOutJsonChild2);
            }

            for (i = nCamCount; i < nCamCount + nMicCount; i++)
            {
                pOutJsonChild2 = json_object_new_object();
                json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_URI,
                        json_object_new_string(arrList[i]));
                json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_TYPE,
                        json_object_new_string(_GetTypeString(DEVICE_MICROPHONE)));
                json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_SUPPORT,
                        json_object_new_boolean(supportList[i]));
                json_object_array_add(pOutJsonChild1, pOutJsonChild2);
            }

            json_object_object_add(pOutJson, CONST_PARAM_NAME_URILIST, pOutJsonChild1);
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: getList, Return: %s", json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "getList success\n");

    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);
    return TRUE;
}

bool CameraService::createHandle(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    bool ret = true;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL;
    struct json_object *pOutJson = NULL;
    DEVICE_RETURN_CODE nErrID;
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        DEVICE_TYPE DevType;
        int Id;
        int DevHandle;

        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter %d\n", DevHandle);
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            ret = devCmd->createHandle(DevID, DevType, &DevHandle);

            if ((ret != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
                json_object_object_add(pOutJson, CONST_DEVICE_HANDLE,
                        json_object_new_int(DevHandle));

            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: create handle, Return: %s",
            json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "getInfo success\n");

    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);
    return TRUE;
}

bool CameraService::getProperties(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    bool ret = true;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL;
    struct json_object *pOutJson = NULL, *pOutJsonChild1 = NULL, *pOutJsonChild2 = NULL;
    DEVICE_RETURN_CODE_T nErrID;
    CAMERA_PROPERTIES_T dev_property;
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        int Id;
        DEVICE_TYPE DevType;

        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            nErrID = devCmd->getProperty(DevID, DevType, &dev_property);

            if ((ret != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));

                pOutJsonChild1 = json_object_new_object();

                if (dev_property.nZoom != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_ZOOM,
                            json_object_new_int(dev_property.nZoom));

                if (dev_property.nGridZoomX != CONST_VARIABLE_INITIALIZE)
                {
                    pOutJsonChild2 = json_object_new_object();
                    json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_COL,
                            json_object_new_int(dev_property.nGridZoomX));
                    json_object_object_add(pOutJsonChild2, CONST_PARAM_NAME_ROW,
                            json_object_new_int(dev_property.nGridZoomY));
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_GRIDZOOM,
                            pOutJsonChild2);
                }

                if (dev_property.nPan != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_PAN,
                            json_object_new_int(dev_property.nPan));

                if (dev_property.nTilt != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_TILT,
                            json_object_new_int(dev_property.nTilt));

                if (dev_property.nContrast != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_CONTRAST,
                            json_object_new_int(dev_property.nContrast));

                if (dev_property.nBrightness != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_BIRGHTNESS,
                            json_object_new_int(dev_property.nBrightness));

                if (dev_property.nSaturation != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_SATURATION,
                            json_object_new_int(dev_property.nSaturation));

                if (dev_property.nSharpness != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_SHARPNESS,
                            json_object_new_int(dev_property.nSharpness));

                if (dev_property.nHue != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_HUE,
                            json_object_new_int(dev_property.nHue));

                if (dev_property.nWhiteBalanceTemperature != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_WHITEBALANCETEMPERATURE,
                            json_object_new_int(dev_property.nWhiteBalanceTemperature));

                if (dev_property.nGain != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_GAIN,
                            json_object_new_int(dev_property.nGain));

                if (dev_property.nGamma != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_GAMMA,
                            json_object_new_int(dev_property.nGamma));

                if (dev_property.nFrequency != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_FREQUENCY,
                            json_object_new_int(dev_property.nFrequency));

                if (dev_property.bMirror != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_MIRROR,
                            json_object_new_boolean(dev_property.bMirror));

                if (dev_property.nExposure != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_EXPOSURE,
                            json_object_new_int(dev_property.nExposure));

                if (dev_property.bAutoExposure != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_AUTOEXPOSURE,
                            json_object_new_boolean(dev_property.bAutoExposure));

                if (dev_property.bAutoWhiteBalance != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_AUTOWHITEBALANCE,
                            json_object_new_boolean(dev_property.bAutoWhiteBalance));

                if (dev_property.nBitrate != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_BITRATE,
                            json_object_new_int(dev_property.nBitrate));

                if (dev_property.nFramerate != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_FRAMERATE,
                            json_object_new_int(dev_property.nFramerate));

                if (dev_property.ngopLength != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_GOPLENGTH,
                            json_object_new_int(dev_property.ngopLength));

                if (dev_property.bLed != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_LED,
                            json_object_new_boolean(dev_property.bLed));

                if (dev_property.bYuvMode != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_YUVMODE,
                            json_object_new_boolean(dev_property.bYuvMode));

                if (dev_property.bBacklightCompensation != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_BACKLIGHT_COMPENSATION,
                            json_object_new_boolean(dev_property.bBacklightCompensation));

                if (dev_property.nMicMaxGain != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_MICMAXGAIN,
                            json_object_new_int(dev_property.nMicMaxGain));

                if (dev_property.nMicMinGain != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_MICMINGAIN,
                            json_object_new_int(dev_property.nMicMinGain));

                if (dev_property.nMicGain != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_MICGAIN,
                            json_object_new_int(dev_property.nMicGain));

                if (dev_property.bMicMute != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_MICMUTE,
                            json_object_new_boolean(dev_property.bMicMute));

                if (dev_property.bMicMute != CONST_VARIABLE_INITIALIZE)
                    json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_MICMUTE,
                            json_object_new_boolean(dev_property.bMicMute));

                json_object_object_add(pOutJson, CONST_PARAM_NAME_PARAMS, pOutJsonChild1);
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: getProperties, Return: %s",
            json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
    PMLOG_INFO(CONST_MODULE_LUNA, "getProperties success\n");

    LSMessageUnref(message);
    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);
    return TRUE;
}

bool CameraService::setProperties(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    bool ret = true;
    const char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL, *pInJsonChild2 = NULL,
            *pInJsonChild3 = NULL;
    struct json_object *pOutJson = NULL;
    DEVICE_RETURN_CODE nErrID;
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);
    payload = LSMessageGetPayload(message);
    pInJson = json_tokener_parse(payload);

    if (!(pInJson))
    {
        pOutJson = json_object_new_object();

        nErrID = DEVICE_ERROR_JSON_PARSING;
        json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE, json_object_new_int(nErrID));
        json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                json_object_new_string(_GetErrorString(nErrID)));
    }
    else
    {
        char *devID = NULL;
        char *devType = NULL;
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        int DevID;
        int Id;
        DEVICE_TYPE DevType;
        CAMERA_PROPERTIES_T oParams;
        int nParamCount;

        nParamCount = 0;
        oParams.nZoom = CONST_VARIABLE_INITIALIZE;
        oParams.nGridZoomX = CONST_VARIABLE_INITIALIZE;
        oParams.nGridZoomY = CONST_VARIABLE_INITIALIZE;
        oParams.nPan = CONST_VARIABLE_INITIALIZE;
        oParams.nTilt = CONST_VARIABLE_INITIALIZE;
        oParams.nContrast = CONST_VARIABLE_INITIALIZE;
        oParams.nBrightness = CONST_VARIABLE_INITIALIZE;
        oParams.nSaturation = CONST_VARIABLE_INITIALIZE;
        oParams.nSharpness = CONST_VARIABLE_INITIALIZE;
        oParams.nHue = CONST_VARIABLE_INITIALIZE;
        oParams.nWhiteBalanceTemperature = CONST_VARIABLE_INITIALIZE;
        oParams.nGain = CONST_VARIABLE_INITIALIZE;
        oParams.nGamma = CONST_VARIABLE_INITIALIZE;
        oParams.nFrequency = CONST_VARIABLE_INITIALIZE;
        oParams.bMirror = CONST_VARIABLE_INITIALIZE;
        oParams.nExposure = CONST_VARIABLE_INITIALIZE;
        oParams.bAutoExposure = CONST_VARIABLE_INITIALIZE;
        oParams.bAutoWhiteBalance = CONST_VARIABLE_INITIALIZE;
        oParams.nBitrate = CONST_VARIABLE_INITIALIZE;
        oParams.nFramerate = CONST_VARIABLE_INITIALIZE;
        oParams.ngopLength = CONST_VARIABLE_INITIALIZE;
        oParams.bLed = CONST_VARIABLE_INITIALIZE;
        oParams.bYuvMode = CONST_VARIABLE_INITIALIZE;
        oParams.bBacklightCompensation = CONST_VARIABLE_INITIALIZE;

        oParams.nMicMaxGain = CONST_VARIABLE_INITIALIZE;
        oParams.nMicMinGain = CONST_VARIABLE_INITIALIZE;
        oParams.nMicGain = CONST_VARIABLE_INITIALIZE;
        oParams.bMicMute = CONST_VARIABLE_INITIALIZE;

        pOutJson = json_object_new_object();

        if (nParamCheck && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_ID, &pInJsonChild1))
            devID = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_TYPE, &pInJsonChild1))
            devType = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_PARAMS, &pInJsonChild1))
        {
            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_ZOOM, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nZoom = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_GRIDZOOM, &pInJsonChild2))
            {
                nParamCount++;
                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild2, CONST_PARAM_NAME_COL,
                                &pInJsonChild3))
                    oParams.nGridZoomX = json_object_get_int(pInJsonChild3);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

                if (nParamCheck
                        && json_object_object_get_ex(pInJsonChild2, CONST_PARAM_NAME_ROW,
                                &pInJsonChild3))
                    oParams.nGridZoomY = json_object_get_int(pInJsonChild3);
                else
                    nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_PAN, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nPan = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_TILT, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nTilt = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_CONTRAST, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nContrast = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_BIRGHTNESS,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.nBrightness = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_SATURATION,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.nSaturation = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_SHARPNESS,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.nSharpness = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_HUE, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nHue = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_WHITEBALANCETEMPERATURE,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.nWhiteBalanceTemperature = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_GAIN, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nGain = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_GAMMA, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nGamma = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_FREQUENCY,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.nFrequency = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_MIRROR, &pInJsonChild2))
            {
                nParamCount++;
                oParams.bMirror = json_object_get_boolean(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_EXPOSURE, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nExposure = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_AUTOEXPOSURE,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.bAutoExposure = json_object_get_boolean(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_AUTOWHITEBALANCE,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.bAutoWhiteBalance = json_object_get_boolean(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_BITRATE, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nBitrate = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_FRAMERATE,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.nFramerate = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_GOPLENGTH,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.ngopLength = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_LED, &pInJsonChild2))
            {
                nParamCount++;
                oParams.bLed = json_object_get_boolean(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_YUVMODE, &pInJsonChild2))
            {
                nParamCount++;
                oParams.bYuvMode = json_object_get_boolean(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_BACKLIGHT_COMPENSATION,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.bBacklightCompensation = json_object_get_boolean(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_MICMAXGAIN,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.nMicMaxGain = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_MICMINGAIN,
                    &pInJsonChild2))
            {
                nParamCount++;
                oParams.nMicMinGain = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_MICGAIN, &pInJsonChild2))
            {
                nParamCount++;
                oParams.nMicGain = json_object_get_int(pInJsonChild2);
            }

            if (json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_MICMUTE, &pInJsonChild2))
            {
                nParamCount++;
                oParams.bMicMute = json_object_get_boolean(pInJsonChild2);
            }
        }

        if (!nParamCheck)
        {
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }
        else
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");
            ret = parse_parameter(devID, devType, &DevType, &DevID, &Id);
            ret = devCmd->setProperty(DevID, DevType, &oParams);

            if ((ret != DEVICE_OK))
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                        json_object_new_int(nErrID));
                json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                        json_object_new_string(_GetErrorString(nErrID)));
            }
            else
            {
                json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                        json_object_new_boolean(CONST_PARAM_VALUE_TRUE));
            }
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: setProperties, Return: %s",
            json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);

    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "setProperties success\n");

    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);
    return TRUE;
}

bool CameraService::loadPlugin(LSHandle *sh, LSMessage *message, void *ctx)
{

    LSError lserror;
    int ret = -1;
    char *payload;
    struct json_object *pInJson = NULL, *pInJsonChild1 = NULL, *pInJsonChild2 = NULL;
    struct json_object *pOutJson = NULL, *pOutJsonChild1 = NULL;
    char *plugin = NULL;
    char *plugin_name = NULL;
    char *path = NULL;

    DEVICE_RETURN_CODE nErrID;
    // json parsing
    LSErrorInit(&lserror);
    LSMessageRef(message);

    int c;
    FILE *fp = fopen("plugin.json", "rt");
    if (!fp)
    {
        PMLOG_INFO(CONST_MODULE_LUNA, "Unable to open file \n");
        return false;
    }
    int size = 0;
    int i;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    payload = (char *) malloc((size + 1) * sizeof(char));

    while ((c = fgetc(fp)) != EOF)
    {
        payload[i] = c;
        i++;
    }
    payload[i] = '\0';

    {
        int nParamCheck = CONST_PARAM_VALUE_TRUE;
        pInJson = json_tokener_parse(payload);

        pOutJson = json_object_new_object();

        PMLOG_INFO(CONST_MODULE_LUNA, "Starting parse_parameter\n");

        if (nParamCheck
                && json_object_object_get_ex(pInJson, CONST_PARAM_NAME_PLUGIN, &pInJsonChild1))
            plugin = (char *) json_object_get_string(pInJsonChild1);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_NAME_PLUGIN_NAME,
                        &pInJsonChild2))
            plugin_name = (char *) json_object_get_string(pInJsonChild2);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (nParamCheck
                && json_object_object_get_ex(pInJsonChild1, CONST_PARAM_PLUGIN_PATH,
                        &pInJsonChild2))
            path = (char *) json_object_get_string(pInJsonChild2);
        else
            nParamCheck = nParamCheck & CONST_PARAM_VALUE_FALSE;

        if (!nParamCheck)
        {
            PMLOG_INFO(CONST_MODULE_LUNA, "nParamCheck failed\n");
            nErrID = DEVICE_ERROR_PARAM_IS_MISSING;
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_FALSE));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_CODE,
                    json_object_new_int(nErrID));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_ERROR_TEXT,
                    json_object_new_string(_GetErrorString(nErrID)));
        }

        else
        {
            pOutJson = json_object_new_object();
            json_object_object_add(pOutJson, CONST_PARAM_NAME_RETURNVALUE,
                    json_object_new_boolean(CONST_PARAM_VALUE_TRUE));

            pOutJsonChild1 = json_object_new_object();

            json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_PLUGIN,
                    json_object_new_string(plugin));
            json_object_object_add(pOutJsonChild1, CONST_PARAM_NAME_PLUGIN_NAME,
                    json_object_new_string(plugin_name));
            json_object_object_add(pOutJsonChild1, CONST_PARAM_PLUGIN_PATH,
                    json_object_new_string(path));
            json_object_object_add(pOutJson, CONST_PARAM_NAME_INFO, pOutJsonChild1);
        }
    }

    PMLOG_INFO(CONST_MODULE_LUNA, "API: load_plugin, Return: %s",
            json_object_to_json_string(pOutJson));
    LSErrorInit(&lserror);
    ret = LSMessageReply(sh, message, json_object_to_json_string(pOutJson), &lserror);
    if (!ret)
    {
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }
    LSMessageUnref(message);

    H_SERVICE_JSON_PUT(pInJson);
    H_SERVICE_JSON_PUT(pOutJson);
    return TRUE;
}

LSHandle* camera_ls2_getHandle(void)
{
    return _gpstLsHandle;
}

int main(int argc, char *argv[])
{
    g_print("Start luna for camera service\n");
    GThread *_gMainLoopThread;
    LSError lserror;
    LSErrorInit(&lserror);

    CAMERA_COMMANDSERVICE_Init();
    g_Mainloop = g_main_loop_new(NULL, FALSE);
    g_mainLoop = g_main_loop_new(NULL, FALSE);

    if (!LSRegister("com.webos.service.camera2", &_gpstLsHandle, &lserror))
    {
        g_print("Unable to register to luna-bus\n");
        LSErrorPrint(&lserror, stdout);
        return false;
    }

    if (!LSRegisterCategory(_gpstLsHandle, "/", camera_methods, NULL, NULL, &lserror))
    {
        g_print("Unable to register category and method\n");
        LSErrorPrint(&lserror, stdout);
        return false;
    }

    if (!LSGmainAttach(_gpstLsHandle, g_mainLoop, &lserror))
    {
        g_print("Unable to attach service\n");
        LSErrorPrint(&lserror, stdout);

        return false;
    }

    _gMainLoopThread = g_thread_new("camera_LS_service", _ls2_handleloop, NULL);

    _sender_get_nonstorage_device_list();
    g_main_loop_run(g_mainLoop);
    g_main_loop_run(g_Mainloop);

    if (!LSUnregister(_gpstLsHandle, &lserror))
    {
        g_print("Unable to unregister service\n");
        LSErrorPrint(&lserror, stdout);
        return false;
    }

    g_thread_join(_gMainLoopThread);

    g_main_loop_unref(g_mainLoop);
    g_main_loop_unref(g_Mainloop);
    g_mainLoop = NULL;

    return 0;
}
