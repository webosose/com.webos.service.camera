// Copyright (c) 2019-2021 LG Electronics, Inc.
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

#include "camera_types.h"
#include "whitelist_checker.h"

#define CONST_MODULE_WLIST "WhitelistChecker"

WhitelistChecker::WhitelistChecker()
{
    PMLOG_INFO(CONST_MODULE_WLIST, "%s", __func__);
    confPath_ = "/etc/com.webos.service.camera/whitelist.conf";
    PMLOG_INFO(CONST_MODULE_WLIST, "confPath_ %s",  confPath_.c_str());
}

WhitelistChecker::~WhitelistChecker()
{
    PMLOG_INFO(CONST_MODULE_WLIST, "%s", __func__);
}

bool WhitelistChecker::check(LSHandle *lsHandle, const std::string &vendor, const std::string &subtype)
{
    PMLOG_INFO(CONST_MODULE_WLIST, "%s", __func__);

    bool retValue = isSupportedCamera(vendor, subtype);

    if(retValue)
    {
        createToast(lsHandle, subtype);
    } else
    {
        createToast(lsHandle, "Unsupported camera");
    }

    return retValue;
}


bool WhitelistChecker::createToast(    LSHandle *lsHandle, const std::string &message)
{
    PMLOG_INFO(CONST_MODULE_WLIST, "%s %s", __func__, message.c_str());

    LSError lserror;
    LSErrorInit(&lserror);
    bool retValue = false;

    pbnjson::JObject params = pbnjson::JObject();
    params.put("message", pbnjson::JValue(message));
    params.put("sourceId", "com.webos.service.camera");

    retValue = LSCallOneReply( lsHandle,
        "luna://com.webos.notification/createToast",
        params.stringify().c_str(),
        NULL, NULL, NULL, &lserror);

    if(!retValue)
    {
        PMLOG_ERROR(CONST_MODULE_WLIST, "Notification: %s line : %d error on LSCallOneReply for createToast",__FUNCTION__, __LINE__);
        LSErrorPrint(&lserror, stderr);
        LSErrorFree(&lserror);
    }

    return retValue;
}

pbnjson::JValue WhitelistChecker::getListFromConfigd()
{
    PMLOG_INFO(CONST_MODULE_WLIST, "%s not implemented", __func__);
    return NULL;
}

bool WhitelistChecker::isSupportedCamera(std::string vendor, std::string subtype)
{
    PMLOG_INFO(CONST_MODULE_WLIST, "%s [%s] [%s]", __func__, vendor.c_str(), subtype.c_str());

    bool retValue = false;

    // get the JDOM tree from Confid
    auto root = getListFromConfigd();
    if (root.isObject()) {
        PMLOG_INFO(CONST_MODULE_WLIST, "Reading Configd : OK");
    }
    else {
        // get the JDOM tree from configuration file
        root = pbnjson::JDomParser::fromFile(confPath_.c_str());
        if (!root.isObject()) {
            PMLOG_ERROR(CONST_MODULE_WLIST, "configuration file parsing error! need to check %s", confPath_.c_str());
            return false;
        }
        PMLOG_INFO(CONST_MODULE_WLIST, "Reading default configuration  : OK");
    }

    // check cameraWhilteList field
    if (!root.hasKey("cameraWhilteList")) {
        PMLOG_ERROR(CONST_MODULE_WLIST, "Can't find cameraWhilteList field. need to check it!");
        return false;
    }

    // get the whitelist from cameraWhilteList field
    auto whiteList = root["cameraWhilteList"];

    for (int idx = 0; idx < whiteList.arraySize(); idx++) {
        auto wlist = whiteList[idx];
        if(wlist["vendorName"].asString().compare(vendor)==0 && wlist["deviceSubtype"].asString().compare(subtype)==0) {
            retValue = true;
        }
    }

    PMLOG_INFO(CONST_MODULE_WLIST, "isSupported : %d", retValue);
    return retValue;
}

