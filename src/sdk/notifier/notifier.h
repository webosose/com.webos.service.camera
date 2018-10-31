#ifndef CAMERA_NOTIFIER
#define CAMERA_NOTIFIER

#include <iostream>
#include <functional>
#include "cameratypes.h"
#include "device_notifier.h"

using namespace std;

class Notifier
{
private:
    using handler_cb_ = std::function<void(camera_info_t *)>;
    handler_cb_ subscribeToDeviceState;

    int dev_count_;
    camera_info_t st_dev_info_[MAX_DEVICE_COUNT];

public:
    DeviceNotifier *p_client_notifier_;
    int addNotifier(notifier_client_t);
    int registerCallback(handler_cb_);

    virtual int updateDeviceList(std::string,int,camera_info_t *);
    virtual int queryDeviceStatus(int,device_event_state_t);
    virtual int getDeviceInfo(int,camera_info_t *);
};

#endif
