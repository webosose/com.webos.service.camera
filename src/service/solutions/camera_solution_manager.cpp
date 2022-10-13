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
#include "camera_solution.h"
#include "camera_types.h"

#ifdef FEATURE_LG_AUTOCONTRAST
#include "auto_contrast/auto_contrast.hpp"
#endif

#ifdef FEATURE_LG_DUMMY
#include "dummy/dummy.hpp"
#endif

#ifdef FEATURE_LG_AIF_FACEDETECTION
#include "face_detection_aif/face_detection_aif.hpp"
#endif

#include <pbnjson.hpp>

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

    json = obj_supportedSolution;
    return SOLUTION_MANAGER_NO_ERROR;
}

solution_params_t getSolutionInfo(const pbnjson::JValue &json, const std::string &key)
{

    PMLOG_INFO(CONST_MODULE_SM, "E");

    solution_params_t solutionParams_{"", "", "", false, false};

    if (!json.isObject())
    {
        PMLOG_ERROR(CONST_MODULE_SM, "error to get solution list so return false");
        return solutionParams_;
    }

    // check cameraWhilteList field
    if (!json.hasKey(CONST_PARAM_NAME_SOLUTION))
    {
        PMLOG_ERROR(CONST_MODULE_SM, "Can't find %s field. need to check it!",
                    CONST_PARAM_NAME_SOLUTION);
        return solutionParams_;
    }

    // get the whitelist from cameraWhilteList field
    auto solutionList = json[CONST_PARAM_NAME_SOLUTION];
    for (int idx = 0; idx < solutionList.arraySize(); idx++)
    {

        auto slist               = solutionList[idx];
        std::string solutionName = slist["name"].asString().c_str();

        if (solutionName == key)
        {
            bool support  = false;
            bool metadata = false;

            auto sParams = slist["params"];

            solutionParams_.name = solutionName;

            if (sParams.hasKey("support"))
            {
                sParams["support"].asBool(support);
                solutionParams_.support = support;
            }

            if (sParams.hasKey("type"))
            {
                solutionParams_.type = sParams["type"].asString().c_str();
            }

            if (sParams.hasKey("algorithm"))
            {
                solutionParams_.algorithm = sParams["algorithm"].asString().c_str();
            }

            if (sParams.hasKey("metadata"))
            {
                sParams["metadata"].asBool(metadata);
                solutionParams_.metadata = metadata;
            }

            PMLOG_INFO(CONST_MODULE_SM, "solution Info ==========");
            PMLOG_INFO(CONST_MODULE_SM, "name: %s", solutionParams_.name.c_str());
            PMLOG_INFO(CONST_MODULE_SM, "support: %d", solutionParams_.support);
            PMLOG_INFO(CONST_MODULE_SM, "algorithm: %s", solutionParams_.algorithm.c_str());
            PMLOG_INFO(CONST_MODULE_SM, "type: %s", solutionParams_.type.c_str());
            PMLOG_INFO(CONST_MODULE_SM, "metadata: %d", solutionParams_.metadata);
            break;
        }
    }

    return solutionParams_;
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

#ifdef FEATURE_LG_AUTOCONTRAST
    solution_params_t solutionParams_ = getSolutionInfo(obj_solutionInfo, SOLUTION_AUTOCONTRAST);
    if (solutionParams_.support == true)
    {
        lstSolution_.push_back(std::make_unique<AutoContrast>());
        auto &i = lstSolution_.back();
        i->getSolutionParams(solutionParams_);
    }
#endif

#ifdef FEATURE_LG_DUMMY
    solutionParams_ = getSolutionInfo(obj_solutionInfo, SOLUTION_DUMMY);
    if (solutionParams_.support == true)
    {
        lstSolution_.push_back(std::make_unique<Dummy>());
        auto &i = lstSolution_.back();
        i->getSolutionParams(solutionParams_);
    }
#endif

#ifdef FEATURE_LG_AIF_FACEDETECTION
    solutionParams_ = getSolutionInfo(obj_solutionInfo, SOLUTION_FACEDETECTION_AIF);
    if (solutionParams_.support == true)
    {
        lstSolution_.push_back(std::make_unique<FaceDetectionAIF>());
        auto &i = lstSolution_.back();
        i->getSolutionParams(solutionParams_);
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
    {
        size += i->getMetaSizeHint();
    }

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
        i->release();
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
                PMLOG_INFO(CONST_MODULE_SM, "candidate enabled solutionName %s", s.c_str());
            }
        }
    }

    // check if the parameters from client are all valid by comparing candidateSolutionCnt number
    // and parameters number.
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
    // check if the parameters from client are all valid by comparing candidateSolutionCnt number
    // and parameters number.
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
