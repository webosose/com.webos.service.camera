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
#include <string>
#include <vector>

class CameraSharedMemoryImpl;
class CameraSharedMemoryEx
{
public:
    CameraSharedMemoryEx();
    ~CameraSharedMemoryEx();

    int create(const std::string name, size_t dataSize, size_t metaSize, size_t extraSize,
               size_t solutionSize, size_t bufferCount);
    bool open(int fd);
    int open(const std::string name);
    void close(void);
    bool incrementWriteIndex(void);
    bool writeHeader(int index, size_t dataSize);
    bool getBufferList(std::vector<void *> *pDataList, std::vector<void *> *pMetaList,
                       std::vector<void *> *pExtraList, std::vector<void *> *pSolutionList);
    bool getBufferInfo(size_t *bufferCount, size_t *dataSize, size_t *metaSize, size_t *extraSize,
                       size_t *solutionSize);
    bool read(unsigned char **ppData, size_t *pDataSize, unsigned char **ppMeta = nullptr,
              size_t *pMetaSize = nullptr, unsigned char **ppExtra = nullptr,
              size_t *pExtraSize = nullptr, unsigned char **ppSolution = nullptr,
              size_t *pSolutionSize = nullptr, int timeoutMs = 1000);
    bool write(const unsigned char *pData, size_t dataSize, const unsigned char *pMeta,
               size_t metaSize, const unsigned char *pExtra, size_t extraSize,
               const unsigned char *pSolution, size_t solutionSize);

    int createSignal(const std::string &name = std::string("default"));
    bool notifySignal(void);
    bool waitForSignal(int timeoutMs = 1000, const std::string &name = std::string("default"));
    bool attachSignal(int fd, const std::string &name = std::string("default"));
    bool detachSignal(const std::string &name);
    void releaseAllSignals(void);
    int getWriteIndex(void);

private:
    std::unique_ptr<CameraSharedMemoryImpl> pImpl_;
};
