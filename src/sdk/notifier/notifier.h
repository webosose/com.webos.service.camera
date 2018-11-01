#ifndef CAMERA_NOTIFIER
#define CAMERA_NOTIFIER

#include <iostream>
#include <functional>
#include "cameratypes.h"
#include "device_notifier.h"
#include "udev_client.h"
#include "pdm_client.h"

using namespace std;

class Notifier
{
private:
    using handler_cb_ = std::function<void(camera_info_t *)>;
    handler_cb_ subscribeToDeviceState;

    PDMClient pdm_;
    UDEVClient udev_;
    int dev_count_;
    camera_info_t st_dev_info_[MAX_DEVICE_COUNT];

public:
    virtual ~Notifier(){}
    DeviceNotifier *p_client_notifier_;
    int addNotifier(notifier_client_t);
    int registerCallback(handler_cb_);

    virtual int updateDeviceList(std::string,int,camera_info_t *);
    virtual int getDeviceInfo(int,camera_info_t *);
};

#endif
