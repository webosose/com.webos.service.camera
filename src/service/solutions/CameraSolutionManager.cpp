/*
 * Mobile Communication Company, LG ELECTRONICS INC., SEOUL, KOREA
 * Copyright(c) 2017 by LG Electronics Inc.
 *
 * All rights reserved. No part of this work may be reproduced, stored in a
 * retrieval system, or transmitted by any means without prior written
 * Permission of LG Electronics Inc.
 */

#include "CameraSolutionManager.h"

#ifdef FEATURE_LG_AUTOCONTRAST
#include "solutions/AutoContrast/LGCameraSolutionAutoContrast.h"
#endif

#ifdef FEATURE_LG_FACEDETECTION
#include "solutions/FaceDetection/LGCameraSolutionFaceDetection.h"
#endif

#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "CameraSolutionManager"

CameraSolutionManager::CameraSolutionManager()
    : mAutoContrast(NULL),
      mFaceDetection(NULL),
      mEnabledSolutions(0),
      mEnabledSolutionCount(0)
{

#ifdef FEATURE_LG_AUTOCONTRAST
    mAutoContrast = new LGCameraSolutionAutoContrast(this);
    mAutoContrast->setSupportStatus(true);
    mTotalSolutionList.push_back(mAutoContrast);
#endif

#ifdef FEATURE_LG_FACEDETECTION
    mFaceDetection = new LGCameraSolutionFaceDetection(this);
    mFaceDetection->setSupportStatus(true);
    mTotalSolutionList.push_back(mFaceDetection);
#endif

}

CameraSolutionManager::~CameraSolutionManager()
{

    if (mAutoContrast != NULL)
        delete mAutoContrast;
    mAutoContrast = NULL;

    if (mFaceDetection != NULL)
            delete mFaceDetection;
    mFaceDetection = NULL;

}

void CameraSolutionManager::initializeForSolutions(stream_format_t streamformat, int key)
{

    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {

        if((*it)->isEnabled()){
            (*it)->initialize(streamformat, key);
        }
    }
}

void CameraSolutionManager::releaseForSolutions()
{

    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {

        if((*it)->isEnabled()){
            (*it)->release();
        }
    }
}

void CameraSolutionManager::processCaptureForSolutions(void* frame_buffer, stream_format_t streamformat)
{
    registerSolutionList(streamformat);

    for (std::list<CameraSolution *>::iterator it = mEnabledSolutionList.begin(); it != mEnabledSolutionList.end(); ++it) {
        //(*it)->initialize(streamformat);
        //if(mEnableDump) dumpImage(*it,images,numImageBuffer,DUMP_BEFORE);
        (*it)->processForSnapshot(frame_buffer, streamformat);
        //if(mEnableDump) dumpImage(*it,images,numImageBuffer,DUMP_AFTER);
    }
}

void CameraSolutionManager::processPreviewForSolutions(void* frame_buffer, stream_format_t streamformat)
{
    //registerSolutionList(streamformat);

    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {

        if((*it)->isEnabled()){
            //(*it)->initialize(streamformat);
            //if(mEnableDump) dumpImage(*it,images,numImageBuffer,DUMP_BEFORE);
            (*it)->processForPreview(frame_buffer, streamformat);
            //if(mEnableDump) dumpImage(*it,images,numImageBuffer,DUMP_AFTER);
        }
    }
}

void CameraSolutionManager::registerSolutionList(stream_format_t streamformat)
{
    int count = 0;
    mEnabledSolutionList.clear();

    //TODO: need to make interface to enable solutions
    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {
        if((*it)->isEnabled()){
            mEnabledSolutionList.push_back((*it));
            count++;
        }
    }

    if (count != mEnabledSolutionCount)
    {
        //LOGD(LOG_DEBUG_PRINT,"Enable Solution Error request count : %d / enable count : %d", mEnabledSolutionCount, count);
    }
    //LOGE(LOG_RELEASE, "mEnabledSolutions = 0x%x count %d", mEnabledSolutions, count);
}

void CameraSolutionManager::releaseSolutionList()
{
    mEnabledSolutionList.clear();
    mTotalSolutionList.clear();
}

void CameraSolutionManager::releaseSolutions() {
    for (std::list<CameraSolution *>::iterator it = mEnabledSolutionList.begin();
            it != mEnabledSolutionList.end(); ++it) {
        (*it)->release();
    }
    releaseSolutionList();
}

bool CameraSolutionManager::isEnableSolution(int solutionIndex) {
    return mEnabledSolutions & solutionIndex;
}

std::string CameraSolutionManager::getSupportedSolutionInfo() {
    PMLOG_INFO(CONST_MODULE_CM, "getSupportedSolutionInfo : E");

    return createSolutionListObjectJsonString();
}

std::string CameraSolutionManager::enableCameraSolutionInfo(const char *enabledSolutionList) {
    PMLOG_INFO(CONST_MODULE_CM, "CameraSolutionManager enableCameraSolutionInfo E\n");

    return enableSolutionListObjectJsonString(enabledSolutionList);
}

std::string CameraSolutionManager::disableCameraSolutionInfo(const char *disabledSolutionList) {
    PMLOG_INFO(CONST_MODULE_CM, "CameraSolutionManager disabledSolutionList E\n");

    return disableSolutionListObjectJsonString(disabledSolutionList);
}

std::string CameraSolutionManager::createSolutionListObjectJsonString()
{
    PMLOG_INFO(CONST_MODULE_CM, "CameraSolutionManager createSolutionListObjectJsonString E\n");
    int solutionCnt = 0;
    int errorType = NO_ERROR;
    jvalue_ref json_outobj = jobject_create();
    jvalue_ref json_solutioninfoarray = jarray_create(0);
    std::string strreply;

    //check supported solution list
    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {

        char solutionName[30];
        memcpy(solutionName, (*it)->getSolutionStr().data(), (*it)->getSolutionStr().size());
        jarray_append(json_solutioninfoarray,jstring_create(solutionName));

        solutionCnt++;

        PMLOG_INFO(CONST_MODULE_CM, "solution name %s \n",(*it)->getSolutionStr().c_str());
    }

    //no supported solution case
    if(solutionCnt == 0)
    {
        jarray_append(json_solutioninfoarray,jstring_create("No supported solution"));
        errorType = NO_SUPPORTED_SOLUTION;
    }

    jobject_put(json_outobj, J_CSTR_TO_JVAL("SolutionInfo"), json_solutioninfoarray);
    jobject_put(json_outobj, J_CSTR_TO_JVAL("returnValue"), jnumber_create_i32(errorType));

    strreply = jvalue_stringify(json_outobj);
    j_release(&json_outobj);
    PMLOG_INFO(CONST_MODULE_CM, "createSolutionListObjectJsonString : X , solution info %s \n",strreply.c_str());
    return strreply;
}

std::string CameraSolutionManager::enableSolutionListObjectJsonString(const char *enabledSolutionList)
{
    PMLOG_INFO(CONST_MODULE_CM, "CameraSolutionManager enableSolutionListObjectJsonString E\n");

    jvalue_ref j_obj = jobject_create();
    jvalue_ref json_solutionlistarray = jarray_create(0);
    std::string strreply;
    int solutionCnt = 0;
    int errorType = NO_ERROR;

    // parameter check
    if(enabledSolutionList == NULL){

        jobject_put(j_obj, J_CSTR_TO_JVAL("returnValue"),
                    jnumber_create_i32(PARAMETER_ERROR));

        strreply = jvalue_stringify(j_obj);

        PMLOG_INFO(CONST_MODULE_CM, "parameter is null \n");

        j_release(&j_obj);
        return strreply;
    }

    //enabled solutions following solution list set by app
    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {

        if(strstr(enabledSolutionList, (*it)->getSolutionStr().c_str()) != NULL){
            (*it)->setEnableValue(true);

            char solutionName[30];
            memcpy(solutionName, (*it)->getSolutionStr().data(), (*it)->getSolutionStr().size());
            jarray_append(json_solutionlistarray,jstring_create(solutionName));
            solutionCnt++;

            PMLOG_INFO(CONST_MODULE_CM, "enabled solutionName %s \n",solutionName);

        }
    }

    //no enabled solution case
    if(solutionCnt == 0)
    {
        jarray_append(json_solutionlistarray,jstring_create("no enabled solution"));
        errorType = NO_ENABLED_SOLUTION;
    }

    jobject_put(j_obj, J_CSTR_TO_JVAL("enabledSolutionList"), json_solutionlistarray);
    jobject_put(j_obj, J_CSTR_TO_JVAL("returnValue"), jnumber_create_i32(errorType));

    strreply = jvalue_stringify(j_obj);

    PMLOG_INFO(CONST_MODULE_CM, "enableSolutionListObjectJsonString %s \n",strreply);

    j_release(&j_obj);
    return strreply;

}

std::string CameraSolutionManager::disableSolutionListObjectJsonString(const char *disabledSolutionList)
{
    PMLOG_INFO(CONST_MODULE_CM, "CameraSolutionManager enableSolutionListObjectJsonString E\n");

    jvalue_ref j_obj = jobject_create();
    jvalue_ref json_solutionlistarray = jarray_create(0);
    std::string strreply;
    int solutionCnt = 0;
    int errorType = NO_ERROR;
    char solutionName[100];

    // parameter check
    if(disabledSolutionList == NULL){

        jobject_put(j_obj, J_CSTR_TO_JVAL("returnValue"),
                    jnumber_create_i32(PARAMETER_ERROR));

        strreply = jvalue_stringify(j_obj);

        PMLOG_INFO(CONST_MODULE_CM, "parameter is null \n");

        j_release(&j_obj);
        return strreply;
    }

    //enabled solutions following solution list set by app
    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {

        if(strstr(disabledSolutionList, (*it)->getSolutionStr().c_str()) != NULL){
            (*it)->setEnableValue(false);
            PMLOG_INFO(CONST_MODULE_CM, "disabled solutionName %s \n",solutionName);
        }
        if((*it)->isEnabled() == true)
        {
            char solutionName[30];
            memcpy(solutionName, (*it)->getSolutionStr().data(), (*it)->getSolutionStr().size());
            jarray_append(json_solutionlistarray,jstring_create(solutionName));
            solutionCnt++;
        }
    }

    //no enabled solution case
    if(solutionCnt == 0)
    {
        jarray_append(json_solutionlistarray,jstring_create("no enabled solution"));
        errorType = NO_ENABLED_SOLUTION;
    }

    jobject_put(j_obj, J_CSTR_TO_JVAL("enabledSolutionList"), json_solutionlistarray);
    jobject_put(j_obj, J_CSTR_TO_JVAL("returnValue"), jnumber_create_i32(errorType));

    strreply = jvalue_stringify(j_obj);

    PMLOG_INFO(CONST_MODULE_CM, "disableSolutionListObjectJsonString %s \n",strreply);

    j_release(&j_obj);
    return strreply;

}
