#ifndef PDM_CLIENT
#define PDM_CLIENT

#include <functional>
#include "cameratypes.h"
#include "device_notifier.h"
#include <luna-service2/lunaservice.hpp>

static bool deviceStateCb(LSHandle *, LSMessage *, void *);
using pdmhandler_cb = std::function<void(camera_info_t *)>;

class PDMClient : public DeviceNotifier
{
private:
    int subscribeToPdmService();

public:
    virtual ~PDMClient(){}
    virtual int subscribeToClient(pdmhandler_cb) override;
};

#endif
