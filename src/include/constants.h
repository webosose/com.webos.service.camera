// Copyright (c) 2018 LG Electronics, Inc.
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

#ifndef SRC_INCLUDE_CAMERA_CONST_H_
#define SRC_INCLUDE_CAMERA_CONST_H_

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define H_SERVICE_JSON_PUT(x)                                                                      \
  do                                                                                               \
  {                                                                                                \
    if (x)                                                                                         \
      json_object_put(x);                                                                          \
    x = NULL;                                                                                      \
  } while (0)

#define CONST_SERVICE_URI_NAME "camera"

#define CONST_SERVICE_NAME_CAMERA "com.webos.service.camera2"
#define CONST_LS_SERVICE_NAME_PDM "luna://com.webos.service.pdm"
#define CONST_LS_SERVICE_FUNCTION_NAME_GET_ATTACHED_NONSTORAGE_DEVICE_LIST                         \
  "getAttachedNonStorageDeviceList"

#define CONST_PARAM_NAME_RETURNVALUE "returnValue"
#define CONST_PARAM_NAME_ERROR_CODE "errorCode"
#define CONST_PARAM_NAME_ERROR_TEXT "errorText"
#define CONST_PARAM_NAME_SUBSCRIBE "subscribe"
#define CONST_PARAM_NAME_SUBSCRIBED "subscribed"
#define CONST_PARAM_NAME_PLUGIN_NAME "PluginName"
#define CONST_PARAM_NAME_PLUGIN "Plugin"
#define CONST_PARAM_NAME_NAME "name"
#define CONST_PARAM_NAME_TYPE "type"
#define CONST_MODULE_DM "DeviceManager"
#define CONST_MODULE_VDM "VirtualDeviceManager"
#define CONST_MODULE_CM "CommandHandler"
#define CONST_MODULE_DC "DeviceController"
#define CONST_MODULE_SM "SessionManager"
#define CONST_MODULE_HAL "HAL"
#define CONST_PARAM_NAME_URI "uri"
#define CONST_DEVICE_NAME_CAMERA "camera"
#define CONST_DEVICE_NAME_MIC "mic"
#define CONST_DEVICE_HANDLE "handle"
#define CONST_DEVICE_KEY "key"
#define CONST_PARAM_NAME_SOURCE "source"

#define CONST_VARIABLE_INITIALIZE -999
// default cam parameter
#define CONST_DEFAULT_HEIGHT 480
#define CONST_DEFAULT_WIDTH 640
#define CONST_DEFAULT_FORMAT CAMERA_FORMAT_JPEG
#define CONST_DEFAULT_NIMAGE "nimage"

#define CONST_MAX_LONG_STRING_LENGTH 1000

#define CONST_MAX_STRING_LENGTH 256
#define CONST_MAX_PATH 256
#define CONST_MODULE_LUNA "service"
#define CONST_PARAM_PLUGIN_PATH "PluginPath"

#define CONST_PARAM_VALUE_FALSE 0
#define CONST_PARAM_VALUE_TRUE 1
#define CONST_MAX_DEVICE_COUNT 10

#define CONST_PARAM_NAME_AUTOEXPOSURE "autoExposure"
#define CONST_PARAM_NAME_AUTOWHITEBALANCE "autoWhiteBalance"
#define CONST_PARAM_NAME_BACKLIGHT_COMPENSATION "backlightCompensation"
#define CONST_PARAM_NAME_BEFORE "before"
#define CONST_PARAM_NAME_BIRGHTNESS "brightness"
#define CONST_PARAM_NAME_BITRATE "bitrate"
#define CONST_PARAM_NAME_BROADCASTID "broadcastId"
#define CONST_PARAM_NAME_BUILTIN "builtin"
#define CONST_PARAM_NAME_BUTTONS "buttons"
#define CONST_PARAM_NAME_CAMERA "camera"
#define CONST_PARAM_NAME_CAMERA_READY "cameraReady"
#define CONST_PARAM_NAME_CAMERA_SERVICE "camera-service"
#define CONST_PARAM_NAME_CLIENT_NAME "clientName"
#define CONST_PARAM_NAME_CODEC "codec"
#define CONST_PARAM_NAME_COL "col"
#define CONST_PARAM_NAME_COMBINE "combine"
#define CONST_PARAM_NAME_CONTEXT "context"
#define CONST_PARAM_NAME_CONTRAST "contrast"
#define CONST_PARAM_NAME_COUNTRY_LANGUAGE "countryLanguage"
#define CONST_PARAM_NAME_COVERED "covered"
#define CONST_PARAM_NAME_CROP "crop"
#define CONST_PARAM_NAME_DAY "day"
#define CONST_PARAM_NAME_DETAILS "details"
#define CONST_PARAM_NAME_DEVICE "device"
#define CONST_PARAM_NAME_DEVICE_NUM "deviceNum"
#define CONST_PARAM_NAME_DEVICE_TYPE "deviceType"
#define CONST_PARAM_NAME_DEVICE_SUBTYPE "deviceSubtype"
#define CONST_PARAM_NAME_EFFECT_FILE_SIZE "effectFileSize"
#define CONST_PARAM_NAME_EFFECT_FILE_NAME "effectFileName"
#define CONST_PARAM_NAME_ENABLE "enable"
#define CONST_PARAM_NAME_ENGINE_VERSION "engineVersion"
#define CONST_PARAM_NAME_ERLE "erle"
#define CONST_PARAM_NAME_ERROR_CODE "errorCode"
#define CONST_PARAM_NAME_ERROR_TEXT "errorText"
#define CONST_PARAM_NAME_EXPOSURE "exposure"
#define CONST_PARAM_NAME_EVENTCODE "eventCode"
#define CONST_PARAM_NAME_EVENTTEXT "eventText"
#define CONST_PARAM_NAME_FILENAMELIST "fileNameList"
#define CONST_PARAM_NAME_FORMAT "format"
#define CONST_PARAM_NAME_MODE "mode"
#define CONST_PARAM_NAME_NIMAGE "nimage"
#define CONST_PARAM_NAME_FORCE_FRAMERATE "forceFrameRate"
#define CONST_PARAM_NAME_FRAMERATE "frameRate"
#define CONST_PARAM_NAME_FREQUENCY "frequency"
#define CONST_PARAM_NAME_FREQUENCY_RESPONSE "frequencyResponse"
#define CONST_PARAM_NAME_GAIN "gain"
#define CONST_PARAM_NAME_GAMMA "gamma"
#define CONST_PARAM_NAME_GOPLENGTH "gopLength"
#define CONST_PARAM_NAME_GRAPHICRESOLUTION "graphicResolution"
#define CONST_PARAM_NAME_GRIDZOOM "gridZoom"
#define CONST_PARAM_NAME_HEIGHT "height"
#define CONST_PARAM_NAME_HOUR "hour"
#define CONST_PARAM_NAME_HUE "hue"
#define CONST_PARAM_NAME_ICON_URL "iconUrl"
#define CONST_PARAM_NAME_ID "id"
#define CONST_PARAM_NAME_INFO "info"
#define CONST_PARAM_NAME_INTERVAL "interval"
#define CONST_PARAM_NAME_IS_NEW_DEVICE "isNewDevice"
#define CONST_PARAM_NAME_IS_POWER_ON_CONNECT "isPowerOnConnect"
#define CONST_PARAM_NAME_KEY "key"
#define CONST_PARAM_NAME_LABEL "label"
#define CONST_PARAM_NAME_LOAD_COMPLETED "loadCompleted"
#define CONST_PARAM_NAME_LOCALEINFO "localeInfo"
#define CONST_PARAM_NAME_LOCALES "locales"
#define CONST_PARAM_NAME_LOCALTIME "localtime"
#define CONST_PARAM_NAME_KEYS "keys"
#define CONST_PARAM_NAME_LED "led"
#define CONST_PARAM_NAME_MAIN "MAIN"
#define CONST_PARAM_NAME_MAXWIDTH "maxWidth"
#define CONST_PARAM_NAME_MAXHEIGHT "maxHeight"
#define CONST_PARAM_NAME_MEDIAID "mediaId"
#define CONST_PARAM_NAME_MESSAGE "message"
#define CONST_PARAM_NAME_MESSAGECODE "messageCode"
#define CONST_PARAM_NAME_MESSAGETEXT "messageText"
#define CONST_PARAM_NAME_MICGAIN "micGain"
#define CONST_PARAM_NAME_MICMAXGAIN "micMaxGain"
#define CONST_PARAM_NAME_MICMINGAIN "micMinGain"
#define CONST_PARAM_NAME_MICMUTE "micMute"
#define CONST_PARAM_NAME_MICROPHONE "microphone"
#define CONST_PARAM_NAME_MINUTE "minute"
#define CONST_PARAM_NAME_MIRROR "mirror"
#define CONST_PARAM_NAME_MODAL "modal"
#define CONST_PARAM_NAME_MONTH "month"
#define CONST_PARAM_NAME_MPEGTS "MPEGTS"
#define CONST_PARAM_NAME_NAME "name"
#define CONST_PARAM_NAME_NO_ACTION "noaction"
#define CONST_PARAM_NAME_NOISE "noise"
#define CONST_PARAM_NAME_NON_STORAGE_DEVICE_LIST "nonStorageDeviceList"
#define CONST_PARAM_NAME_ONCLICK "onclick"
#define CONST_PARAM_NAME_OUT_FILE_PATH "outFilePath"
#define CONST_PARAM_NAME_OUT_HEIGHT "outHeight"
#define CONST_PARAM_NAME_OUT_WIDTH "outWidth"
#define CONST_PARAM_NAME_PAN "pan"
#define CONST_PARAM_NAME_PARAMS "params"
#define CONST_PARAM_NAME_PATH "path"
#define CONST_PARAM_NAME_PAYLOAD "payload"
#define CONST_PARAM_NAME_PICTURE "picture"
#define CONST_PARAM_NAME_PIPELINE_MODE "pipelineMode"
#define CONST_PARAM_NAME_POSITION "position"
#define CONST_PARAM_NAME_POWER_ON_KEYWORD "powerOnKeyword"
#define CONST_PARAM_NAME_POWERONLY "poweronly"
#define CONST_PARAM_NAME_POWERONLYMODE "powerOnlyMode"
#define CONST_PARAM_NAME_POWER_ON_MODE "powerOnMode"
#define CONST_PARAM_NAME_POWER_STATUS "powerStatus"
#define CONST_PARAM_NAME_PROCESSING_MODE "processingMode"
#define CONST_PARAM_NAME_PRODUCT_NAME "productName"
#define CONST_PARAM_NAME_RESOLUTION "resolution"
#define CONST_PARAM_NAME_RETURNVALUE "returnValue"
#define CONST_PARAM_NAME_ROW "row"
#define CONST_PARAM_NAME_RESTART "restart"
#define CONST_PARAM_NAME_REVERBERATION_TIME "reverberationTime"
#define CONST_PARAM_NAME_SAMPLINGRATE "samplingRate"
#define CONST_PARAM_NAME_SATURATION "saturation"
#define CONST_PARAM_NAME_SAVE_PATH "savePath"
#define CONST_PARAM_NAME_SECOND "second"
#define CONST_PARAM_NAME_SERIAL_NUMBER "serialNumber"
#define CONST_PARAM_NAME_SERVICE_READY "serviceReady"
#define CONST_PARAM_NAME_SETTINGS "settings"
#define CONST_PARAM_NAME_SHARPNESS "sharpness"
#define CONST_PARAM_NAME_SIGNAL_LEVEL "signalLevel"
#define CONST_PARAM_NAME_SINKTYPE "sinkType"
#define CONST_PARAM_NAME_SNAPSHOTCOUNT "snapshotCount"
#define CONST_PARAM_NAME_SOURCE_ID "sourceId"
#define CONST_PARAM_NAME_STATE "state"
#define CONST_PARAM_NAME_STREAM_TYPE "streamType"
#define CONST_PARAM_NAME_SUBSCRIBE "subscribe"
#define CONST_PARAM_NAME_SUBSCRIBED "subscribed"
#define CONST_PARAM_NAME_SUPPORT "support"
#define CONST_PARAM_NAME_TILT "tilt"
#define CONST_PARAM_NAME_TIME "time"
#define CONST_PARAM_NAME_TIMESTAMP "timestamp"
#define CONST_PARAM_NAME_TITLE "title"
#define CONST_PARAM_NAME_TRIGGER_KEYWORD "triggerKeyword"
#define CONST_PARAM_NAME_TRIGGER_MODE "triggerMode"
#define CONST_PARAM_NAME_TYPE "type"
#define CONST_PARAM_NAME_TYPE_OF_EFFECT "typeOfEffect"
#define CONST_PARAM_NAME_UI "UI"
#define CONST_PARAM_NAME_UPDATING "updating"
#define CONST_PARAM_NAME_URI "uri"
#define CONST_PARAM_NAME_URILIST "uriList"
#define CONST_PARAM_NAME_DEVICE_LIST "deviceList"
#define CONST_PARAM_NAME_UNLOAD_COMPLETED "unloadCompleted"
#define CONST_PARAM_NAME_USB_PORT_NUM "usbPortNum"
#define CONST_PARAM_NAME_VAD "vad"
#define CONST_PARAM_NAME_VENDOR_NAME "vendorName"
#define CONST_PARAM_NAME_VERSION "version"
#define CONST_PARAM_NAME_VIDEO "video"
#define CONST_PARAM_NAME_WHITEBALANCETEMPERATURE "whiteBalanceTemperature"
#define CONST_PARAM_NAME_WIDTH "width"
#define CONST_PARAM_NAME_X "x"
#define CONST_PARAM_NAME_Y "y"
#define CONST_PARAM_NAME_YEAR "year"
#define CONST_PARAM_NAME_YUVMODE "yuvMode"
#define CONST_PARAM_NAME_ZOOM "zoom"
#define CONST_PARAM_NAME_FILENAMELIST "fileNameList"
#define CONST_PARAM_NAME_ILLUMINATION "illumination"

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SRC_INCLUDE_CAMERA_CONST_H_*/
