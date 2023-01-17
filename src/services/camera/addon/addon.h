#ifndef ADDON_H_
#define ADDON_H_

#include "camera_service_addon_interface.h"

class AddOn
{
private:
    static void *handle_;
    static ICameraServiceAddon *plugin_;

private:
    static int getDeviceCounts(std::string);
    static int getDeviceList(std::vector<int> &);
    static int addDevice(DEVICE_LIST_T *, std::string = "");
    static bool removeDevice(int);
    static int getInfo(int, camera_device_info_t *);

private:
    struct Service : public ICameraService
    {
        int getDeviceList(std::vector<int> &) override;
        int getDeviceCounts(std::string) override;
        int addDevice(DEVICE_LIST_T *, std::string = "") override;
        bool removeDevice(int) override;
        int getInfo(int, camera_device_info_t *) override;
        void getSupportedSolutionList(std::vector<std::string> &,
                                      std::vector<std::string> &) override;
    };

    static Service *service_;

public:
    static void open();
    static void close();

    static bool hasImplementation();

    static void initialize(LSHandle *);
    static void setSubscriptionForCameraList(LSMessage &);

    static bool isSupportedCamera(std::string, std::string);
    static bool isAppPermission(std::string);

    static void notifyDeviceAdded(int, const DEVICE_LIST_T &);
    static void notifyDeviceRemoved(int);
    static bool notifyDeviceOpened(int deviceid, std::string appId, std::string appPriority);

    static void notifySolutionEnabled(int, const std::vector<std::string> &);
    static void notifySolutionDisabled(int, const std::vector<std::string> &);
    static std::vector<std::string> getEnabledSolutionList(int);
};

#endif /* ADDON_H_ */
