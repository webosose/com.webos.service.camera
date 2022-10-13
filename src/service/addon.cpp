#include "addon.h"
#include "camera_types.h"
#include "whitelist_checker.h"
#include <dlfcn.h>
#include "command_manager.h"


#define CONST_MODULE_ADDON "ADDON"

void *AddOn::handle_ = nullptr;
ICameraServiceAddon *AddOn::plugin_ = nullptr;
EXTRA_DATA_T AddOn::extra_data_ = nullptr;


typedef void *(*pfn_create_plugin_instance)();
typedef void *(*pfn_destroy_plugin_instance)(void*);

pfn_create_plugin_instance create_plugin_instance;
pfn_destroy_plugin_instance destroy_plugin_instance;


int AddOn::getDeviceList(int *pcamdev, int *pmicdev, int *pcamsupport, int *pmicsupport)
{
    return (int)(CommandManager::getDeviceList(
			    pcamdev, pmicdev, pcamsupport, pmicsupport));
}

EXTRA_DATA_T AddOn::currentExtraData()
{
    return AddOn::extra_data_;
}

void AddOn::open()
{
    handle_ = dlopen("libcamera_service_addon.so", RTLD_LAZY);
    if (!handle_)
    {
        PMLOG_INFO(CONST_MODULE_ADDON, "%s", dlerror());
        return;
    }
    create_plugin_instance = (pfn_create_plugin_instance)dlsym(handle_, "create_camera_service_addon");
    if (!create_plugin_instance)
    {
        PMLOG_INFO(CONST_MODULE_ADDON, "%s", dlerror());
        dlclose(handle_);
        handle_ = nullptr;
        return;
    }
    destroy_plugin_instance = (pfn_destroy_plugin_instance)dlsym(handle_, "destroy_camera_service_addon");
    if (!destroy_plugin_instance)
    {
        PMLOG_INFO(CONST_MODULE_ADDON, "%s", dlerror());
        dlclose(handle_);
        handle_ = nullptr;
        return;
    }

    plugin_ = (ICameraServiceAddon*)create_plugin_instance();
    if (plugin_)
    {
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

void AddOn::setDeviceEvent(DEVICE_LIST_T *device_list, int count)
{
    if (plugin_)
    {
        plugin_->setDeviceEvent(device_list, count, AddOn::getDeviceList);
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
    return plugin_->test(message, AddOn::getDeviceList);
}

void AddOn::getDeviceExtraData(jvalue_ref &jin_array_obj)
{
    if (plugin_)
    {
	    AddOn::extra_data_ = plugin_->getDeviceExtraData(jin_array_obj);
    }
}

void AddOn::setDeviceExtraData(int index, EXTRA_DATA_T extra)
{
    if (plugin_)
    {
        plugin_->setDeviceExtraData(index, extra);
    }
}

void AddOn::logExtraMessage(std::string str_extra_logmsg)
{
    if (plugin_)
    {
        plugin_->logExtraMessage(str_extra_logmsg);
    }
}

int AddOn::getDevicePort(jvalue_ref &jin_array_obj)
{
    if (!plugin_)
    {
        return 0;
    }
    return plugin_->getDevicePort(jin_array_obj);
}
