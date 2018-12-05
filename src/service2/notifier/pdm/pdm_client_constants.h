#ifndef PDM_CLIENT_CONSTANTS_H_
#define PDM_CLIENT_CONSTANTS_H_

#include <string>

const std::string clientsrv = "com.webos.service.pdmclient";
const std::string uri = "luna://com.webos.service.pdm/getAttachedNonStorageDeviceList";
const std::string payload = "{\"subscribe\":true}";
const std::string returnval = "returnValue";

const std::string powerstatus = "powerStatus";
const std::string nonstoragedevlist = "nonStorageDeviceList";
const std::string strdevicenum = "deviceNum";
const std::string usbportnum = "usbPortNum";
const std::string vendorname = "vendorName";
const std::string productname = "productName";
const std::string serialnumber = "serialNumber";
const std::string devicetype = "deviceType";
const std::string devicesubtype = "deviceSubtype";
const std::string cam = "CAM";

#endif /* PDM_CLIENT_CONSTANTS_H_ */
