///******************************************************************************
// *   DTV LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
// *   Copyright(c) 1999-2016 by LG Electronics Inc.
//*
//*   All rights reserved. No part of this work may be reproduced, stored in a
// *   retrieval system, or transmitted by any means without prior written
// *   permission of LG Electronics Inc.
// *****************************************************************************/

///** @file h_command_service_util.c
//*
//* The requests to other service using luna sending
//* this file is related by LSCall or LSCallOneReply interface.
//*
//* @author    Woojin Choi (wjin.choi@lge.com)
//* @version    1.0
//* @date        2013. 11. 25
//* @note
//* @see
//*/

/*-----------------------------------------------------------------------------

 (Global Control Constants)
 ------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 #include
 (File Inclusions)
 ------------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

//#include "camera_common.h"
#include "command_service_util.h"
#include "luna-service2/lunaservice.h"

#include "service_main.h"

/*-----------------------------------------------------------------------------

 (Constant Definitions)
 ------------------------------------------------------------------------------*/
#define CONST_MAX_COMMAND_QUEUE_SIZE    10
#define CONST_MAX_TOKEN_TABLE_SIZE        20

/*-----------------------------------------------------------------------------

 (Macro Definitions)
 ------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------

 (Type Definitions)
 ------------------------------------------------------------------------------*/
typedef struct COMMAND_TABLE
{
    char strService[CONST_MAX_STRING_LENGTH];
    char strCategory[CONST_MAX_STRING_LENGTH];
    char strFunction[CONST_MAX_STRING_LENGTH];
    char strJson[CONST_MAX_LONG_STRING_LENGTH];
    int bSubscribe;
    pfnHandler pfnCB;
} COMMAND_TABLE_T;

typedef struct TOKEN_TABLE
{
    char strCall[CONST_MAX_STRING_LENGTH];
    LSMessageToken nToken;
    int bUsed;
} TOKEN_TABLE_T;

/*-----------------------------------------------------------------------------
 prototype
 (Extern Variables & External Function Prototype Declarations)
 ------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 Prototype
 (Define global variables)
 ------------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------
 Static Static prototype
 (Static Variables & Function Prototypes Declarations)
 ------------------------------------------------------------------------------*/
static sem_t gCommandSem;
static COMMAND_TABLE_T oCommandQueue[CONST_MAX_COMMAND_QUEUE_SIZE];
static int gnFront;
static int gnRear;
static int gbStartCommandThread;
static TOKEN_TABLE_T oTokenTable[CONST_MAX_TOKEN_TABLE_SIZE];
pthread_t gnCommandServiceThread;

static COMMANDSERVICE_RETURN_CODE_T _CAMERA_COMMANDSERVICE_Pop(COMMAND_TABLE_T *pCmd);
static void *_CAMERA_COMMANDSERVICE_Thread(void *arg);
static COMMANDSERVICE_RETURN_CODE_T _CAMERA_COMMANDSERVICE_AddToken(const char* pStrCall,
        LSMessageToken nToken);
static void _CAMERA_COMMANDSERVICE_CallCancelAll(void);

/*-----------------------------------------------------------------------------
 Static  Global
 (Implementation of static and global functions)
 ------------------------------------------------------------------------------*/

COMMANDSERVICE_RETURN_CODE_T CAMERA_COMMANDSERVICE_Init(void)
{
    CAMERA_PRINT_INFO("%s:%d] Started!!\n", __FUNCTION__, __LINE__);

    int i;

    for (i = 0; i < CONST_MAX_COMMAND_QUEUE_SIZE; i++)
        memset(&oCommandQueue[i], 0, sizeof(COMMAND_TABLE_T));

    gnFront = gnRear = 0;

    for (i = 0; i < CONST_MAX_TOKEN_TABLE_SIZE; i++)
        memset(&oTokenTable[i], 0, sizeof(TOKEN_TABLE_T));

    // semaphore
    sem_init(&gCommandSem, 0, 0);

    gbStartCommandThread = 1;

    if (pthread_create(&gnCommandServiceThread, NULL,
            (void*(*)(void*))_CAMERA_COMMANDSERVICE_Thread, NULL) != 0 )
    {
        CAMERA_PRINT_INFO("%s:%d] create Command Service thread failed!!\n",__FUNCTION__,__LINE__);
        return COMMANDSERVICE_ERROR_UNKNOWN;
    }

    CAMERA_PRINT_INFO("%s:%d] Ended!!\n", __FUNCTION__, __LINE__);
    return COMMANDSERVICE_RETURN_OK;
}

COMMANDSERVICE_RETURN_CODE_T CAMERA_COMMANDSERVICE_Uninit(void)
{
    CAMERA_PRINT_INFO("%s:%d] Started!!\n", __FUNCTION__, __LINE__);

    gbStartCommandThread = 0;

    sem_post(&gCommandSem);

    _CAMERA_COMMANDSERVICE_CallCancelAll();

    CAMERA_PRINT_INFO("%s:%d] Ended!!\n", __FUNCTION__, __LINE__);
    return COMMANDSERVICE_RETURN_OK;
}

COMMANDSERVICE_RETURN_CODE_T CAMERA_COMMANDSERVICE_Push(const char* pService, const char* pCategory,
        const char* pFunction, const char* pJsonString, int bSubscribe, pfnHandler pfnCB)
{
    CAMERA_PRINT_INFO("%s:%d] Started!!(Service: %s, Category: %s, function: %s, Json: %s)\n",
            __FUNCTION__, __LINE__, pService, pCategory, pFunction, pJsonString);

    if ((gnRear + 1) % CONST_MAX_COMMAND_QUEUE_SIZE == gnFront)
        return COMMANDSERVICE_ERROR_TABLE_IS_FULL;

    strncpy(oCommandQueue[gnRear].strService, pService, CONST_MAX_STRING_LENGTH - 1);
    oCommandQueue[gnRear].strService[CONST_MAX_STRING_LENGTH - 1] = '\0';

    if (pCategory != NULL)
    {
        strncpy(oCommandQueue[gnRear].strCategory, pCategory, CONST_MAX_STRING_LENGTH - 1);
        oCommandQueue[gnRear].strCategory[CONST_MAX_STRING_LENGTH - 1] = '\0';
    }
    strncpy(oCommandQueue[gnRear].strFunction, pFunction, CONST_MAX_STRING_LENGTH - 1);
    oCommandQueue[gnRear].strFunction[CONST_MAX_STRING_LENGTH - 1] = '\0';
    strncpy(oCommandQueue[gnRear].strJson, pJsonString, CONST_MAX_LONG_STRING_LENGTH - 1);
    oCommandQueue[gnRear].strJson[CONST_MAX_LONG_STRING_LENGTH - 1] = '\0';
    oCommandQueue[gnRear].bSubscribe = bSubscribe;
    oCommandQueue[gnRear].pfnCB = pfnCB;

    gnRear = (gnRear + 1) % CONST_MAX_COMMAND_QUEUE_SIZE;

    sem_post(&gCommandSem);

    CAMERA_PRINT_INFO("%s:%d] Ended!!\n", __FUNCTION__, __LINE__);
    return COMMANDSERVICE_RETURN_OK;
}

static COMMANDSERVICE_RETURN_CODE_T _CAMERA_COMMANDSERVICE_Pop(COMMAND_TABLE_T *pCmd)
{
    CAMERA_PRINT_INFO("%s:%d] Started!!\n", __FUNCTION__, __LINE__);

    if (gnFront == gnRear)
        return COMMANDSERVICE_ERROR_TABLE_IS_EMPTY;

    if (pCmd == NULL)
        return COMMANDSERVICE_ERROR_WRONG_PARAM;

    strncpy(pCmd->strService, oCommandQueue[gnFront].strService, CONST_MAX_STRING_LENGTH);
    strncpy(pCmd->strCategory, oCommandQueue[gnFront].strCategory, CONST_MAX_STRING_LENGTH);
    strncpy(pCmd->strFunction, oCommandQueue[gnFront].strFunction, CONST_MAX_STRING_LENGTH);
    strncpy(pCmd->strJson, oCommandQueue[gnFront].strJson, CONST_MAX_LONG_STRING_LENGTH);
    pCmd->bSubscribe = oCommandQueue[gnFront].bSubscribe;
    pCmd->pfnCB = oCommandQueue[gnFront].pfnCB;

    memset(&oCommandQueue[gnFront], 0, sizeof(COMMAND_TABLE_T));

    gnFront = (gnFront + 1) % CONST_MAX_COMMAND_QUEUE_SIZE;

    CAMERA_PRINT_INFO("%s:%d] Ended!!\n", __FUNCTION__, __LINE__);
    return COMMANDSERVICE_RETURN_OK;
}

static void *_CAMERA_COMMANDSERVICE_Thread(void *arg)
{
    CAMERA_PRINT_INFO("%s:%d] Started!!\n", __FUNCTION__, __LINE__);

    COMMAND_TABLE_T oCmd;
    LSError lserror;
    bool ret = false;
    char strTemp[CONST_MAX_STRING_LENGTH];
    LSMessageToken nToken;

    while (gbStartCommandThread)
    {
        // signal
        CAMERA_PRINT_INFO("%s:%d] Wating...\n", __FUNCTION__, __LINE__);
        sem_wait(&gCommandSem);
        CAMERA_PRINT_INFO("%s:%d] Received!!!\n", __FUNCTION__, __LINE__);

        if (_CAMERA_COMMANDSERVICE_Pop(&oCmd) != COMMANDSERVICE_RETURN_OK)
        {
            CAMERA_PRINT_INFO("%s:%d] Command service queue pop error!!\n", __FUNCTION__, __LINE__);
            continue;
        }
        else
            CAMERA_PRINT_INFO(
                    "%s:%d] Pop ok! (Service: %s, Category: %s, function: %s, Json: %s)!!\n",
                    __FUNCTION__, __LINE__, oCmd.strService, oCmd.strCategory, oCmd.strFunction,
                    oCmd.strJson);

        if (strncmp(oCmd.strCategory, "", CONST_MAX_STRING_LENGTH) != 0)
            snprintf(strTemp, CONST_MAX_STRING_LENGTH, "%s/%s/%s", oCmd.strService,
                    oCmd.strCategory, oCmd.strFunction);
        else
            snprintf(strTemp, CONST_MAX_STRING_LENGTH, "%s/%s", oCmd.strService, oCmd.strFunction);

        CAMERA_PRINT_INFO("%s:%d] %s '%s'", __FUNCTION__, __LINE__, strTemp, oCmd.strJson);

        LSErrorInit(&lserror);

        if (oCmd.bSubscribe)
        {
            ret = LSCall(camera_ls2_getHandle(), strTemp, oCmd.strJson, oCmd.pfnCB, NULL, &nToken,
                    &lserror);
            _CAMERA_COMMANDSERVICE_AddToken(strTemp, nToken);
        }
        else
            ret = LSCallOneReply(camera_ls2_getHandle(), strTemp, oCmd.strJson, oCmd.pfnCB, NULL,
                    NULL, &lserror);

        if (!ret)
        {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }
    }

    // semaphore
    sem_destroy(&gCommandSem);

    CAMERA_PRINT_INFO("%s:%d] Ended!!\n", __FUNCTION__, __LINE__);

    pthread_detach(gnCommandServiceThread);

    return NULL;
}

static COMMANDSERVICE_RETURN_CODE_T _CAMERA_COMMANDSERVICE_AddToken(const char* pStrCall,
        LSMessageToken nToken)
{
    CAMERA_PRINT_INFO("%s:%d] Started!!(pStrCall: %s, nToken: %ld)\n", __FUNCTION__, __LINE__,
            pStrCall, nToken);

    int i;

    for (i = 0; i < CONST_MAX_TOKEN_TABLE_SIZE; i++)
    {
        if (oTokenTable[i].bUsed && strstr(oTokenTable[i].strCall, pStrCall) != NULL)
        {
            CAMERA_PRINT_INFO("%s:%d] Duplicated call. please check camera-service!!\n",
                    __FUNCTION__, __LINE__);
            return COMMANDSERVICE_ERROR_DUPLICATED;
        }
    }

    for (i = 0; i < CONST_MAX_TOKEN_TABLE_SIZE; i++)
    {
        if (!oTokenTable[i].bUsed)
        {
            strncpy(oTokenTable[i].strCall, pStrCall, CONST_MAX_STRING_LENGTH - 1);
            oTokenTable[i].strCall[CONST_MAX_STRING_LENGTH - 1] = '\0';
            oTokenTable[i].nToken = nToken;
            oTokenTable[i].bUsed = 1;
            break;
        }

    }

    CAMERA_PRINT_INFO("%s:%d] Ended!!\n", __FUNCTION__, __LINE__);
    return COMMANDSERVICE_RETURN_OK;
}

COMMANDSERVICE_RETURN_CODE_T CAMERA_COMMANDSERVICE_CallCancel(const char* pService,
        const char* pCategory, const char* pFunction)
{
    CAMERA_PRINT_INFO("%s:%d] Started!!(Service: %s, Category: %s, function: %s)\n", __FUNCTION__,
            __LINE__, pService, pCategory, pFunction);

    int i;
    int bFound;
    int nToken;
    char strTemp[CONST_MAX_STRING_LENGTH];
    LSError lserror;
    bool ret = false;

    bFound = 0;
    nToken = -1;

    if (pCategory != NULL)
        snprintf(strTemp, CONST_MAX_STRING_LENGTH, "%s/%s/%s", pService, pCategory, pFunction);
    else
        snprintf(strTemp, CONST_MAX_STRING_LENGTH, "%s/%s", pService, pFunction);

    for (i = 0; i < CONST_MAX_TOKEN_TABLE_SIZE; i++)
    {
        if (oTokenTable[i].bUsed && strstr(oTokenTable[i].strCall, strTemp) != NULL)
        {
            bFound = 1;
            nToken = oTokenTable[i].nToken;

            memset(&oTokenTable[i], 0, sizeof(TOKEN_TABLE_T));
            break;
        }
    }

    if (bFound)
    {
        LSErrorInit(&lserror);

        CAMERA_PRINT_INFO("%s:%d] Callcancel started. (%s, %d)\n", __FUNCTION__, __LINE__, strTemp,
                nToken);
        ret = LSCallCancel(camera_ls2_getHandle(), nToken, &lserror);

        if (!ret)
        {
            LSErrorPrint(&lserror, stderr);
            LSErrorFree(&lserror);
        }
    }

    CAMERA_PRINT_INFO("%s:%d] Ended!!\n", __FUNCTION__, __LINE__);
    return COMMANDSERVICE_RETURN_OK;
}

static void _CAMERA_COMMANDSERVICE_CallCancelAll(void)
{
    CAMERA_PRINT_INFO("%s:%d] Started!!\n", __FUNCTION__, __LINE__);

    int i;
    LSError lserror;
    bool ret = false;

    for (i = 0; i < CONST_MAX_TOKEN_TABLE_SIZE; i++)
    {
        if (oTokenTable[i].bUsed)
        {
            CAMERA_PRINT_INFO("%s:%d] Callcancel started. (%s, %ld)\n", __FUNCTION__, __LINE__,
                    oTokenTable[i].strCall, oTokenTable[i].nToken);

            LSErrorInit(&lserror);
            ret = LSCallCancel(camera_ls2_getHandle(), oTokenTable[i].nToken, &lserror);

            if (!ret)
            {
                LSErrorPrint(&lserror, stderr);
                LSErrorFree(&lserror);
            }

            memset(&oTokenTable[i], 0, sizeof(TOKEN_TABLE_T));
        }
    }

    CAMERA_PRINT_INFO("%s:%d] Ended!!\n", __FUNCTION__, __LINE__);
    return;
}

