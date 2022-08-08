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

#include "camera_solution_manager.h"
#include "auto_contrast/auto_contrast.hpp"
#include "camera_solution.h"
#include "camera_types.h"
#include "dummy/dummy.hpp"
#include <pbnjson.hpp>

#ifdef FEATURE_LG_OPENCV_FACEDETECTION
#include "facedetection_opencv/facedetection_opencv.hpp"
#endif

#ifdef FEATURE_LG_AIF_FACEDETECTION
#include "facedetection_aif/facedetection_aif.hpp"
#endif

#ifdef FEATURE_LG_CNN_FACEDETECTION
#include "facedetection_cnn/facedetection_cnn.hpp"
#endif

enum LgSolutionErrorValue
{
    SOLUTION_MANAGER_NO_ERROR = 0,
    SOLUTION_MANAGER_NO_SUPPORTED_SOLUTION,
    SOLUTION_MANAGER_NO_ENABLED_SOLUTION,
    SOLUTION_MANAGER_PARAMETER_ERROR,
    SOLUTION_MANAGER_PARSING_ERROR,
    SOLUTION_MANAGER_MAX_ERROR_INDEX,
};

// To load configuration
LgSolutionErrorValue loadSolutionList(pbnjson::JValue &json)
{
    std::string jsonPath_      = "/etc/com.webos.service.camera/supported_solution_info.conf";
    auto obj_supportedSolution = pbnjson::JDomParser::fromFile(jsonPath_.c_str());
    if (!obj_supportedSolution.isObject())
    {
        PMLOG_ERROR(CONST_MODULE_SM, "configuration file parsing error! need to check %s",
                    jsonPath_.c_str());
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

bool isSolutionSupported(const pbnjson::JValue &json, const std::string &key)
{
    bool value = false;

    auto solutionList = json[0]; // we already know array number of solutionInfo
                                 // is 1 and never get increased, so we put 0 on
                                 // it.

    if (solutionList.hasKey(key) && solutionList[key].isBoolean() &&
        solutionList[key].asBool(value) == CONV_OK)
    {
        if (value == true)
        {
            PMLOG_ERROR(CONST_MODULE_SM, "%s is enabled", key.c_str());
        }
    }

    return value;
}

CameraSolutionManager::CameraSolutionManager(void)
{
    pbnjson::JValue obj_solutionInfo = nullptr;

    bool retValue = loadSolutionList(obj_solutionInfo);
    if (retValue != SOLUTION_MANAGER_NO_ERROR)
    {
        PMLOG_ERROR(CONST_MODULE_SM,
                    "failed to get solution list info so "
                    "can't enable solutions. (error:%d)",
                    retValue);
        return;
    }

    if (isSolutionSupported(obj_solutionInfo, SOLUTION_AUTOCONTRAST))
    {
        lstSolution_.push_back(std::make_unique<AutoContrast>());
    }

    if (isSolutionSupported(obj_solutionInfo, SOLUTION_DUMMY))
    {
        lstSolution_.push_back(std::make_unique<Dummy>());
    }

#ifdef FEATURE_LG_OPENCV_FACEDETECTION
    if (isSolutionSupported(obj_solutionInfo, SOLUTION_OPENCV_FACEDETECTION))
    {
        lstSolution_.push_back(std::make_unique<FaceDetectionOpenCV>());
    }
#endif

#ifdef FEATURE_LG_AIF_FACEDETECTION
    if (isSolutionSupported(obj_solutionInfo, SOLUTION_AIF_FACEDETECTION))
    {
        lstSolution_.push_back(std::make_unique<FaceDetectionAIF>());
    }
#endif

#ifdef FEATURE_LG_CNN_FACEDETECTION
    if (isSolutionSupported(obj_solutionInfo, SOLUTION_FACE_DETECTION_CNN))
    {
        lstSolution_.push_back(std::make_unique<FaceDetectionCNN>());
    }
#endif
}

CameraSolutionManager::~CameraSolutionManager(void) {}

void CameraSolutionManager::setEventListener(CameraSolutionEvent *pEvent)
{
    std::lock_guard<std::mutex> lg(mtxApi_);
    for (auto &i : lstSolution_)
        i->setEventListener(pEvent);
}

int32_t CameraSolutionManager::getMetaSizeHint(void)
{
    std::lock_guard<std::mutex> lg(mtxApi_);
    int32_t size{0};
    for (auto &i : lstSolution_)
        size += i->getMetaSizeHint();

    return size;
}

void CameraSolutionManager::initialize(stream_format_t streamFormat)
{
    for (auto &i : lstSolution_)
        i->initialize(streamFormat);
}

void CameraSolutionManager::release(void)
{
    for (auto &i : lstSolution_)
    {
        if (i->isEnabled())
        {
            i->release();
        }
    }
}

void CameraSolutionManager::processCapture(buffer_t frame_buffer)
{
    std::lock_guard<std::mutex> lg(mtxApi_);
    for (auto &i : lstSolution_)
    {
        if (i->isEnabled() && i->getProperty() & LG_SOLUTION_SNAPSHOT)
        {
            i->processForSnapshot(frame_buffer);
        }
    }
}

void CameraSolutionManager::processPreview(buffer_t frame_buffer)
{
    std::lock_guard<std::mutex> lg(mtxApi_);
    for (auto &i : lstSolution_)
    {
        if (i->isEnabled() && i->getProperty() & LG_SOLUTION_PREVIEW)
        {
            i->processForPreview(frame_buffer);
        }
    }
}

void CameraSolutionManager::getSupportedSolutionInfo(SolutionNames &names)
{
    PMLOG_INFO(CONST_MODULE_SM, "");

    // check supported solution list
    for (auto &i : lstSolution_)
    {
        names.push_back(i->getSolutionStr());
        PMLOG_INFO(CONST_MODULE_SM, "solution name %s", i->getSolutionStr().c_str());
    }
}

void CameraSolutionManager::getEnabledSolutionInfo(SolutionNames &names)
{
    std::lock_guard<std::mutex> lg(mtxApi_);
    PMLOG_INFO(CONST_MODULE_SM, "");

    // check supported solution list
    for (auto &i : lstSolution_)
    {
        if (i->isEnabled() == true)
        {
            names.push_back(i->getSolutionStr());
        }
        PMLOG_INFO(CONST_MODULE_SM, "solution name %s", i->getSolutionStr().c_str());
    }
}

DEVICE_RETURN_CODE_T
CameraSolutionManager::enableCameraSolution(const SolutionNames &names)
{
    PMLOG_INFO(CONST_MODULE_SM, "E\n");
    uint32_t candidateSolutionCnt = 0;
    for (auto &s : names)
    {
        for (auto &i : lstSolution_)
        {
            if (s == i->getSolutionStr())
            {
                candidateSolutionCnt++;
                PMLOG_INFO(CONST_MODULE_SM, "candidate enabled solutionName %s", s.c_str());
            }
        }
    }

    // check if the parameters from client are all valid by comparing
    // candidateSolutionCnt number and parameters number.
    if (names.size() == candidateSolutionCnt)
    {
        for (auto &s : names)
        {
            for (auto &i : lstSolution_)
            {
                if (s == i->getSolutionStr())
                {
                    i->setEnableValue(true);
                }
            }
        }
    }
    else
    {
        if (names.size() == 0)
        {
            return DEVICE_ERROR_PARAM_IS_MISSING;
        }
        return DEVICE_ERROR_WRONG_PARAM;
    }

    return DEVICE_OK;
}

DEVICE_RETURN_CODE_T
CameraSolutionManager::disableCameraSolution(const SolutionNames &names)
{
    std::lock_guard<std::mutex> lg(mtxApi_);
    PMLOG_INFO(CONST_MODULE_SM, "");
    uint32_t candidateSolutionCnt = 0;

    for (auto &s : names)
    {
        for (auto &i : lstSolution_)
        {
            if (s == i->getSolutionStr())
            {
                candidateSolutionCnt++;
                PMLOG_INFO(CONST_MODULE_SM, "candidate disabled solutionName %s", s.c_str());
            }
        }
    }
    // check if the parameters from client are all valid by comparing
    // candidateSolutionCnt number and parameters number.
    if (names.size() == candidateSolutionCnt)
    {
        for (auto &s : names)
        {
            for (auto &i : lstSolution_)
            {
                if (s == i->getSolutionStr())
                {
                    i->setEnableValue(false);
                }
            }
        }
    }
    else
    {
        if (names.size() == 0)
        {
            return DEVICE_ERROR_PARAM_IS_MISSING;
        }
        return DEVICE_ERROR_WRONG_PARAM;
    }

    return DEVICE_OK;
}
