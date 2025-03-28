#include "addon.h"
#include "camera_types.h"
#include "command_manager.h"
#include "device_manager.h"
#include "whitelist_checker.h"
#include <dlfcn.h>

AddOn::AddOn()
{
    pFeature_ = pluginFactory_.createFeature("addon");
    if (pFeature_)
    {
        service_ = new Service();

        void *pInterface = nullptr;
        pFeature_->queryInterface("addon", &pInterface);
        plugin_ = static_cast<IAddon *>(pInterface);
    }
}

AddOn::~AddOn()
{
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

void AddOn::initialize(void *lsHandle)
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
    return plugin_->isSupportedCamera(std::move(productId), std::move(vendorId));
}

bool AddOn::isAppPermission(std::string appId)
{
    if (!plugin_)
    {
        return true;
    }
    return plugin_->isAppPermission(std::move(appId));
}

bool AddOn::notifyDeviceOpened(std::string deviceKey, std::string appId, std::string appPriority)
{
    if (!plugin_)
    {
        return false;
    }
    return plugin_->notifyDeviceOpened(std::move(deviceKey), std::move(appId),
                                       std::move(appPriority));
}

void AddOn::notifySolutionEnabled(std::string deviceKey, const std::vector<std::string> &solutions)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->notifySolutionEnabled(std::move(deviceKey), solutions);
}

void AddOn::notifySolutionDisabled(std::string deviceKey, const std::vector<std::string> &solutions)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->notifySolutionDisabled(std::move(deviceKey), solutions);
}

void AddOn::notifyDeviceAdded(const void *deviceInfo)
{
    if (!plugin_)
    {
        return;
    }

    plugin_->notifyDeviceAdded(deviceInfo);
}

void AddOn::notifyDeviceRemoved(const void *deviceInfo)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->notifyDeviceRemoved(deviceInfo);
}

void AddOn::notifyDeviceListUpdated(std::string deviceType, const void *deviceList)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->notifyDeviceListUpdated(std::move(deviceType), deviceList);
}

std::vector<std::string> AddOn::getEnabledSolutionList(std::string deviceKey)
{
    if (!plugin_)
    {
        return std::vector<std::string>{};
    }
    return plugin_->getEnabledSolutionList(std::move(deviceKey));
}

int AddOn::Service::getDeviceCounts(std::string deviceType)
{
    return DeviceManager::getInstance().getDeviceCounts(std::move(deviceType));
}

bool AddOn::Service::updateDeviceList(std::string deviceType, const void *deviceList)
{
    return DeviceManager::getInstance().updateDeviceList(
        std::move(deviceType), *static_cast<const std::vector<DEVICE_LIST_T> *>(deviceList));
}
