#ifndef CAMERA_SERVICE_ADDON_INTERFACE_H_
#define CAMERA_SERVICE_ADDON_INTERFACE_H_

#include "camera_device_types.h"
#include <luna-service2/lunaservice.h>
#include <pbnjson.h>
#include <string>
#include <vector>


typedef int (*DEVICE_LIST_CALLBACK)(int*, int*, int*, int*);
typedef int (*DEVICE_COUNT_CALLBACK)(DEVICE_TYPE_T);
typedef int (*DEVICE_ADD_CALLBACK)(DEVICE_LIST_T*);
typedef bool (*DEVICE_REMOVE_CALLBACK)(int);
typedef int (*DEVICE_REMOTE_ADD_CALLBACK)(deviceInfo_t*);
typedef int (*DEVICE_REMOTE_REMOVE_CALLBACK)(int);
typedef bool (*DEVICE_CURRENT_INFO_CALLBACK)(std::string&, std::string&, std::string&);
typedef void (*DEVICE_PRIVATE_CALLBACK)(std::vector<std::string>&, std::vector<std::string>&);

struct DeviceEventCallback
{
    DEVICE_LIST_CALLBACK            getDeviceList;
    DEVICE_COUNT_CALLBACK           getDeviceCounts;
    DEVICE_ADD_CALLBACK             addDevice;
    DEVICE_REMOVE_CALLBACK          removeDevice;
    DEVICE_REMOTE_ADD_CALLBACK      addRemoteCamera;
    DEVICE_REMOTE_REMOVE_CALLBACK   removeRemoteCamera;
    DEVICE_CURRENT_INFO_CALLBACK    getCurrentDeviceInfo;
    DEVICE_PRIVATE_CALLBACK         getSupportedSolutionList;
};


class ICameraServiceAddon
{
public:
    virtual ~ICameraServiceAddon()
    {
    }

    virtual bool hasImplementation() = 0;

    virtual void initialize(LSHandle*) = 0;
    virtual void setSubscriptionForCameraList(LSMessage &) = 0;
    virtual void setDeviceEvent(DEVICE_LIST_T*, int, bool, bool, DeviceEventCallback*) = 0;
    virtual bool setPermission(LSMessage &) = 0;
    virtual bool isSupportedCamera(std::string, std::string) = 0;
    virtual bool isAppPermission(std::string) = 0;
    virtual bool test(LSMessage &, DeviceEventCallback*) = 0;
    virtual bool isResumeDone() = 0;

    virtual bool toastCameraUsingPopup(DeviceEventCallback*) = 0;

    virtual void logMessagePrivate(std::string) = 0;

    virtual void attachPrivateComponentToDevice(int, const std::vector<std::string>&) = 0;
    virtual void detachPrivateComponentFromDevice(int, const std::vector<std::string>&) = 0;
    virtual void pushDevicePrivateData(int, int, DEVICE_TYPE_T, DEVICE_LIST_T*, DeviceEventCallback*) = 0;
    virtual void popDevicePrivateData(int) = 0;
    virtual std::vector<std::string> getDevicePrivateData(int) = 0;
    virtual void updateDevicePrivateHandle(int, int) = 0;
};

#endif /* CAMERA_SERVICE_ADDON_INTERFACE_H_ */
