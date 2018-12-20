#ifndef NOTIFIER_H_
#define NOTIFIER_H_

#include "camera_types.h"
#include "service_types.h"
#include <functional>
#include <iostream>

#include "device_notifier.h"
#include "luna-service2/lunaservice.hpp"
#include "pdm_client.h"

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
