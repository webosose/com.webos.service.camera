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
#define LOG_TAG "CameraSharedMemoryEx"
#include "camera_shared_memory_ex.h"
#include "camera_log.h"
#include "camera_shared_memory_impl.h"

CameraSharedMemoryEx::CameraSharedMemoryEx() : pImpl_(std::make_unique<CameraSharedMemoryImpl>())
{
    PLOGI("");
}

CameraSharedMemoryEx::~CameraSharedMemoryEx() { PLOGI(""); }

int CameraSharedMemoryEx::create(const std::string name, size_t dataSize, size_t metaSize,
                                 size_t extraSize, size_t solutionSize, size_t bufferCount)
{
    return pImpl_->create(name, dataSize, metaSize, extraSize, solutionSize, bufferCount);
}

bool CameraSharedMemoryEx::open(int fd) { return pImpl_->open(fd); }

int CameraSharedMemoryEx::open(const std::string name) { return pImpl_->open(name); }

void CameraSharedMemoryEx::close(void) { return pImpl_->close(); }

bool CameraSharedMemoryEx::incrementWriteIndex(void) { return pImpl_->incrementWriteIndex(); }

bool CameraSharedMemoryEx::writeHeader(int index, size_t dataSize)
{
    return pImpl_->writeHeader(index, dataSize);
}

bool CameraSharedMemoryEx::getBufferList(std::vector<void *> *pDataList,
                                         std::vector<void *> *pMetaList,
                                         std::vector<void *> *pExtraList,
                                         std::vector<void *> *pSolutionList)
{
    return pImpl_->getBufferList(pDataList, pMetaList, pExtraList, pSolutionList);
}

bool CameraSharedMemoryEx::getBufferInfo(size_t *bufferCount, size_t *dataSize, size_t *metaSize,
                                         size_t *extraSize, size_t *solutionSize)
{
    return pImpl_->getBufferInfo(bufferCount, dataSize, metaSize, extraSize, solutionSize);
}

bool CameraSharedMemoryEx::read(unsigned char **ppData, size_t *pDataSize, unsigned char **ppMeta,
                                size_t *pMetaSize, unsigned char **ppExtra, size_t *pExtraSize,
                                unsigned char **ppSolution, size_t *pSolutionSize, int timeoutMs)
{
    return pImpl_->read(ppData, pDataSize, ppMeta, pMetaSize, ppExtra, pExtraSize, ppSolution,
                        pSolutionSize, timeoutMs);
}

bool CameraSharedMemoryEx::write(const unsigned char *pData, size_t dataSize,
                                 const unsigned char *pMeta, size_t metaSize,
                                 const unsigned char *pExtra, size_t extraSize,
                                 const unsigned char *pSolution, size_t solutionSize)
{
    return pImpl_->write(pData, dataSize, pMeta, metaSize, pExtra, extraSize, pSolution,
                         solutionSize);
}

int CameraSharedMemoryEx::createSignal(const std::string &name)
{
    return pImpl_->createSignal(name);
}

bool CameraSharedMemoryEx::notifySignal(void) { return pImpl_->notifySignal(); }

bool CameraSharedMemoryEx::waitForSignal(int timeoutMs, const std::string &name)
{
    return pImpl_->waitForSignal(timeoutMs, name);
}

bool CameraSharedMemoryEx::attachSignal(int fd, const std::string &name)
{
    return pImpl_->attachSignal(fd, name);
}

bool CameraSharedMemoryEx::detachSignal(const std::string &name)
{
    return pImpl_->detachSignal(name);
}

void CameraSharedMemoryEx::releaseAllSignals(void) { pImpl_->releaseAllSignals(); }
