#ifndef DEVICE_INFO_MANAGED_AS_A_LIST_ITEM_H_
#define DEVICE_INFO_MANAGED_AS_A_LIST_ITEM_H_


#include "camera_constants.h"

struct DEVICE_LIST_T
{
    int nDeviceNum;
    int nPortNum;
    int isPowerOnConnect;
    std::string strVendorName;
    std::string strProductName;
    std::string strVendorID;
    std::string strProductID;
    std::string strDeviceType;
    std::string strDeviceSubtype;
    std::string strDeviceNode;
    std::string strHostControllerInterface;
    std::string strDeviceKey;
};

#endif /* DEVICE_INFO_MANAGED_AS_A_LIST_ITEM_H_ */
