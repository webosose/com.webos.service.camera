/******************************************************************************
 *   DTV LABORATORY, LG ELECTRONICS INC., SEOUL, KOREA
 *   Copyright(c) 1999 by LG Electronics Inc.
 *
 *   All rights reserved. No part of this work may be reproduced, stored in a
 *   retrieval system, or transmitted by any means without prior written
 *   permission of LG Electronics Inc.
 *****************************************************************************/

/** @file h_command_service_util.c
 *
 * The requests to other service using luna sending
 * this file is related by LSCall or LSCallOneReply interface.
 *
 * @author     Woojin Choi (wjin.choi@lge.com)
 * @version  1.0
 * @date     2013. 11. 25
 * @note
 * @see
 */

#ifndef _CAMERA_COMMANDSERVICEUTIL_H_
#define _CAMERA_COMMANDSERVICEUTIL_H_

/*-----------------------------------------------------------------------------

 (File Inclusions)
 ------------------------------------------------------------------------------*/

#ifdef    __cplusplus
extern "C"
{
#endif /* __cplusplus */

#include "camera_types.h"

COMMANDSERVICE_RETURN_CODE_T CAMERA_COMMANDSERVICE_Init(void);

COMMANDSERVICE_RETURN_CODE_T CAMERA_COMMANDSERVICE_Uninit(void);
COMMANDSERVICE_RETURN_CODE_T CAMERA_COMMANDSERVICE_Push(const char* pService, const char* pCategory,
        const char* pFunction, const char* pJsonString, int bSubscribe, pfnHandler pfnCB);
COMMANDSERVICE_RETURN_CODE_T CAMERA_COMMANDSERVICE_CallCancel(const char* pService,
        const char* pCategory, const char* pFunction);

#ifdef    __cplusplus
}
#endif /* __cplusplus */

#endif /*_CAMERA_COMMANDSERVICEUTIL_H_*/

