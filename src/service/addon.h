#ifndef ADDON_H_
#define ADDON_H_

#include "camera_service_addon_interface.h"

class AddOn
{
private:
    static void * handle_;
    static ICameraServiceAddon *plugin_;
    static int getDeviceList(int*,int*,int*,int*);

private:
    static EXTRA_DATA_T extra_data_;

public:
    static EXTRA_DATA_T currentExtraData();

public:
    static void open();
    static void close();

    static bool hasImplementation();

    static void initialize(LSHandle*);
    static void setSubscriptionForCameraList(LSMessage &);
    static void setDeviceEvent(DEVICE_LIST_T*, int);
    static bool setPermission(LSMessage &);
    static bool isSupportedCamera(std::string, std::string);
    static bool isAppPermission(std::string);
    static bool test(LSMessage &);
    static void getDeviceExtraData(jvalue_ref &);
    static void setDeviceExtraData(int, EXTRA_DATA_T);
    static void logExtraMessage(std::string);
    static int getDevicePort(jvalue_ref &);
};

#endif /* ADDON_H_ */
