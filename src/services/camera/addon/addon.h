#ifndef ADDON_H_
#define ADDON_H_

#include "camera_service_addon_interface.h"

class AddOn
{
private:
    static void *handle_;
    static ICameraServiceAddon *plugin_;

private:
    static int getDeviceCounts(std::string deviceType);
    static bool updateDeviceList(std::string deviceType,
                                 const std::vector<DEVICE_LIST_T> &deviceList);

private:
    struct Service : public ICameraService
    {
        int getDeviceCounts(std::string deviceType) override;
        bool updateDeviceList(std::string deviceType,
                              const std::vector<DEVICE_LIST_T> &deviceList) override;
    };

    static Service *service_;

public:
    static void open();
    static void close();

    static bool hasImplementation();
    static void initialize(LSHandle *lsHandle);
    static bool isSupportedCamera(std::string productId, std::string vendorId);
    static bool isAppPermission(std::string appId);
    static void notifyDeviceAdded(const DEVICE_LIST_T &deviceInfo);
    static void notifyDeviceRemoved(const DEVICE_LIST_T &deviceInfo);
    static void notifyDeviceListUpdated(std::string deviceType,
                                        const std::vector<DEVICE_LIST_T> &deviceList);
    static bool notifyDeviceOpened(std::string deviceKey, std::string appId,
                                   std::string appPriority);
    static void notifySolutionEnabled(std::string deviceKey,
                                      const std::vector<std::string> &solutions);
    static void notifySolutionDisabled(std::string deviceKey,
                                       const std::vector<std::string> &solutions);
    static std::vector<std::string> getEnabledSolutionList(std::string deviceKey);
};

#endif /* ADDON_H_ */
