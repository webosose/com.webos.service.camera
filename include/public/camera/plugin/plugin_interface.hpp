#pragma once
#include <cstddef>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

inline const char *basename(const char *path)
{
    const char *cp = strrchr(path, '/');
    return (cp ? cp + 1 : path);
}

#define __FILENAME__ (basename(__FILE__))

/**
 * IFeature:
 * The interface for the generic feature.
 */
struct IFeature
{
    virtual ~IFeature() {}
    virtual bool queryInterface(const char *szName, void **ppInterface) = 0;
};

/**
 * IPlugin:
 * The interface for the plugins.
 */
struct IPlugin
{
    virtual ~IPlugin() {}
    virtual const char *getName(void)                       = 0;
    virtual const char *getDescription(void)                = 0;
    virtual const char *getCategory(void)                   = 0;
    virtual const char *getVersion(void)                    = 0;
    virtual const char *getOrganization(void)               = 0;
    virtual size_t getFeatureCount(void)                    = 0;
    virtual const char *getFeatureName(const size_t nIndex) = 0;
    virtual IFeature *createFeature(const char *szName)     = 0;
};

/**
 * IHal:
 * The interface for the hal featurs.
 */
struct IHal : public IFeature
{
    virtual int openDevice(std::string devname, std::string payload)   = 0;
    virtual int closeDevice()                                          = 0;
    virtual int setFormat(const void *stream_format)                   = 0;
    virtual int getFormat(void *stream_format)                         = 0;
    virtual int setBuffer(int num_buffer, int io_mode, void **usrbufs) = 0;
    virtual int getBuffer(void *outbuf)                                = 0;
    virtual int releaseBuffer(const void *inbuf)                       = 0;
    virtual int destroyBuffer()                                        = 0;
    virtual int startCapture()                                         = 0;
    virtual int stopCapture()                                          = 0;
    virtual int setProperties(const void *cam_in_param)                = 0;
    virtual int getProperties(void *cam_out_param)                     = 0;
    virtual int getInfo(void *cam_info, std::string devicenode)        = 0;
    virtual int getBufferFd(int *bufFd, int *count)                    = 0;
};

/**
 * IAddon:
 * The interface for the addon featurs.
 */

struct ICameraService
{
    virtual ~ICameraService() {}
    virtual int getDeviceCounts(std::string type) { return 0; }
    virtual bool updateDeviceList(std::string deviceType, const void *deviceList) { return false; }
};

struct IAddon : public IFeature
{
    virtual bool hasImplementation()                                                     = 0;
    virtual void setCameraService(ICameraService *camera_service)                        = 0;
    virtual void initialize(void *lshandle)                                              = 0;
    virtual bool isSupportedCamera(std::string productID, std::string vendorID)          = 0;
    virtual bool isAppPermission(std::string appId)                                      = 0;
    virtual void notifyDeviceAdded(const void *deviceInfo)                               = 0;
    virtual void notifyDeviceRemoved(const void *deviceInfo)                             = 0;
    virtual void notifyDeviceListUpdated(std::string deviceType, const void *deviceList) = 0;
    virtual bool notifyDeviceOpened(std::string deviceKey, std::string appId,
                                    std::string appPriority)                             = 0;
    virtual void notifySolutionEnabled(std::string deviceKey,
                                       const std::vector<std::string> &solutions)        = 0;
    virtual void notifySolutionDisabled(std::string deviceKey,
                                        const std::vector<std::string> &solutions)       = 0;
    virtual std::vector<std::string> getEnabledSolutionList(std::string deviceKey)       = 0;
};

/**
 * IHal:
 * The interface for the solution featurs.
 */

struct CameraSolutionEvent
{
    virtual void onInitialized(void) {}
    virtual void onEnabled(void) {}
    virtual void onDone(const char *szResult) {}
    virtual void onDisabled(void) {}
    virtual void onReleased(void) {}
};

struct ISolution : public IFeature
{
    virtual void setEventListener(CameraSolutionEvent *pEvent) = 0;
    virtual int32_t getMetaSizeHint(void)                      = 0;
    virtual void initialize(const void *streamFormat, const std::string &shmName,
                            void *lsHandle)                    = 0;
    virtual void setEnableValue(bool enableValue)              = 0;
    virtual int getProperty()                                  = 0;
    virtual bool isEnabled(void)                               = 0;
    virtual std::string getSolutionStr(void)                   = 0;
    virtual void processForSnapshot(const void *inBuf)         = 0;
    virtual void processForPreview(const void *inBuf)          = 0;
    virtual void release(void)                                 = 0;
};

/**
 * INotifier:
 * The interface for the notifier featurs.
 */
struct INotifier : public IFeature
{
public:
    using handlercb = std::function<bool(std::string notifierType, const void *deviceList)>;

    virtual void subscribeToClient(handlercb cb, void *mainLoop) = 0;
    virtual void setLSHandle(void *lshandle)                     = 0;
};

/**
 * plugin_entrypoint
 *
 * Returns the interface pointer of plugin as IPlugin.
 * Caller must delete the IPlugin pointer after use.
 *
 * Returns: (transfer all): pointer of IPlugin.
 **/
using plugin_entrypoint = IPlugin *(*)(void);
