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
#include "plugin_factory.hpp"

CameraSolutionManager::CameraSolutionManager(void)
{
#ifdef FIX_ME // not support sync solution
    std::vector<std::string> list, enabledList;
    getSupportedSolutionList(list, enabledList);
    PMLOG_INFO(CONST_MODULE_SM, "solution list count %zd", list.size());


    if (std::find(list.begin(), list.end(), SOLUTION_AUTOCONTRAST) != list.end())
        lstSolution_.push_back(std::make_unique<AutoContrast>());

    if (std::find(list.begin(), list.end(), SOLUTION_DUMMY) != list.end())
        lstSolution_.push_back(std::make_unique<Dummy>());
#endif
    PluginFactory factory;
    std::vector<std::string> list = factory.getFeatureList("SOLUTION");
    for (auto &s : list)
    {
        lstSolution_.push_back(std::make_unique<CameraSolutionProxy>(s));
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

void CameraSolutionManager::initialize(stream_format_t streamFormat, int shmKey, LSHandle *sh)
{
    for (auto &i : lstSolution_)
        i->initialize(streamFormat, shmKey, sh);
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
