#ifndef PDM_CLIENT
#define PDM_CLIENT

#include "cameratypes.h"
#include "dev_notifier.h"
#include <functional>
#include <luna-service2/lunaservice.hpp>

static bool deviceStateCb(LSHandle *, LSMessage *, void *);
using pdmhandler_cb = std::function<void(camera_details_t *)>;

class PdmClient : public DevNotifier
{
private:
  int subscribeToPdmService();

public:
  virtual ~PdmClient() {}
  virtual int subscribeToClient(pdmhandler_cb) override;
};

#endif
