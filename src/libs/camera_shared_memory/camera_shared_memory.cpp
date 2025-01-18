// Copyright (c) 2024 LG Electronics, Inc.
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

#define LOG_CONTEXT "libs"
#define LOG_TAG "CameraSharedMemory"
#include "camera_shared_memory.h"
#include "camera_shared_memory_impl.h"
#include "camera_utils_log.h"
#include <unistd.h>

CameraSharedMemory::CameraSharedMemory() : pImpl_(std::make_unique<CameraSharedMemoryImpl>())
{
    PLOGI("");
}

CameraSharedMemory::~CameraSharedMemory() { PLOGI(""); }

bool CameraSharedMemory::open(int bufferFd, int signalFd)
{
    PLOGI("bufferFd %d, signalFd %d", bufferFd, signalFd);

    if (!pImpl_->open(bufferFd))
    {
        PLOGE("Fail to open");
        return false;
    }

    if (!pImpl_->attachSignal(signalFd))
    {
        PLOGE("Fail to attachSignal");
        return false;
    }

    return true;
}

bool CameraSharedMemory::read(unsigned char **ppData, size_t *pDataSize, unsigned char **ppMeta,
                              size_t *pMetaSize, unsigned char **ppExtra, size_t *pExtraSize,
                              unsigned char **ppSolution, size_t *pSolutionSize, int timeoutMs)
{
    PLOGD("timeout %d ms", timeoutMs);
    return pImpl_->read(ppData, pDataSize, ppMeta, pMetaSize, ppExtra, pExtraSize, ppSolution,
                        pSolutionSize, timeoutMs, false);
}

void CameraSharedMemory::close(void)
{
    PLOGI("");
    pImpl_->releaseAllSignals();
    pImpl_->close();
}
