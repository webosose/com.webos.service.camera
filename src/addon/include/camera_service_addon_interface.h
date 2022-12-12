#ifndef CAMERA_SERVICE_ADDON_INTERFACE_H_
#define CAMERA_SERVICE_ADDON_INTERFACE_H_

#include <luna-service2/lunaservice.h>
#include <pbnjson.h>
#include <string>
#include "camera_device_types.h"


typedef int (*DEVICE_LIST_CALLBACK)(int*, int*, int*, int*);
typedef int (*DEVICE_COUNT_CALLBACK)(DEVICE_TYPE_T);
typedef int (*DEVICE_ADD_CALLBACK)(DEVICE_LIST_T*);
typedef bool (*DEVICE_REMOVE_CALLBACK)(int);
typedef int (*DEVICE_REMOTE_ADD_CALLBACK)(deviceInfo_t*);
typedef int (*DEVICE_REMOTE_REMOVE_CALLBACK)(int);
typedef bool (*DEVICE_CURRENT_INFO_CALLBACK)(std::string&, std::string&, std::string&);


class ICameraServiceAddon
{
public:
    virtual ~ICameraServiceAddon()
    {
    }

    virtual bool hasImplementation() = 0;

    virtual void initialize(LSHandle*) = 0;
    virtual void setSubscriptionForCameraList(LSMessage &) = 0;
    virtual void setDeviceEvent(DEVICE_LIST_T*, int, bool, bool, 
                                DEVICE_LIST_CALLBACK) = 0;
    virtual bool setPermission(LSMessage &) = 0;
    virtual bool isSupportedCamera(std::string, std::string) = 0;
    virtual bool isAppPermission(std::string) = 0;
    virtual bool test(LSMessage &, DEVICE_COUNT_CALLBACK,
                                   DEVICE_ADD_CALLBACK,
                                   DEVICE_REMOVE_CALLBACK,
                                   DEVICE_REMOTE_ADD_CALLBACK,
                                   DEVICE_REMOTE_REMOVE_CALLBACK,
                                   DEVICE_LIST_CALLBACK) = 0;
    virtual bool isResumeDone() = 0;

    virtual bool toastCameraUsingPopup(DEVICE_CURRENT_INFO_CALLBACK) = 0;

    virtual void logMessagePrivate(std::string) = 0;
};

#endif /* CAMERA_SERVICE_ADDON_INTERFACE_H_ */
