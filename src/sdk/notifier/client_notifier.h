#ifndef CAMERA_NOTIFIER
#define CAMERA_NOTIFIER

#include "cameratypes.h"
#include "dev_notifier.h"
#include "pdmclient.h"
#include "udev_client.h"
#include <functional>
#include <iostream>

using namespace std;

class Notifier
{
private:
  using handler_cb_ = std::function<void(camera_details_t *)>;
  handler_cb_ subscribeToDeviceState;
  DevNotifier *p_client_notifier_;
  PdmClient pdm_;
  UDEVClient udev_;
  int dev_count_;
  camera_details_t st_dev_info_[MAX_DEVICE_COUNT];

public:
  Notifier()
  {
    p_client_notifier_ = nullptr;
    dev_count_ = 0;
  }
  virtual ~Notifier() {}
  int addNotifier(notifier_client_t);
  int registerCallback(handler_cb_);

  virtual int updateDeviceList(std::string, int, camera_details_t *);
  virtual int getDeviceInfo(int, camera_details_t *);
};

#endif
