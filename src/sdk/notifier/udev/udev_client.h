#ifndef UDEV_CLIENT
#define UDEV_CLIENT

#include <functional>
#include <pthread.h>

#include "cameratypes.h"
#include "dev_notifier.h"
#include "libudev.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

class UDEVClient : public DevNotifier
{
private:
  using udevhandler_cb = std::function<void(camera_details_t *)>;
  udevhandler_cb subscribeToDeviceState_;

  camera_details_t *cam_info_;
  struct udev *camudev_;
  pthread_t tid_;

  static void *runMonitorDeviceThread(void *);
  int udevCreate();
  void monitorDevice();
  void getDevice(struct udev_device *, camera_details_t *);

public:
  UDEVClient();
  virtual ~UDEVClient();
  virtual int subscribeToClient(udevhandler_cb) override;
};

#endif
