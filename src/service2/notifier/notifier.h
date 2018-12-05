#ifndef NOTIFIER_H_
#define NOTIFIER_H_

#include <iostream>
#include <functional>
#include "service_types.h"
#include "device_notifier.h"
#include "pdm_client.h"
#include "luna-service2/lunaservice.hpp"

static void updateDeviceList(device_info_t *);

class Notifier
{
  private:
    using handlercb = std::function<void(device_info_t *)>;

    PDMClient pdm_;
    LSHandle *lshandle_;

  public:
    virtual ~Notifier() {}
    DeviceNotifier *p_client_notifier_;

    void addNotifier(NotifierClient);
    void registerCallback(handlercb);
    void setLSHandle(LSHandle *);
    int getDeviceInfo(int, device_info_t *);
};

#endif /* NOTIFIER_H_ */
