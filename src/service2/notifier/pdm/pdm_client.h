#ifndef PDM_CLIENT
#define PDM_CLIENT

#include "camera_types.h"
#include "device_notifier.h"
#include "service_types.h"
#include <functional>
#include <luna-service2/lunaservice.hpp>

static bool deviceStateCb(LSHandle *, LSMessage *, void *);

using pdmhandlercb = std::function<void(DEVICE_LIST_T *)>;

class PDMClient : public DeviceNotifier
{
private:
  int subscribeToPdmService();
  LSHandle *lshandle_;

public:
  virtual ~PDMClient() {}
  virtual void subscribeToClient(pdmhandlercb) override;
  void setLSHandle(LSHandle *);
};

#endif
