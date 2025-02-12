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

#include <cstddef>
#include <map>
#include <mutex>
#include <pthread.h>
#include <string>
#include <vector>

#pragma pack(push, 4)
struct ShmHeader
{
    int writeIndex;
    size_t bufferCount;
    size_t dataSize;
    size_t metaSize;
    size_t extraSize;
    size_t solutionSize;
};

struct ShmBuffer
{
    size_t *pDataSize;
    unsigned char *pData;
    size_t *pMetaSize;
    unsigned char *pMeta;
    size_t *pExtraSize;
    unsigned char *pExtra;
    size_t *pSolutionSize;
    unsigned char *pSolution;
};
#pragma pack(pop)

class CameraSharedMemoryImpl
{
public:
    CameraSharedMemoryImpl();
    ~CameraSharedMemoryImpl();

    int create(const std::string name, size_t dataSize, size_t metaSize, size_t extraSize,
               size_t solutionSize, size_t bufferCount);
    bool open(int fd);
    int open(const std::string name);
    void close(void);

    bool incrementWriteIndex(void);
    bool writeHeader(int index, size_t dataSize);
    bool getBufferList(std::vector<void *> *pDataList, std::vector<void *> *pMetaList,
                       std::vector<void *> *pExtraList, std::vector<void *> *pSolutionList);
    bool getBufferInfo(size_t *pBufferCount, size_t *pDataSize, size_t *pMetaSize,
                       size_t *pExtraSize, size_t *pSolutionSize);
    bool read(unsigned char **ppData, size_t *pDataSize, unsigned char **ppMeta, size_t *pMetaSize,
              unsigned char **ppExtra, size_t *pExtraSize, unsigned char **ppSolution,
              size_t *pSolutionSize, int timeoutMs, bool skipSignal);
    bool write(const unsigned char *pData, size_t dataSize, const unsigned char *pMeta,
               size_t metaSize, const unsigned char *pExtra, size_t extraSize,
               const unsigned char *pSolution, size_t solutionSize);

    int createSignal(const std::string &name = std::string("default"));
    bool notifySignal(void);
    bool waitForSignal(int timeoutMs = 10000, const std::string &name = std::string("default"));
    bool attachSignal(int fd, const std::string &name = std::string("default"));
    bool detachSignal(const std::string &name = std::string("default"));
    void releaseAllSignals(void);
    int getWriteIndex(void);

private:
    bool initShmem(int fd);
    void initBuffers(void);
    void printShmHeader(void);
    bool readData(unsigned char **ppData, size_t *pDataSize, unsigned char **ppMeta,
                  size_t *pMetaSize, unsigned char **ppExtra, size_t *pExtraSize,
                  unsigned char **ppSolution, size_t *pSolutionSize);

private:
    std::mutex m_;
    bool isCreated_{false};
    int shmFd_;
    std::string shmName_{""};
    void *shmAddr_;
    size_t shmSize_;
    ShmHeader *shmHeader_;
    std::vector<ShmBuffer> shmBuffers_;

    uint64_t eventValue_{0};
    std::map<std::string, int> signalFdMap_;
};
