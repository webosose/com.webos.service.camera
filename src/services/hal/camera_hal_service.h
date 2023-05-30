/**
 * Copyright(c) 2023 by LG Electronics Inc.
 * CTO, LG Electronics., Seoul, Korea
 *
 * All rights reserved. No part of this work may be reproduced,
 * stored in a retrieval system, or transmitted by any means without
 * prior written Permission of LG Electronics Inc.

 * @Filename    camera_hal_service.h
 * @contact     Multimedia_TP-Camera@lge.com
 *
 * Description  Camera HAL service
 *
 */

#pragma once

#include "luna-service2/lunaservice.hpp"
#include <glib.h>

class DeviceControl;
class CameraHalService : public LS::Handle
{
    using mainloop          = std::unique_ptr<GMainLoop, void (*)(GMainLoop *)>;
    mainloop main_loop_ptr_ = {g_main_loop_new(nullptr, false), g_main_loop_unref};

    std::unique_ptr<DeviceControl> pDeviceControl;

public:
    CameraHalService(const char *service_name);

    CameraHalService(CameraHalService const &)            = delete;
    CameraHalService(CameraHalService &&)                 = delete;
    CameraHalService &operator=(CameraHalService const &) = delete;
    CameraHalService &operator=(CameraHalService &&)      = delete;

    bool createHal(LSMessage &message);
    bool destroyHal(LSMessage &message);
    bool open(LSMessage &message);
    bool close(LSMessage &message);
    bool startPreview(LSMessage &message);
    bool stopPreview(LSMessage &message);
    bool startCapture(LSMessage &message);
    bool stopCapture(LSMessage &message);
    bool getDeviceProperty(LSMessage &message);
    bool setDeviceProperty(LSMessage &message);
    bool setFormat(LSMessage &message);
    bool getFormat(LSMessage &message);
    bool getDeviceInfo(LSMessage &message);
    bool getFd(LSMessage &message);
    bool registerClient(LSMessage &message);
    bool unregisterClient(LSMessage &message);
    bool isRegisteredClient(LSMessage &message);
    bool requestPreviewCancel(LSMessage &message);
    bool getSupportedCameraSolutionInfo(LSMessage &message);
    bool getEnabledCameraSolutionInfo(LSMessage &message);
    bool enableCameraSolution(LSMessage &message);
    bool disableCameraSolution(LSMessage &message);
    bool subscribe(LSMessage &);
};
