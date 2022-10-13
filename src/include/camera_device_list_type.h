#ifndef DEVICE_INFO_MANAGED_AS_A_LIST_ITEM_H_
#define DEVICE_INFO_MANAGED_AS_A_LIST_ITEM_H_


#include "camera_constants.h"

typedef struct
{
    int nDeviceNum;
    int nPortNum;
    char strVendorName[CONST_MAX_STRING_LENGTH];
    char strProductName[CONST_MAX_STRING_LENGTH];
    char strVendorID[CONST_MAX_STRING_LENGTH];
    char strProductID[CONST_MAX_STRING_LENGTH];
    char strDeviceType[CONST_MAX_STRING_LENGTH];
    char strDeviceSubtype[CONST_MAX_STRING_LENGTH];
    int isPowerOnConnect;
    char strDeviceNode[CONST_MAX_STRING_LENGTH];
} DEVICE_LIST_T;

#endif /* DEVICE_INFO_MANAGED_AS_A_LIST_ITEM_H_ */
