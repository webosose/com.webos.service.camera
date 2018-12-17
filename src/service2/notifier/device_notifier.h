#ifndef DEVICE_NOTIFIER_H_
#define DEVICE_NOTIFIER_H_

#include "camera_types.h"
#include "luna-service2/lunaservice.hpp"

class DeviceNotifier
{
private:
    using handlercb = std::function<void(DEVICE_LIST_T *)>;
public:
    virtual void subscribeToClient(handlercb) = 0;
    virtual void setLSHandle(LSHandle *) = 0;
};

#endif /* DEVICE_NOTIFIER_H_ */
