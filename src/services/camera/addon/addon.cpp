#include "addon.h"
#include "camera_types.h"
#include "command_manager.h"
#include "device_manager.h"
#include "whitelist_checker.h"
#include <dlfcn.h>

#define CONST_MODULE_ADDON "ADDON"

AddOn::AddOn()
{
    if (!pPluginFactory_)
    {
        pPluginFactory_ = new PluginFactory();
        pFeature_       = pPluginFactory_->createFeature("addon");

        service_ = new Service();
    }

    void *pInterface = nullptr;
    pFeature_->queryInterface("addon", &pInterface);
    plugin_ = static_cast<IAddon *>(pInterface);
}

AddOn::~AddOn()
{
    if (pPluginFactory_)
    {
        delete pPluginFactory_;
        pPluginFactory_ = nullptr;
    }
    if (service_)
    {
        delete service_;
        service_ = nullptr;
    }
}

bool AddOn::hasImplementation()
{
    if (plugin_)
        return plugin_->hasImplementation();
    return false;
}

void AddOn::initialize(LSHandle *lsHandle)
{
    if (plugin_)
    {
        plugin_->setCameraService(AddOn::service_);
        plugin_->initialize(lsHandle);
    }
}

bool AddOn::isSupportedCamera(std::string productId, std::string vendorId)
{
    if (!plugin_)
    {
        return WhitelistChecker::isSupportedCamera(productId, vendorId);
    }
    return plugin_->isSupportedCamera(productId, vendorId);
}

bool AddOn::isAppPermission(std::string appId)
{
    if (!plugin_)
    {
        return true;
    }
    return plugin_->isAppPermission(appId);
}

bool AddOn::notifyDeviceOpened(std::string deviceKey, std::string appId, std::string appPriority)
{
    if (!plugin_)
    {
        return false;
    }
    return plugin_->notifyDeviceOpened(deviceKey, appId, appPriority);
}

void AddOn::notifySolutionEnabled(std::string deviceKey, const std::vector<std::string> &solutions)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->notifySolutionEnabled(deviceKey, solutions);
}

void AddOn::notifySolutionDisabled(std::string deviceKey, const std::vector<std::string> &solutions)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->notifySolutionDisabled(deviceKey, solutions);
}

void AddOn::notifyDeviceAdded(const DEVICE_LIST_T &deviceInfo)
{
    if (!plugin_)
    {
        return;
    }

    plugin_->notifyDeviceAdded(deviceInfo);
}

void AddOn::notifyDeviceRemoved(const DEVICE_LIST_T &deviceInfo)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->notifyDeviceRemoved(deviceInfo);
}

void AddOn::notifyDeviceListUpdated(std::string deviceType,
                                    const std::vector<DEVICE_LIST_T> &deviceList)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->notifyDeviceListUpdated(deviceType, deviceList);
}

std::vector<std::string> AddOn::getEnabledSolutionList(std::string deviceKey)
{
    if (!plugin_)
    {
        std::vector<std::string> empty{};
        return empty;
    }
    return plugin_->getEnabledSolutionList(deviceKey);
}

int AddOn::Service::getDeviceCounts(std::string deviceType)
{
    return DeviceManager::getInstance().getDeviceCounts(deviceType);
}

bool AddOn::Service::updateDeviceList(std::string deviceType,
                                      const std::vector<DEVICE_LIST_T> &deviceList)
{
    return DeviceManager::getInstance().updateDeviceList(deviceType, deviceList);
}
