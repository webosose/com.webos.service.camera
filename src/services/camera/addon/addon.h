#ifndef ADDON_H_
#define ADDON_H_

#include <plugin_factory.hpp>
#include <plugin_interface.hpp>

class AddOn
{
private:
    IAddon *plugin_{nullptr};
    PluginFactory *pPluginFactory_{nullptr};
    IFeaturePtr pFeature_;

private:
    struct Service : public ICameraService
    {
        int getDeviceCounts(std::string deviceType) override;
        bool updateDeviceList(std::string deviceType,
                              const std::vector<DEVICE_LIST_T> &deviceList) override;
    };
    Service *service_{nullptr};

public:
    AddOn();
    virtual ~AddOn();

    bool hasImplementation();
    void initialize(LSHandle *lsHandle);
    bool isSupportedCamera(std::string productId, std::string vendorId);
    bool isAppPermission(std::string appId);
    void notifyDeviceAdded(const DEVICE_LIST_T &deviceInfo);
    void notifyDeviceRemoved(const DEVICE_LIST_T &deviceInfo);
    void notifyDeviceListUpdated(std::string deviceType,
                                 const std::vector<DEVICE_LIST_T> &deviceList);
    bool notifyDeviceOpened(std::string deviceKey, std::string appId, std::string appPriority);
    void notifySolutionEnabled(std::string deviceKey, const std::vector<std::string> &solutions);
    void notifySolutionDisabled(std::string deviceKey, const std::vector<std::string> &solutions);
    std::vector<std::string> getEnabledSolutionList(std::string deviceKey);
};

#endif /* ADDON_H_ */
