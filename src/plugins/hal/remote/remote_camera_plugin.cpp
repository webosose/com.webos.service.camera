// Copyright (c) 2019-2022 LG Electronics, Inc.
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

#include "remote_camera_plugin.h"
#include "camera_hal_types.h"
#include "camera_types.h"
#include <dlfcn.h>

const char *rc_library = "libremote-camera.so";

/* function pointer loaded from libRemoteCamera.so */
void *create_remote_camera_handle()
{
    void *plugin = dlopen(rc_library, RTLD_LAZY);

    if (!plugin)
    {
        PMLOG_INFO(CONST_MODULE_RCP, "dlopen failed for : %s(%s)", rc_library, dlerror());
        return nullptr;
    }

    typedef void *(*pfn_create_handle)();

    pfn_create_handle pf_create_handle;
    pf_create_handle = (pfn_create_handle)dlsym(plugin, "create_handle");
    dlclose(plugin);

    if (!pf_create_handle)
    {
        PMLOG_INFO(CONST_MODULE_RCP, "dlsym failed ");
        return nullptr;
    }

    void *handle = (void *)pf_create_handle();
    PMLOG_INFO(CONST_MODULE_RCP, "handle : %p ", handle);

    return handle;
}

void *create_handle(void) { return create_remote_camera_handle(); }

void destroy_remote_camera_handle(void *handle)
{
    void *plugin = dlopen(rc_library, RTLD_LAZY);

    if (!plugin)
    {
        PMLOG_INFO(CONST_MODULE_RCP, "dlopen failed for : %s(%s)", rc_library, dlerror());
        return;
    }

    typedef void (*pfn_destroy_handle)(void *);
    pfn_destroy_handle pf_destroy_handle;
    pf_destroy_handle = (pfn_destroy_handle)dlsym(plugin, "destroy_handle");
    dlclose(plugin);

    if (!pf_destroy_handle)
    {
        PMLOG_INFO(CONST_MODULE_RCP, "dlsym failed ");
        return;
    }

    pf_destroy_handle(handle);
}

void destroy_handle(void *handle) { destroy_remote_camera_handle(handle); }
