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
#include "CameraSolutionProxy.h"
#include "camera_types.h"
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

struct SolutionInfo
{
    bool support_; // support or not
    bool enable_;  // default enable or disable
    std::string model_;
};

class SupportedSolution
{
private:
    SupportedSolution() { parseSolutionInfo(); };
    SupportedSolution(const SupportedSolution &){};
    ~SupportedSolution(){};
    void parseSolutionInfo(void);
    std::map<std::string, std::shared_ptr<SolutionInfo>> solutions_;

public:
    static SupportedSolution &getInstance()
    {
        static SupportedSolution instance;
        return instance;
    };
    bool getSolutionInfo(std::string name, std::shared_ptr<SolutionInfo> &info)
    {
        if (!solutions_.count(name))
            return false;
        info = solutions_[name];
        return true;
    };
};

void SupportedSolution::parseSolutionInfo(void)
{
    std::string jsonPath_ = "/etc/com.webos.service.camera/supported_solution_info.conf";
    auto obj_conf         = pbnjson::JDomParser::fromFile(jsonPath_.c_str());
    if (!obj_conf.isObject())
    {
        PMLOG_ERROR(CONST_MODULE_SM, "configuration file parsing error! need to check %s",
                    jsonPath_.c_str());
        return;
    }

    // check solution_info field
    if (!obj_conf.hasKey("solutionInfo"))
    {
        PMLOG_ERROR(CONST_MODULE_SM, "Can't find solutionInfo field. need to check it!");
        return;
    }

    auto obj_solutionInfo = obj_conf["solutionInfo"];
    size_t count          = obj_solutionInfo.arraySize();
    for (size_t i = 0; i < count; i++)
    {
        if (obj_solutionInfo[i].hasKey("name"))
        {
            std::string name   = obj_solutionInfo[i]["name"].asString();
            SolutionInfo *info = new SolutionInfo{};
            if (obj_solutionInfo[i].hasKey("params"))
            {
                auto obj_params = obj_solutionInfo[i]["params"];
                if (obj_params.hasKey("support") && obj_params["support"].isBoolean())
                {
                    info->support_ = obj_params["support"].asBool();
                }
                if (obj_params.hasKey("enable") && obj_params["enable"].isBoolean())
                {
                    info->enable_ = obj_params["enable"].asBool();
                }
                if (obj_params.hasKey("model") && obj_params["model"].isString())
                {
                    info->model_ = obj_params["model"].asString();
                }
            }
            PMLOG_INFO(CONST_MODULE_SM, "supportedSolutionInfo [%s, %d, %d]", name.c_str(),
                       info->support_, info->enable_);
            solutions_.insert(std::make_pair(name, info));
        }
    }
}

void CameraSolutionManager::getSupportedSolutionList(std::vector<std::string> &supportedList,
                                                     std::vector<std::string> &enabledList)
{
    std::shared_ptr<SolutionInfo> info;
    if (SupportedSolution::getInstance().getSolutionInfo(SOLUTION_DUMMY, info))
    {
        if (info && info->support_)
            supportedList.push_back(SOLUTION_DUMMY);
        if (info && info->enable_)
            enabledList.push_back(SOLUTION_DUMMY);
    }
    if (SupportedSolution::getInstance().getSolutionInfo(SOLUTION_AUTOCONTRAST, info))
    {
        if (info && info->support_)
            supportedList.push_back(SOLUTION_AUTOCONTRAST);
        if (info && info->enable_)
            enabledList.push_back(SOLUTION_AUTOCONTRAST);
    }
    if (SupportedSolution::getInstance().getSolutionInfo(SOLUTION_FACEDETECTION, info))
    {
        if (info && info->support_)
            supportedList.push_back(SOLUTION_FACEDETECTION);
        if (info && info->enable_)
            enabledList.push_back(SOLUTION_FACEDETECTION);
    }
}

CameraSolutionManager::CameraSolutionManager(void)
{
    std::vector<std::string> list, enabledList;
    getSupportedSolutionList(list, enabledList);
    PMLOG_INFO(CONST_MODULE_SM, "solution list count %zd", list.size());

#ifdef FIX_ME // not support sync solution
    if (std::find(list.begin(), list.end(), SOLUTION_AUTOCONTRAST) != list.end())
        lstSolution_.push_back(std::make_unique<AutoContrast>());

    if (std::find(list.begin(), list.end(), SOLUTION_DUMMY) != list.end())
        lstSolution_.push_back(std::make_unique<Dummy>());
#endif
    if (std::find(list.begin(), list.end(), SOLUTION_FACEDETECTION) != list.end())
    {
        std::shared_ptr<SolutionInfo> info;
        bool ret = SupportedSolution::getInstance().getSolutionInfo(SOLUTION_FACEDETECTION, info);
        if (ret && info->model_ == FACEDETECTION_MODEL_AIF) // default model
        {
            lstSolution_.push_back(std::make_unique<CameraSolutionProxy>(SOLUTION_FACEDETECTION));
        }
    }
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

void CameraSolutionManager::initialize(stream_format_t streamFormat, int shmKey)
{
    for (auto &i : lstSolution_)
        i->initialize(streamFormat, shmKey);
}

void CameraSolutionManager::release(void)
{
    for (auto &i : lstSolution_)
        i->release();
}

void CameraSolutionManager::processCapture(buffer_t frame_buffer)
{
#ifdef FIX_ME // not support sync solution
    std::lock_guard<std::mutex> lg(mtxApi_);
    for (auto &i : lstSolution_)
    {
        if (i->isEnabled() && i->getProperty() & LG_SOLUTION_SNAPSHOT)
        {
            i->processForSnapshot(frame_buffer);
        }
    }
#endif
}

void CameraSolutionManager::processPreview(buffer_t frame_buffer)
{
#ifdef FIX_ME // not support sync solution
    std::lock_guard<std::mutex> lg(mtxApi_);
    for (auto &i : lstSolution_)
    {
        if (i->isEnabled() && i->getProperty() & LG_SOLUTION_PREVIEW)
        {
            i->processForPreview(frame_buffer);
        }
    }
#endif
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

DEVICE_RETURN_CODE_T CameraSolutionManager::enableCameraSolution(const SolutionNames &names)
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

DEVICE_RETURN_CODE_T CameraSolutionManager::disableCameraSolution(const SolutionNames &names)
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
