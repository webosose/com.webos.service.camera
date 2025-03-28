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

#pragma once

#include <memory>

class CameraSharedMemoryImpl;
class CameraSharedMemory
{
public:
    CameraSharedMemory();
    ~CameraSharedMemory();

    bool open(int bufferFd, int signalFd);
    bool read(unsigned char **ppData, size_t *pDataSize, unsigned char **ppMeta, size_t *pMetaSize,
              unsigned char **ppExtra, size_t *pExtraSize, unsigned char **ppSolution,
              size_t *pSolutionSize, int timeoutMs = 10000);
    void close(void);

private:
    std::unique_ptr<CameraSharedMemoryImpl> pImpl_;
};
