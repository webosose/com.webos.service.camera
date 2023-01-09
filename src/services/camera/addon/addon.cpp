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

int AddOn::getDeviceList(std::vector<int> &idList)
{
    return (int)(CommandManager::getDeviceList(idList));
}

int AddOn::getDeviceCounts(std::string type)
{
    return DeviceManager::getInstance().getDeviceCounts(type);
}

int AddOn::addDevice(DEVICE_LIST_T *devList, std::string payload)
{
    return DeviceManager::getInstance().addDevice(devList, payload);
}

bool AddOn::removeDevice(int dev_idx) { return DeviceManager::getInstance().removeDevice(dev_idx); }

int AddOn::getInfo(int deviceid, camera_device_info_t *p_info)
{
    return DeviceManager::getInstance().getInfo(deviceid, p_info);
}

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

void AddOn::initialize(LSHandle *ls_handle)
{
    if (plugin_)
    {
        plugin_->setCameraService(AddOn::service_);
        plugin_->initialize(ls_handle);
    }
}

void AddOn::setSubscriptionForCameraList(LSMessage &message)
{
    if (plugin_)
    {
        plugin_->setSubscriptionForCameraList(message);
    }
}

void AddOn::setDeviceEvent(DEVICE_LIST_T *device_list, int count, bool resumed, bool remote)
{
    if (plugin_)
    {
        plugin_->setDeviceEvent(device_list, count, resumed, remote);
    }
}

bool AddOn::setPermission(LSMessage &message)
{
    if (!plugin_)
    {
        return true;
    }
    return plugin_->setPermission(message);
}

bool AddOn::isSupportedCamera(std::string str_productId, std::string str_vendorId)
{
    if (!plugin_)
    {
        return WhitelistChecker::isSupportedCamera(str_productId, str_vendorId);
    }
    return plugin_->isSupportedCamera(str_productId, str_vendorId);
}

bool AddOn::isAppPermission(std::string appId)
{
    if (!plugin_)
    {
        return true;
    }
    return plugin_->isAppPermission(appId);
}

bool AddOn::test(LSMessage &message)
{
    if (!plugin_)
    {
        return true;
    }
    return plugin_->test(message);
}

bool AddOn::isResumeDone()
{
    if (!plugin_)
    {
        return false;
    }
    return plugin_->isResumeDone();
}

bool AddOn::toastCameraUsingPopup(int deviceid)
{
    if (!plugin_)
    {
        return false;
    }
    return plugin_->toastCameraUsingPopup(deviceid);
}

void AddOn::logMessagePrivate(std::string msg)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->logMessagePrivate(msg);
}

void AddOn::attachPrivateComponentToDevice(int deviceid,
                                           const std::vector<std::string> &privateStrVecData)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->attachPrivateComponentToDevice(deviceid, privateStrVecData);
}

void AddOn::detachPrivateComponentFromDevice(int deviceid,
                                             const std::vector<std::string> &privateStrVecData)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->detachPrivateComponentFromDevice(deviceid, privateStrVecData);
}

void AddOn::pushDevicePrivateData(int deviceid, DEVICE_LIST_T *pstList)
{
    if (!plugin_)
    {
        return;
    }

    plugin_->pushDevicePrivateData(deviceid, pstList);
}

void AddOn::popDevicePrivateData(int deviceid)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->popDevicePrivateData(deviceid);
}

std::vector<std::string> AddOn::getDevicePrivateData(int deviceid)
{
    if (!plugin_)
    {
        std::vector<std::string> empty{};
        return empty;
    }
    return plugin_->getDevicePrivateData(deviceid);
}

int AddOn::Service::getDeviceList(std::vector<int> &idList) { return AddOn::getDeviceList(idList); }

int AddOn::Service::getDeviceCounts(std::string type) { return AddOn::getDeviceCounts(type); }

int AddOn::Service::addDevice(DEVICE_LIST_T *devList, std::string payload)
{
    return AddOn::addDevice(devList, payload);
}

bool AddOn::Service::removeDevice(int dev_idx) { return AddOn::removeDevice(dev_idx); }

int AddOn::Service::getInfo(int deviceid, camera_device_info_t *p_info)
{
    return AddOn::getInfo(deviceid, p_info);
}

void AddOn::Service::getSupportedSolutionList(std::vector<std::string> &supportedList,
                                              std::vector<std::string> &enabledList)
{
#ifdef FIX_ME
    CameraSolutionManager::getSupportedSolutionList(supportedList, enabledList);
#endif
}
