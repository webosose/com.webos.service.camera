// Copyright (c) 2019-2023 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef CAMERA_CONST_H_
#define CAMERA_CONST_H_

#include <string>

#define CONST_PARAM_DEFAULT_VALUE -999
#define CONST_MAX_STRING_LENGTH 256
#define CONST_PARAM_VALUE_FALSE 0
#define CONST_PARAM_VALUE_TRUE 1
#define CONST_MAX_DEVICE_COUNT 10

#define CONST_MODULE_DM "DeviceManager"
#define CONST_MODULE_VDM "VirtualDeviceManager"
#define CONST_MODULE_CM "CommandHandler"
#define CONST_MODULE_DC "DeviceController"
#define CONST_MODULE_LUNA "Service"
#define CONST_MODULE_NOTIFIER "Notifier"
#define CONST_MODULE_PDMCLIENT "PDMClient"
#define CONST_MODULE_SHM "SHM_API"
#define CONST_MODULE_SM "SolutionManager"
#define CONST_MODULE_DPY "PreviewDisplayControl"

#define CONST_DEVICE_NAME_CAMERA "camera"
#define CONST_DEVICE_HANDLE "handle"
#define CONST_DEVICE_KEY "key"
#define CONST_DEFAULT_NIMAGE "nimage"

#define CONST_PARAM_NAME_RETURNVALUE "returnValue"
#define CONST_PARAM_NAME_ERROR_CODE "errorCode"
#define CONST_PARAM_NAME_ERROR_TEXT "errorText"
#define CONST_PARAM_NAME_NAME "name"
#define CONST_PARAM_NAME_TYPE "type"
#define CONST_PARAM_NAME_SOURCE "source"
#define CONST_PARAM_NAME_AUTOEXPOSURE "autoExposure"
#define CONST_PARAM_NAME_AUTOWHITEBALANCE "autoWhiteBalance"
#define CONST_PARAM_NAME_AUTOFOCUS "autoFocus"
#define CONST_PARAM_NAME_FOCUS_ABSOLUTE "focusAbsolute"
#define CONST_PARAM_NAME_BACKLIGHT_COMPENSATION "backlightCompensation"
#define CONST_PARAM_NAME_BIRGHTNESS "brightness"
#define CONST_PARAM_NAME_BITRATE "bitrate"
#define CONST_PARAM_NAME_BUILTIN "builtin"
#define CONST_PARAM_NAME_SUPPORTED "supported"
#define CONST_PARAM_NAME_DETAILS "details"
#define CONST_PARAM_NAME_CONTRAST "contrast"
#define CONST_PARAM_NAME_FORMAT "format"
#define CONST_PARAM_NAME_MODE "mode"
#define CONST_PARAM_NAME_EXPOSURE "exposure"
#define CONST_PARAM_NAME_ID "id"
#define CONST_PARAM_NAME_HEIGHT "height"
#define CONST_PARAM_NAME_FRAMERATE "frameRate"
#define CONST_PARAM_NAME_INFO "info"
#define CONST_PARAM_NAME_HUE "hue"
#define CONST_PARAM_NAME_GAIN "gain"
#define CONST_PARAM_NAME_GAMMA "gamma"
#define CONST_PARAM_NAME_FREQUENCY "frequency"
#define CONST_PARAM_NAME_GOPLENGTH "gopLength"
#define CONST_PARAM_NAME_IS_POWER_ON_CONNECT "isPowerOnConnect"
#define CONST_PARAM_NAME_MAXWIDTH "maxWidth"
#define CONST_PARAM_NAME_MAXHEIGHT "maxHeight"
#define CONST_PARAM_NAME_MIRROR "mirror"
#define CONST_PARAM_NAME_LED "led"
#define CONST_PARAM_NAME_MICMAXGAIN "micMaxGain"
#define CONST_PARAM_NAME_MICMINGAIN "micMinGain"
#define CONST_PARAM_NAME_MICGAIN "micGain"
#define CONST_PARAM_NAME_MICMUTE "micMute"
#define CONST_PARAM_NAME_PARAMS "params"
#define CONST_PARAM_NAME_PICTURE "picture"
#define CONST_PARAM_NAME_PAN "pan"
#define CONST_PARAM_NAME_SATURATION "saturation"
#define CONST_PARAM_NAME_SHARPNESS "sharpness"
#define CONST_PARAM_NAME_TILT "tilt"
#define CONST_PARAM_NAME_DEVICE_LIST "deviceList"
#define CONST_PARAM_NAME_APP_PRIORITY "mode"
#define CONST_PARAM_NAME_WIDTH "width"
#define CONST_PARAM_NAME_VIDEO "video"
#define CONST_PARAM_NAME_ZOOM_ABSOLUTE "zoomAbsolute"
#define CONST_PARAM_NAME_WHITEBALANCETEMPERATURE "whiteBalanceTemperature"
#define CONST_PARAM_NAME_YUVMODE "yuvMode"
#define CONST_PARAM_NAME_ILLUMINATION "illumination"
#define CONST_PARAM_NAME_DEVICE_LIST_INFO "deviceListInfo"
#define CONST_PARAM_NAME_NON_STORAGE_DEVICE_LIST "nonStorageDeviceList"
#define CONST_PARAM_NAME_DEVICE_NUM "deviceNum"
#define CONST_PARAM_NAME_USB_PORT_NUM "usbPortNum"
#define CONST_PARAM_NAME_VENDOR_NAME "vendorName"
#define CONST_PARAM_NAME_VIDEO_DEVICE_LIST "videoDeviceList"
#define CONST_PARAM_NAME_PRODUCT_NAME "productName"
#define CONST_PARAM_NAME_VENDOR_ID "vendorID"
#define CONST_PARAM_NAME_PRODUCT_ID "productID"
#define CONST_PARAM_NAME_SERIAL_NUMBER "serialNumber"
#define CONST_PARAM_NAME_SUB_DEVICE_LIST "subDeviceList"
#define CONST_PARAM_NAME_DEVICE_TYPE "deviceType"
#define CONST_PARAM_NAME_DEVICE_SUBTYPE "deviceSubtype"
#define CONST_PARAM_NAME_EVENT "eventType"
#define CONST_PARAM_NAME_RESOLUTION "resolution"
#define CONST_PARAM_NAME_FORMATINFO "formatInfo"
#define CONST_PARAM_NAME_PROPERTIESINFO "propertiesInfo"
#define CONST_EVENT_KEY_PREVIEW_FAULT "EventPreviewFault"
#define CONST_EVENT_KEY_CAMERA_LIST "EventCameraList"
#define CONST_EVENT_KEY_FORMAT "EventFormat"
#define CONST_EVENT_KEY_PROPERTIES "EventProperties"
#define CONST_PARAM_NAME_FPS "fps"
#define CONST_PARAM_NAME_IMAGE_PATH "path"
#define CONST_PARAM_NAME_DEVICE_PATH "devPath"
#define CONST_PARAM_NAME_CAPABILITIES "capabilities"
#define CONST_PARAM_NAME_SUBSYSTEM "SUBSYSTEM"
#define CONST_PARAM_NAME_KERNEL "KERNEL"
#define CONST_CLIENT_PROCESS_ID "pid"
#define CONST_CLIENT_SIGNAL_NUM "sig"
#define CONST_PARAM_NAME_MAX "max"
#define CONST_PARAM_NAME_MIN "min"
#define CONST_PARAM_NAME_STEP "step"
#define CONST_PARAM_NAME_DEFAULT_VALUE "default"
#define CONST_PARAM_NAME_VALUE "value"
#define CONST_PARAM_NAME_NOTSUPPORT "not support"
#define CONST_PARAM_NAME_SUPPORTED "supported"
#define CONST_PARAM_NAME_SOLUTION "solutions"
#define CONST_PARAM_NAME_SUBSCRIBED "subscribed"
#define CONST_EVENT_KEY_FORMAT "EventFormat"
#define CONST_PARAM_NAME_WINDOW_ID "windowId"

const int n_invalid_id = -1;
const int frame_count = 8;
const int extra_buffer = 1024;
const int buffer_count = 4;
const int n_invalid_pid = -1;
const int n_invalid_sig = -1;

const std::string cstr_empty = "";
const std::string cstr_pdmclient = "com.webos.service.pdmclient";
const std::string cstr_uri = "luna://com.webos.service.pdm/getAttachedNonStorageDeviceList";
const std::string cstr_payload = "{\"subscribe\":true,\"category\":\"Video\", \"groupSubDevices\":true}";
const std::string cstr_powerstatus = "powerStatus";
const std::string cstr_cam = "CAM";
const std::string cstr_capture = ":capture:";
const std::string cstr_invaliddeviceid = "-1";
const std::string cstr_yuvformat = "YUV";
const std::string cstr_h264esformat = "H264ES";
const std::string cstr_jpegformat = "JPEG";
const std::string cstr_nv12format = "NV12";
const std::string cstr_primary = "primary";
const std::string cstr_secondary = "secondary";
const std::string cstr_format = "format";
const std::string cstr_properties = "properties";
const std::string cstr_oneshot = "MODE_ONESHOT";
const std::string cstr_burst = "MODE_BURST";
const std::string cstr_continuous = "MODE_CONTINUOUS";
const std::string cstr_connect = "device_connect";
const std::string cstr_disconnect = "device_disconnect";
const std::string cstr_devicefault = "device_fault";

#endif /*CAMERA_CONST_H_*/
