#ifndef ADDON_H_
#define ADDON_H_

#include "camera_service_addon_interface.h"

class AddOn
{
private:
    static void * handle_;
    static ICameraServiceAddon *plugin_;

private:
    static int getDeviceCounts(DEVICE_TYPE_T);
    static int getDeviceList(int*, int*, int*, int*);
    static int addDevice(DEVICE_LIST_T*);
    static bool removeDevice(int);
    static int addRemoteCamera(deviceInfo_t*);
    static int removeRemoteCamera(int);
    static bool getCurrentDeviceInfo(std::string&, std::string&, std::string&);

public:
    static void open();
    static void close();

    static bool hasImplementation();

    static void initialize(LSHandle*);
    static void setSubscriptionForCameraList(LSMessage &);
    static void setDeviceEvent(DEVICE_LIST_T*, int, bool, bool remote = false);
    static bool setPermission(LSMessage &);
    static bool isSupportedCamera(std::string, std::string);
    static bool isAppPermission(std::string);
    static bool test(LSMessage &);
    static bool isResumeDone();

    static bool toastCameraUsingPopup();
    static void logMessagePrivate(std::string);
};

#endif /* ADDON_H_ */
