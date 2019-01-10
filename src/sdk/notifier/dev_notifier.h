#ifndef DEV_NOTIFIER
#define DEV_NOTIFIER

#include "cameratypes.h"

class DevNotifier
{
private:
  using handlerCb = std::function<void(camera_details_t *)>;

public:
  virtual int subscribeToClient(handlerCb) = 0;
};

#endif
