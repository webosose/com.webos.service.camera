/**
 * Copyright(c) 2023 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    camera_solution_service.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera Solution service
 *
 */

#pragma once

#include "luna-service2/lunaservice.hpp"
#include "plugin_factory.hpp"
#include <glib.h>

class CameraSolution;
class CameraSharedMemoryEx;
class CameraSolutionService : public LS::Handle
{
    using mainloop          = std::unique_ptr<GMainLoop, void (*)(GMainLoop *)>;
    mainloop main_loop_ptr_ = {g_main_loop_new(nullptr, false), g_main_loop_unref};

    PluginFactory pluginFactory_;
    IFeaturePtr pFeature_;
    ISolution *pSolution_{nullptr};

public:
    CameraSolutionService(const char *service_name);

    CameraSolutionService(CameraSolutionService const &)            = delete;
    CameraSolutionService(CameraSolutionService &&)                 = delete;
    CameraSolutionService &operator=(CameraSolutionService const &) = delete;
    CameraSolutionService &operator=(CameraSolutionService &&)      = delete;

    bool create(LSMessage &message);
    bool init(LSMessage &message);
    bool enable(LSMessage &message);
    bool release(LSMessage &message);
    bool subscribe(LSMessage &);

private:
    std::unique_ptr<CameraSharedMemoryEx> camShmem_;
};

std::string parseSolutionServiceName(int argc, char *argv[]) noexcept;
