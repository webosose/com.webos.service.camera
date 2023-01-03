#ifndef DEVICE_CAMERA_TYPES_H_
#define DEVICE_CAMERA_TYPES_H_

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

struct deviceInfo_t
{
    std::string manufacturer;
    std::string modelName;
    std::string deviceName;
    std::string clientKey;
    std::string deviceLabel;
};

#endif /* DEVICE_CAMERA_TYPES_H_ */
