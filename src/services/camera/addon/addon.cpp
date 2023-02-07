#include "addon.h"
#ifdef FIX_ME
#include "camera_solution_manager.h"
#endif
#include "camera_types.h"
#include "command_manager.h"
#include "device_manager.h"
#include "whitelist_checker.h"
#include <dlfcn.h>

#define CONST_MODULE_ADDON "ADDON"

void *AddOn::handle_                = nullptr;
ICameraServiceAddon *AddOn::plugin_ = nullptr;
AddOn::Service *AddOn::service_     = nullptr;

typedef void *(*pfn_create_plugin_instance)();
typedef void *(*pfn_destroy_plugin_instance)(void *);

pfn_create_plugin_instance create_plugin_instance;
pfn_destroy_plugin_instance destroy_plugin_instance;

int AddOn::getDeviceCounts(std::string type)
{
    return DeviceManager::getInstance().getDeviceCounts(type);
}

bool AddOn::updateDeviceList(std::string deviceType, const std::vector<DEVICE_LIST_T> &deviceList)
{
    return DeviceManager::getInstance().updateDeviceList(deviceType, deviceList);
};

void AddOn::open()
{
    handle_ = dlopen("libcamera_service_addon.so", RTLD_LAZY);
    if (!handle_)
    {
        PMLOG_INFO(CONST_MODULE_ADDON, "%s", dlerror());
        return;
    }
    create_plugin_instance =
        (pfn_create_plugin_instance)dlsym(handle_, "create_camera_service_addon");
    if (!create_plugin_instance)
    {
        PMLOG_INFO(CONST_MODULE_ADDON, "%s", dlerror());
        dlclose(handle_);
        handle_ = nullptr;
        return;
    }
    destroy_plugin_instance =
        (pfn_destroy_plugin_instance)dlsym(handle_, "destroy_camera_service_addon");
    if (!destroy_plugin_instance)
    {
        PMLOG_INFO(CONST_MODULE_ADDON, "%s", dlerror());
        dlclose(handle_);
        handle_ = nullptr;
        return;
    }

    plugin_ = (ICameraServiceAddon *)create_plugin_instance();
    if (plugin_)
    {
        AddOn::service_ = new Service();
        if (nullptr == AddOn::service_)
        {
            PMLOG_INFO(CONST_MODULE_ADDON, "DeviceEventCallback object malloc failed.");
            close();
            PMLOG_INFO(CONST_MODULE_ADDON, "module instance closed.\n");
        }
        PMLOG_INFO(CONST_MODULE_ADDON, "module instance created and ready : OK!");
    }
    else
    {
        PMLOG_INFO(CONST_MODULE_ADDON, "module instance creation failed : FAIL!");
    }
}

void AddOn::close()
{
    if (plugin_)
    {
        destroy_plugin_instance(plugin_);
        plugin_ = nullptr;
    }
    if (handle_)
    {
        dlclose(handle_);
        handle_ = nullptr;
    }
    if (AddOn::service_)
    {
        delete AddOn::service_;
        AddOn::service_ = nullptr;
    }
}

bool AddOn::hasImplementation()
{
    if (!handle_)
    {
        return false;
    }
    if (!plugin_)
    {
        return false;
    }
    return plugin_->hasImplementation();
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
    return AddOn::getDeviceCounts(deviceType);
}

bool AddOn::Service::updateDeviceList(std::string deviceType,
                                      const std::vector<DEVICE_LIST_T> &deviceList)
{
    return AddOn::updateDeviceList(deviceType, deviceList);
}
