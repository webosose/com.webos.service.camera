#ifndef NOTIFIER_H_
#define NOTIFIER_H_

#include <iostream>
#include <functional>
#include "service_types.h"
#include "camera_types.h"

#include "device_notifier.h"
#include "pdm_client.h"
#include "luna-service2/lunaservice.hpp"

static void updateDeviceList(DEVICE_LIST_T *);

class Notifier
{
private:
    using handlercb = std::function<void(DEVICE_LIST_T *)>;

    PDMClient pdm_;
    LSHandle *lshandle_;

  public:
    virtual ~Notifier() {}
    DeviceNotifier *p_client_notifier_;

    void addNotifier(NotifierClient);
    void registerCallback(handlercb);
    void setLSHandle(LSHandle *);
};

#endif /* NOTIFIER_H_ */
