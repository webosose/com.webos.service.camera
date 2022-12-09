#ifndef CAMERA_SERVICE_ADDON_INTERFACE_H_
#define CAMERA_SERVICE_ADDON_INTERFACE_H_

#include "camera_device_types.h"
#include <luna-service2/lunaservice.h>
#include <pbnjson.h>
#include <string>


typedef int (*DEVICE_LIST_CALLBACK)(int*, int*, int*, int*);
typedef void* EXTRA_DATA_T;

class ICameraServiceAddon
{
public:
    virtual ~ICameraServiceAddon()
    {
    }

    virtual bool hasImplementation() = 0;

    virtual void initialize(LSHandle*) = 0;
    virtual void setSubscriptionForCameraList(LSMessage &) = 0;
    virtual void setDeviceEvent(DEVICE_LIST_T*, int, DEVICE_LIST_CALLBACK) = 0;
    virtual bool setPermission(LSMessage &) = 0;
    virtual bool isSupportedCamera(std::string, std::string) = 0;
    virtual bool isAppPermission(std::string) = 0;
    virtual bool test(LSMessage &, DEVICE_LIST_CALLBACK) = 0;

    virtual EXTRA_DATA_T getDeviceExtraData(jvalue_ref) = 0;
    virtual void setDeviceExtraData(int, EXTRA_DATA_T) = 0;
    virtual void logExtraMessage(std::string) = 0;
    virtual int getDevicePort(jvalue_ref) = 0;
};

#endif /* CAMERA_SERVICE_ADDON_INTERFACE_H_ */
