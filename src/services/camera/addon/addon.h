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
    static bool updateDeviceList(std::string, const std::vector<DEVICE_LIST_T> &);

private:
    struct Service : public ICameraService
    {
        int getDeviceCounts(std::string) override;
        bool updateDeviceList(std::string, const std::vector<DEVICE_LIST_T> &) override;
    };

    static Service *service_;

public:
    static void open();
    static void close();

    static bool hasImplementation();
    static void initialize(LSHandle *);
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
