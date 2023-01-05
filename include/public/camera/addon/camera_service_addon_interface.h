#ifndef CAMERA_SERVICE_ADDON_INTERFACE_H_
#define CAMERA_SERVICE_ADDON_INTERFACE_H_

#include "camera_device_types.h"
#include "camera_hal_if_types.h"
#include <luna-service2/lunaservice.h>
#include <pbnjson.h>
#include <string>
#include <vector>

struct ICameraService
{
    virtual ~ICameraService() {}
    virtual int getDeviceList(std::vector<int> &idList) { return -1; }
    virtual int getDeviceCounts(std::string type) { return 0; }
    virtual int addDevice(DEVICE_LIST_T *devList, std::string payload = "") { return 0; }
    virtual bool removeDevice(int dev_idx) { return false; }
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

    virtual bool hasImplementation() = 0;

    virtual void setCameraService(ICameraService *camera_service) = 0;

    virtual void initialize(LSHandle *lshandle)                                               = 0;
    virtual void setSubscriptionForCameraList(LSMessage &message)                             = 0;
    virtual void setDeviceEvent(DEVICE_LIST_T *devList, int count, bool resumed, bool remote) = 0;
    virtual bool setPermission(LSMessage &message)                                            = 0;
    virtual bool isSupportedCamera(std::string productId, std::string vendorId)               = 0;
    virtual bool isAppPermission(std::string appId)                                           = 0;
    virtual bool test(LSMessage &message)                                                     = 0;
    virtual bool isResumeDone()                                                               = 0;

    virtual bool toastCameraUsingPopup(int deviceid) = 0;

    virtual void logMessagePrivate(std::string privateMessage) = 0;

    virtual void attachPrivateComponentToDevice(int deviceid,
                                                const std::vector<std::string> &solutions)   = 0;
    virtual void detachPrivateComponentFromDevice(int deviceid,
                                                  const std::vector<std::string> &solutions) = 0;
    virtual void pushDevicePrivateData(int deviceid, DEVICE_LIST_T *pstList)                 = 0;
    virtual void popDevicePrivateData(int deviceid)                                          = 0;
    virtual std::vector<std::string> getDevicePrivateData(int deviceid)                      = 0;
};

#endif /* CAMERA_SERVICE_ADDON_INTERFACE_H_ */
