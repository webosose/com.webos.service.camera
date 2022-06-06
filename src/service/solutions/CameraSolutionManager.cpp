/**
 * Copyright(c) 2022 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    CameraSolutionManager.cpp
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution Manager
 *
 */

#include "CameraSolutionManager.h"

#include "solutions/AutoContrast/LGCameraSolutionAutoContrast.h"
#include "solutions/Dummy/LGCameraSolutionDummy.h"

#ifdef FEATURE_LG_OPENCV_FACEDETECTION
#include "solutions/FaceDetection/LGCameraSolutionFaceDetection.h"
#endif

#ifdef FEATURE_LG_AIF_FACEDETECTION
#include "solutions/AIFFaceDetection/LGCameraSolutionAIFFaceDetection.h"
#endif


#ifdef LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "CameraSolutionManager"

CameraSolutionManager::CameraSolutionManager()
    : mAutoContrast(NULL),
      mFaceDetection(NULL),
      mDummy(NULL),
      mAIFFaceDetection(NULL)

{

    pbnjson::JValue obj_solutionInfo = nullptr;

    bool retValue = loadSolutionList(obj_solutionInfo);
    if(retValue != SOLUTION_MANAGER_NO_ERROR)
    {
        PMLOG_ERROR(CONST_MODULE_SM, "failed to get solution list info so can't enable solutions. (error:%d)",retValue);
        return;
    }

    if(isSolutionSupported(obj_solutionInfo, SOLUTION_AUTOCONTRAST))
    {
        mAutoContrast = new LGCameraSolutionAutoContrast(this);
        mAutoContrast->setSupportStatus(true);
        mTotalSolutionList.push_back(mAutoContrast);
    }

    if(isSolutionSupported(obj_solutionInfo, SOLUTION_DUMMY))
    {
        mDummy = new LGCameraSolutionDummy(this);
        mDummy->setSupportStatus(true);
        mTotalSolutionList.push_back(mDummy);
    }

#ifdef FEATURE_LG_OPENCV_FACEDETECTION
    if(isSolutionSupported(obj_solutionInfo, SOLUTION_OPENCV_FACEDETECTION))
    {
        mFaceDetection = new LGCameraSolutionFaceDetection(this);
        mFaceDetection->setSupportStatus(true);
        mTotalSolutionList.push_back(mFaceDetection);
    }
#endif

#ifdef FEATURE_LG_AIF_FACEDETECTION
    if(isSolutionSupported(obj_solutionInfo, SOLUTION_AIF_FACEDETECTION))
    {
        mAIFFaceDetection = new LGCameraSolutionAIFFaceDetection(this);
        mAIFFaceDetection->setSupportStatus(true);
        mTotalSolutionList.push_back(mAIFFaceDetection);
    }
#endif


}

CameraSolutionManager::~CameraSolutionManager()
{

    if (mAutoContrast != NULL)
    {
        delete mAutoContrast;
        mAutoContrast = NULL;
    }

    if (mFaceDetection != NULL)
    {
        delete mFaceDetection;
        mFaceDetection = NULL;
    }

    if (mDummy != NULL)
    {
        delete mDummy;
        mDummy = NULL;
    }

    if (mAIFFaceDetection != NULL)
    {
        delete mAIFFaceDetection;
        mAIFFaceDetection = NULL;
    }

}

LgSolutionErrorValue CameraSolutionManager::loadSolutionList(pbnjson::JValue& json)
{
    std::string jsonPath_ = "/etc/com.webos.service.camera/supported_solution_info.conf";
    auto obj_supportedSolution = pbnjson::JDomParser::fromFile(jsonPath_.c_str());
    if (!obj_supportedSolution.isObject()) {
        PMLOG_ERROR(CONST_MODULE_SM, "configuration file parsing error! need to check %s", jsonPath_.c_str());
        return SOLUTION_MANAGER_PARSING_ERROR;
    }

    // check solution_info field
    if (!obj_supportedSolution.hasKey("solutionInfo"))
    {
        PMLOG_ERROR(CONST_MODULE_SM, "Can't find solutionInfo field. need to check it!");
        return SOLUTION_MANAGER_PARAMETER_ERROR;
    }

    json = obj_supportedSolution["solutionInfo"];

    return SOLUTION_MANAGER_NO_ERROR;
}

bool CameraSolutionManager::isSolutionSupported(const pbnjson::JValue& json, const std::string& key)
{
    bool value = false;

    auto solutionList = json[0]; //we already know array number of solutionInfo is 1 and never get increased, so we put 0 on it.

    if(solutionList.hasKey(key)
        && solutionList[key].isBoolean()
        && solutionList[key].asBool(value) == CONV_OK)
    {
        if(value == true)
        {
            PMLOG_ERROR(CONST_MODULE_SM, "%s is enabled",key.c_str());
        }
    }

    return value;
}


void CameraSolutionManager::initializeForSolutions(stream_format_t streamformat)
{

    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {
        (*it)->initialize(streamformat);
    }
}

void CameraSolutionManager::releaseForSolutions()
{

    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {

        if((*it)->isEnabled()){
            (*it)->release();
        }
    }

    mTotalSolutionList.clear();
}

void CameraSolutionManager::processCaptureForSolutions(buffer_t frame_buffer, stream_format_t streamformat)
{

    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {

        if((*it)->isEnabled()
            && (*it)->solutionProperty & LG_SOLUTION_SNAPSHOT)
        {
            (*it)->processForSnapshot(frame_buffer, streamformat);
        }
    }
}

void CameraSolutionManager::processPreviewForSolutions(buffer_t frame_buffer, stream_format_t streamformat)
{

    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it) {

        if((*it)->isEnabled()
            && (*it)->solutionProperty & LG_SOLUTION_PREVIEW)
        {
            (*it)->processForPreview(frame_buffer, streamformat);
        }
    }
}

void CameraSolutionManager::releaseSolutionList()
{
    mTotalSolutionList.clear();
}

void CameraSolutionManager::getSupportedSolutionInfo(std::vector<std::string>& solutionsInfo)
{
    PMLOG_INFO(CONST_MODULE_SM, "getSupportedSolutionInfo : E");

    //check supported solution list
    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it)
    {
        solutionsInfo.push_back((*it)->getSolutionStr());
        PMLOG_INFO(CONST_MODULE_SM, "solution name %s \n",(*it)->getSolutionStr().c_str());
    }
}

void CameraSolutionManager::getEnabledSolutionInfo(std::vector<std::string>& solutionsInfo)
{
    PMLOG_INFO(CONST_MODULE_SM, "getSupportedSolutionInfo : E");

    //check supported solution list
    for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it)
    {
        if((*it)->isEnabled() == true)
        {
            solutionsInfo.push_back((*it)->getSolutionStr());
            PMLOG_INFO(CONST_MODULE_SM, "solution name %s \n",(*it)->getSolutionStr().c_str());
        }
    }
}

DEVICE_RETURN_CODE_T CameraSolutionManager::enableCameraSolution(const std::vector<std::string> solutions)
{
    PMLOG_INFO(CONST_MODULE_SM, "CameraSolutionManager enableCameraSolution E\n");
    std::list<CameraSolution *> mCandidateSolutionList;

    for(std::string solution : solutions)
    {
        for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it)
        {
            if(solution.compare((*it)->getSolutionStr()) == 0)
            {
                mCandidateSolutionList.push_back(*it);
                PMLOG_INFO(CONST_MODULE_SM, "enabled solutionName %s\n", solution.c_str());
            }
        }

    }

    if(solutions.size() == mCandidateSolutionList.size())
    {
        for (std::list<CameraSolution *>::iterator it = mCandidateSolutionList.begin(); it != mCandidateSolutionList.end(); ++it)
        {
            (*it)->setEnableValue(true);
        }
    }
    else
    {
        if(solutions.size() == 0 )
        {
            return DEVICE_ERROR_PARAM_IS_MISSING;
        }
        return DEVICE_ERROR_WRONG_PARAM;
    }
    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T CameraSolutionManager::disableCameraSolution(const std::vector<std::string> solutions)
{
    PMLOG_INFO(CONST_MODULE_SM, "CameraSolutionManager disabledSolutionList E\n");
    std::list<CameraSolution *> mCandidateSolutionList;

    for(std::string solution : solutions)
    {
        for (std::list<CameraSolution *>::iterator it = mTotalSolutionList.begin(); it != mTotalSolutionList.end(); ++it)
        {
            if(solution.compare((*it)->getSolutionStr()) == 0)
            {
                mCandidateSolutionList.push_back(*it);
                PMLOG_INFO(CONST_MODULE_SM, "disabled solutionName %s\n", solution.c_str());
            }
        }

    }

    if(solutions.size() == mCandidateSolutionList.size())
    {
        for (std::list<CameraSolution *>::iterator it = mCandidateSolutionList.begin(); it != mCandidateSolutionList.end(); ++it)
        {
            (*it)->setEnableValue(false);
        }
    }
    else
    {
        if(solutions.size() == 0 )
        {
            return DEVICE_ERROR_PARAM_IS_MISSING;
        }
        return DEVICE_ERROR_WRONG_PARAM;
    }
    return DEVICE_OK;

}
