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
    static bool updateDeviceList(std::string, const std::vector<DEVICE_LIST_T> &);
    static int getInfo(int, camera_device_info_t *);

private:
    struct Service : public ICameraService
    {
        int getDeviceList(std::vector<int> &) override;
        int getDeviceCounts(std::string) override;
        bool updateDeviceList(std::string, const std::vector<DEVICE_LIST_T> &) override;
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
    static void notifyDeviceAdded(const DEVICE_LIST_T &);
    static void notifyDeviceRemoved(const DEVICE_LIST_T &);
    static void notifyDeviceListUpdated(std::string, const std::vector<DEVICE_LIST_T> &);
    static bool notifyDeviceOpened(std::string, std::string, std::string);
    static void notifySolutionEnabled(std::string, const std::vector<std::string> &);
    static void notifySolutionDisabled(std::string, const std::vector<std::string> &);
    static std::vector<std::string> getEnabledSolutionList(std::string);
};

#endif /* ADDON_H_ */
