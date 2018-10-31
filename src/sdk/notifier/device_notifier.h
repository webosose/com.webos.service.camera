#ifndef DEVICE_NOTIFIER
#define DEVICE_NOTIFIER

#include "cameratypes.h"

class DeviceNotifier
{
private:
    using handlerCb = std::function<void(camera_info_t *)>;
public:
    virtual int subscribeToClient(handlerCb) = 0;
};

#endif
