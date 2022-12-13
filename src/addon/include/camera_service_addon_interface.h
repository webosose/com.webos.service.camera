#ifndef CAMERA_SERVICE_ADDON_INTERFACE_H_
#define CAMERA_SERVICE_ADDON_INTERFACE_H_

#include "camera_device_types.h"
#include <luna-service2/lunaservice.h>
#include <pbnjson.h>
#include <string>
#include <vector>


struct DeviceEventCallback
{
    virtual ~DeviceEventCallback()
    {
    }
    virtual int getDeviceList(int *pcamdev, int *pmicdev, int *pcamsupport, int *pmicsupport)
    {
        return -1;
    }
    virtual int getDeviceCounts(DEVICE_TYPE_T type)
    { 
        return 0; 
    }
    virtual int addDevice(DEVICE_LIST_T *devList)
    {
        return 0;
    }
    virtual bool removeDevice(int dev_idx)
    {
        return false;
    }
    virtual int addRemoteCamera(deviceInfo_t *devInfo)
    {
        return 0;
    }
    virtual int removeRemoteCamera(int dev_idx)
    {
        return 0;
    }
    virtual bool getCurrentDeviceInfo(std::string &productId, 
                                      std::string &vendorId, 
                                      std::string &productName)
    {
        return false;
    }
    virtual void getSupportedSolutionList(std::vector<std::string> &supportedList, 
                                          std::vector<std::string> &enabledList)
    {
    }
}; 


class ICameraServiceAddon
{
public:
    virtual ~ICameraServiceAddon()
    {
    }

    virtual bool hasImplementation() = 0;

    virtual void initialize(LSHandle *lshandle) = 0;
    virtual void setSubscriptionForCameraList(LSMessage &message) = 0;
    virtual void setDeviceEvent(DEVICE_LIST_T *devList, int count, bool resumed, bool remote, DeviceEventCallback *cb) = 0;
    virtual bool setPermission(LSMessage &message) = 0;
    virtual bool isSupportedCamera(std::string productId, std::string vendorId) = 0;
    virtual bool isAppPermission(std::string appId) = 0;
    virtual bool test(LSMessage &message, DeviceEventCallback *cb) = 0;
    virtual bool isResumeDone() = 0;

    virtual bool toastCameraUsingPopup(DeviceEventCallback *cb) = 0;

    virtual void logMessagePrivate(std::string privateMessage) = 0;

    virtual void attachPrivateComponentToDevice(int deviceid, const std::vector<std::string> &solutions) = 0;
    virtual void detachPrivateComponentFromDevice(int deviceid, const std::vector<std::string> &solutions) = 0;
    virtual void pushDevicePrivateData(int device_id, int dev_idx, DEVICE_TYPE_T type, DEVICE_LIST_T *pstList, DeviceEventCallback *cb) = 0;
    virtual void popDevicePrivateData(int dev_idx) = 0;
    virtual std::vector<std::string> getDevicePrivateData(int deviceid) = 0;
    virtual void updateDevicePrivateHandle(int deviceid, int devicehandle) = 0;
};

#endif /* CAMERA_SERVICE_ADDON_INTERFACE_H_ */
