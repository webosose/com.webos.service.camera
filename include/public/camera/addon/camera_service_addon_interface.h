#ifndef CAMERA_SERVICE_ADDON_INTERFACE_H_
#define CAMERA_SERVICE_ADDON_INTERFACE_H_

#include "camera_device_types.h"
#include "camera_hal_if_cpp_types.h"
#include <luna-service2/lunaservice.h>
#include <pbnjson.h>
#include <string>
#include <vector>

struct ICameraService
{
    virtual ~ICameraService() {}
    virtual int getDeviceList(std::vector<int> &idList) { return -1; }
    virtual int getDeviceCounts(std::string type) { return 0; }
    virtual bool updateDeviceList(std::string deviceType,
                                  const std::vector<DEVICE_LIST_T> &deviceList)
    {
        return false;
    }
    virtual int getInfo(int deviceid, camera_device_info_t *p_info) { return -1; }
    virtual void getSupportedSolutionList(std::vector<std::string> &supportedList,
                                          std::vector<std::string> &enabledList)
    {
    }
};

class ICameraServiceAddon
{
public:
    virtual ~ICameraServiceAddon() {}

    virtual bool hasImplementation()                                                   = 0;
    virtual void setCameraService(ICameraService *camera_service)                      = 0;
    virtual void initialize(LSHandle *lshandle)                                        = 0;
    virtual void setSubscriptionForCameraList(LSMessage &message)                      = 0;
    virtual bool isSupportedCamera(std::string productID, std::string vendorID)        = 0;
    virtual bool isAppPermission(std::string appId)                                    = 0;
    virtual void notifyDeviceAdded(const DEVICE_LIST_T &deviceInfo)                    = 0;
    virtual void notifyDeviceRemoved(const DEVICE_LIST_T &deviceInfo)                  = 0;
    virtual void notifyDeviceListUpdated(std::string deviceType,
                                         const std::vector<DEVICE_LIST_T> &deviceList) = 0;
    virtual bool notifyDeviceOpened(std::string deviceKey, std::string appId,
                                    std::string appPriority)                           = 0;
    virtual void notifySolutionEnabled(std::string deviceKey,
                                       const std::vector<std::string> &solutions)      = 0;
    virtual void notifySolutionDisabled(std::string deviceKey,
                                        const std::vector<std::string> &solutions)     = 0;
    virtual std::vector<std::string> getEnabledSolutionList(std::string deviceKey)     = 0;
};

#endif /* CAMERA_SERVICE_ADDON_INTERFACE_H_ */
