#ifndef PDM_CLIENT
#define PDM_CLIENT

#include <functional>
#include "service_types.h"
#include "device_notifier.h"
#include <luna-service2/lunaservice.hpp>

static bool deviceStateCb(LSHandle *, LSMessage *, void *);

using pdmhandlercb = std::function<void(device_info_t *)>;

class PDMClient : public DeviceNotifier
{
private:
    int subscribeToPdmService();
    LSHandle *lshandle_;

public:
    virtual ~PDMClient(){}
    virtual void subscribeToClient(pdmhandlercb) override;
    void setLSHandle(LSHandle *);
};

#endif
