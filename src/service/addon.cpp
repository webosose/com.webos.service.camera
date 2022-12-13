#include "addon.h"
#include "camera_types.h"
#include "camera_solution_manager.h"
#include "command_manager.h"
#include "device_manager.h"
#include "whitelist_checker.h"
#include <dlfcn.h>


#define CONST_MODULE_ADDON "ADDON"

void *AddOn::handle_ = nullptr;
ICameraServiceAddon *AddOn::plugin_ = nullptr;
AddOn::Callback *AddOn::cb_ = nullptr;

typedef void *(*pfn_create_plugin_instance)();
typedef void *(*pfn_destroy_plugin_instance)(void*);

pfn_create_plugin_instance create_plugin_instance;
pfn_destroy_plugin_instance destroy_plugin_instance;


int AddOn::getDeviceList(int *pcamdev, int *pmicdev, int *pcamsupport, int *pmicsupport)
{
    return (int)(CommandManager::getDeviceList(
                 pcamdev, pmicdev, pcamsupport, pmicsupport));
}

int AddOn::getDeviceCounts(DEVICE_TYPE_T type)
{
    return DeviceManager::getInstance().getDeviceCounts(type);
}

int AddOn::addDevice(DEVICE_LIST_T *devList)
{
    return DeviceManager::getInstance().addDevice(devList);
}

bool AddOn::removeDevice(int dev_idx)
{
    return DeviceManager::getInstance().removeDevice(dev_idx);
}

int AddOn::addRemoteCamera(deviceInfo_t *devInfo)
{
    return DeviceManager::getInstance().addRemoteCamera(devInfo);
}

int AddOn::removeRemoteCamera(int dev_idx)
{
    return DeviceManager::getInstance().removeRemoteCamera(dev_idx);
}

bool AddOn::getCurrentDeviceInfo(std::string &productId, std::string &venderId, std::string &productName)
{
    return DeviceManager::getInstance().getCurrentDeviceInfo(productId, venderId, productName);
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
        AddOn::cb_ = new Callback();
        if (nullptr == AddOn::cb_)
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
    if (AddOn::cb_)
    {
        delete AddOn::cb_;
        AddOn::cb_ = nullptr;
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

void AddOn::setDeviceEvent(DEVICE_LIST_T *device_list, int count, bool resumed, bool remote)
{
    if (plugin_)
    {
        plugin_->setDeviceEvent(device_list, count, resumed, remote, cb_);
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
    return plugin_->test(message, cb_);
}

bool AddOn::isResumeDone()
{
    if (!plugin_)
    {
        return false;
    }
    return plugin_->isResumeDone();
}

bool AddOn::toastCameraUsingPopup()
{
    if (!plugin_)
    {
        return false;
    }
    return plugin_->toastCameraUsingPopup(cb_);
}

void AddOn::logMessagePrivate(std::string msg)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->logMessagePrivate(msg);
}

void AddOn::attachPrivateComponentToDevice(int deviceid, const std::vector<std::string> &privateStrVecData)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->attachPrivateComponentToDevice(deviceid, privateStrVecData);
}

void AddOn::detachPrivateComponentFromDevice(int deviceid, const std::vector<std::string> &privateStrVecData)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->detachPrivateComponentFromDevice(deviceid, privateStrVecData);
}

void AddOn::pushDevicePrivateData(int device_id, int dev_idx, DEVICE_TYPE_T type, DEVICE_LIST_T *pstList) 
{
    if (!plugin_)
    {
        return;
    }

    plugin_->pushDevicePrivateData(device_id, dev_idx, type, pstList, cb_); 
}

void AddOn::popDevicePrivateData(int dev_idx)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->popDevicePrivateData(dev_idx);
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

void AddOn::updateDevicePrivateHandle(int deviceid, int devicehandle)
{
    if (!plugin_)
    {
        return;
    }
    plugin_->updateDevicePrivateHandle(deviceid, devicehandle);
}


int AddOn::Callback::getDeviceList(int *pcamdev, int *pmicdev, int *pcamsupport, int *pmicsupport)
{
    return AddOn::getDeviceList(pcamdev, pmicdev, pcamsupport, pmicsupport);
}

int AddOn::Callback::getDeviceCounts(DEVICE_TYPE_T type)
{
    return AddOn::getDeviceCounts(type);
}

int AddOn::Callback::addDevice(DEVICE_LIST_T *devList)
{
    return AddOn::addDevice(devList);
}

bool AddOn::Callback::removeDevice(int dev_idx)
{
    return AddOn::removeDevice(dev_idx);
}

int AddOn::Callback::addRemoteCamera(deviceInfo_t *devInfo)
{
    return AddOn::addRemoteCamera(devInfo);
}

int AddOn::Callback::removeRemoteCamera(int dev_idx)
{
    return AddOn::removeRemoteCamera(dev_idx);
}

bool AddOn::Callback::getCurrentDeviceInfo(std::string &productId, std::string &vendorId, std::string &productName)
{
    return AddOn::getCurrentDeviceInfo(productId, vendorId, productName);
}

void AddOn::Callback::getSupportedSolutionList(std::vector<std::string> &supportedList,
                                              std::vector<std::string> &enabledList)
{
    CameraSolutionManager::getSupportedSolutionList(supportedList, enabledList);
}

