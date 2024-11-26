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
#define LOG_TAG "CameraSharedMemoryImpl"
#include "camera_shared_memory_impl.h"
#include "camera_log.h"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <limits.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

CameraSharedMemoryImpl::CameraSharedMemoryImpl()
    : shmFd_(-1), shmAddr_(nullptr), shmSize_(0), shmHeader_(nullptr)
{
    PLOGI("");
}

CameraSharedMemoryImpl::~CameraSharedMemoryImpl()
{
    PLOGI("");
    releaseAllSignals();
    close();
}

int CameraSharedMemoryImpl::create(const std::string name, size_t dataSize, size_t metaSize,
                                   size_t extraSize, size_t solutionSize, size_t bufferCount)
{
    PLOGI("name(%s) data(%zu) meta(%zu) extra(%zu) solution(%zu) count(%zu)", name.c_str(),
          dataSize, metaSize, extraSize, solutionSize, bufferCount);

    std::lock_guard<std::mutex> lock(m_);

    shmFd_ =
        shm_open(name.c_str(), O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    if (shmFd_ == -1)
    {
        PLOGE("shm_open failed : %s", name.c_str());
        return -1;
    }
    isCreated_ = true;

    size_t headerSize      = sizeof(ShmHeader);
    size_t dataSectionSize = dataSize + metaSize + extraSize + solutionSize + sizeof(size_t) * 4;
    shmSize_               = headerSize + bufferCount * dataSectionSize;
    PLOGI("headerSize(%zu) dataSectionSize(%zu) shmSize(%zu)", headerSize, dataSectionSize,
          shmSize_);
    if (ftruncate(shmFd_, shmSize_) == -1)
    {
        PLOGE("ftruncate failed");
        ::close(shmFd_);
        shmFd_ = -1;
        return -1;
    }

    shmAddr_ = mmap(NULL, shmSize_, PROT_READ | PROT_WRITE, MAP_SHARED, shmFd_, 0);
    if (shmAddr_ == MAP_FAILED)
    {
        PLOGE("mmap failed");
        shmAddr_ = nullptr;
        ::close(shmFd_);
        shmFd_ = -1;
        return -1;
    }

    struct stat sb;
    if (fstat(shmFd_, &sb) == -1)
    {
        PLOGE("fstat failed");
        ::close(shmFd_);
        shmFd_ = -1;
        return -1;
    }
    size_t stSize_ = sb.st_size;
    PLOGI("st_size(%zu)", stSize_);

    shmName_                 = name;
    shmHeader_               = static_cast<ShmHeader *>(shmAddr_);
    shmHeader_->writeIndex   = -1;
    shmHeader_->bufferCount  = bufferCount;
    shmHeader_->dataSize     = dataSize;
    shmHeader_->metaSize     = metaSize;
    shmHeader_->extraSize    = extraSize;
    shmHeader_->solutionSize = solutionSize;

    initBuffers();
    for (auto &buffer : shmBuffers_)
    {
        *buffer.pDataSize     = 0;
        *buffer.pMetaSize     = 0;
        *buffer.pExtraSize    = 0;
        *buffer.pSolutionSize = 0;
    }

    PLOGI("fd(%d)", shmFd_);
    return shmFd_;
}

int CameraSharedMemoryImpl::open(const std::string name)
{
    PLOGI("name(%s)", name.c_str());

    std::lock_guard<std::mutex> lock(m_);

    int fd = shm_open(name.c_str(), O_RDWR, 0666);
    if (fd == -1)
    {
        PLOGE("shm_open failed : %s", name.c_str());
        return -1;
    }

    if (!initShmem(fd))
    {
        PLOGE("initShmem Fail!");
        return -1;
    }

    initBuffers();

    PLOGI("fd(%d)", fd);
    return fd;
}

bool CameraSharedMemoryImpl::open(int fd)
{
    PLOGI("fd(%d)", fd);

    std::lock_guard<std::mutex> lock(m_);

    if (!initShmem(fd))
    {
        PLOGE("initShmem Fail!");
        return false;
    }

    initBuffers();

    PLOGI("end");
    return true;
}

bool CameraSharedMemoryImpl::initShmem(int fd)
{
    PLOGI("fd(%d)", fd);

    struct stat sb;
    if (fstat(fd, &sb) == -1)
    {
        PLOGE("Failed to get size of shared memory");
        return false;
    }
    size_t shmSize_ = sb.st_size;
    PLOGI("shm size %zu", shmSize_);

    shmAddr_ = (ShmHeader *)mmap(0, shmSize_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shmAddr_ == MAP_FAILED)
    {
        PLOGE("shmem mmap fail!");
        shmAddr_ = nullptr;
        return false;
    }

    shmHeader_ = static_cast<ShmHeader *>(shmAddr_);
    shmFd_     = fd;

    printShmHeader();
    return true;
}

void CameraSharedMemoryImpl::initBuffers(void)
{
    size_t headerSize      = sizeof(ShmHeader);
    size_t dataSectionSize = shmHeader_->dataSize + shmHeader_->metaSize + shmHeader_->extraSize +
                             shmHeader_->solutionSize + sizeof(size_t) * 4;

    shmBuffers_.resize(shmHeader_->bufferCount);
    for (size_t i = 0; i < shmHeader_->bufferCount; ++i)
    {
        unsigned char *base =
            static_cast<unsigned char *>(shmAddr_) + headerSize + i * dataSectionSize;

        shmBuffers_[i].pDataSize = reinterpret_cast<size_t *>(base);
        shmBuffers_[i].pData =
            reinterpret_cast<unsigned char *>(shmBuffers_[i].pDataSize) + sizeof(size_t);
        shmBuffers_[i].pMetaSize =
            reinterpret_cast<size_t *>(shmBuffers_[i].pData + shmHeader_->dataSize);
        shmBuffers_[i].pMeta =
            reinterpret_cast<unsigned char *>(shmBuffers_[i].pMetaSize) + sizeof(size_t);
        shmBuffers_[i].pExtraSize =
            reinterpret_cast<size_t *>(shmBuffers_[i].pMeta + shmHeader_->metaSize);
        shmBuffers_[i].pExtra =
            reinterpret_cast<unsigned char *>(shmBuffers_[i].pExtraSize) + sizeof(size_t);
        shmBuffers_[i].pSolutionSize =
            reinterpret_cast<size_t *>(shmBuffers_[i].pExtra + shmHeader_->extraSize);
        shmBuffers_[i].pSolution =
            reinterpret_cast<unsigned char *>(shmBuffers_[i].pSolutionSize) + sizeof(size_t);
    }
}

void CameraSharedMemoryImpl::printShmHeader(void)
{
    PLOGI("bufferCount  : %zu", shmHeader_->bufferCount);
    PLOGI("writeIndex   : %d", shmHeader_->writeIndex);
    PLOGI("dataSize     : %zu", shmHeader_->dataSize);
    PLOGI("metaSize     : %zu", shmHeader_->metaSize);
    PLOGI("extraSize    : %zu", shmHeader_->extraSize);
    PLOGI("solutionSize : %zu", shmHeader_->solutionSize);
}

void CameraSharedMemoryImpl::close(void)
{
    PLOGI("start");

    std::lock_guard<std::mutex> lock(m_);

    if (shmHeader_)
    {
        shmHeader_ = nullptr;
    }
    if (shmAddr_)
    {
        munmap(shmAddr_, shmSize_);
        shmAddr_ = nullptr;
    }
    if (shmFd_ != -1)
    {
        ::close(shmFd_);
        shmFd_ = -1;
    }
    if (isCreated_ && !shmName_.empty())
    {
        shm_unlink(shmName_.c_str());
        shmName_ = "";
    }
    isCreated_ = false;

    PLOGI("end");
}

bool CameraSharedMemoryImpl::incrementWriteIndex(void)
{
    std::lock_guard<std::mutex> lock(m_);

    if (!shmHeader_)
    {
        PLOGE("shmHeader_ is NULL");
        return false;
    }

    if (shmHeader_->writeIndex < INT_MAX)
        shmHeader_->writeIndex++;
    if (shmHeader_->writeIndex >= (int)shmHeader_->bufferCount)
        shmHeader_->writeIndex = 0;

    PLOGD("writeIndex(%d)", shmHeader_->writeIndex);
    return true;
}

bool CameraSharedMemoryImpl::writeHeader(int index, size_t dataSize)
{
    PLOGD("index(%d) dataSize(%zu)", index, dataSize);

    std::lock_guard<std::mutex> lock(m_);

    if (!shmHeader_)
    {
        PLOGE("shmHeader_ is NULL");
        return false;
    }

    if (index < 0 || index >= (int)shmHeader_->bufferCount)
    {
        PLOGE("index is out of range (%d)", index);
        return false;
    }

    shmHeader_->writeIndex        = index;
    *shmBuffers_[index].pDataSize = dataSize;

    return true;
}

bool CameraSharedMemoryImpl::getBufferList(std::vector<void *> *pDataList,
                                           std::vector<void *> *pMetaList,
                                           std::vector<void *> *pExtraList,
                                           std::vector<void *> *pSolutionList)
{
    PLOGI("");

    std::lock_guard<std::mutex> lock(m_);

    if (!shmHeader_)
    {
        PLOGE("shmHeader_ is NULL");
        return false;
    }

    for (auto &buffer : shmBuffers_)
    {
        if (pDataList)
            pDataList->push_back(reinterpret_cast<void *>(buffer.pData));
        if (pMetaList)
            pMetaList->push_back(reinterpret_cast<void *>(buffer.pMeta));
        if (pExtraList)
            pExtraList->push_back(reinterpret_cast<void *>(buffer.pExtra));
        if (pSolutionList)
            pSolutionList->push_back(reinterpret_cast<void *>(buffer.pSolution));
    }

    return true;
}

bool CameraSharedMemoryImpl::getBufferInfo(size_t *pBufferCount, size_t *pDataSize,
                                           size_t *pMetaSize, size_t *pExtraSize,
                                           size_t *pSolutionSize)
{
    PLOGI("");

    std::lock_guard<std::mutex> lock(m_);

    if (!shmHeader_)
    {
        PLOGE("shmHeader_ is NULL");
        return false;
    }

    if (pBufferCount)
        *pBufferCount = shmHeader_->bufferCount;
    if (pDataSize)
        *pDataSize = shmHeader_->dataSize;
    if (pMetaSize)
        *pMetaSize = shmHeader_->metaSize;
    if (pExtraSize)
        *pExtraSize = shmHeader_->extraSize;
    if (pSolutionSize)
        *pSolutionSize = shmHeader_->solutionSize;

    return true;
}

bool CameraSharedMemoryImpl::read(unsigned char **ppData, size_t *pDataSize, unsigned char **ppMeta,
                                  size_t *pMetaSize, unsigned char **ppExtra, size_t *pExtraSize,
                                  unsigned char **ppSolution, size_t *pSolutionSize, int timeoutMs)
{
    PLOGD("timeout %d ms", timeoutMs);

    if (!waitForSignal(timeoutMs))
    {
        PLOGE("waitForSignal() fail");
        return false;
    }

    const int maxRetries = 100;
    for (int retry = 0; retry <= maxRetries; retry++)
    {
        if (readData(ppData, pDataSize, ppMeta, pMetaSize, ppExtra, pExtraSize, ppSolution,
                     pSolutionSize))
        {
            PLOGD("read done! data(%p) length(%zu)", *ppData, *pDataSize);
            return true;
        }

        usleep(10000);
        PLOGI("readData Fail! retry(%d/%d)", retry, maxRetries);
    }

    return false;
}

bool CameraSharedMemoryImpl::readData(unsigned char **ppData, size_t *pDataSize,
                                      unsigned char **ppMeta, size_t *pMetaSize,
                                      unsigned char **ppExtra, size_t *pExtraSize,
                                      unsigned char **ppSolution, size_t *pSolutionSize)
{
    std::lock_guard<std::mutex> lock(m_);

    if (!shmHeader_)
    {
        PLOGE("shmHeader_ is NULL");
        return false;
    }

    if (shmHeader_->writeIndex == -1)
    {
        PLOGE("No data has been written yet.");
        return false;
    }

    int readIndex =
        (shmHeader_->writeIndex + shmHeader_->bufferCount - 1) % shmHeader_->bufferCount;
    PLOGD("writeIndex(%d) readIndex(%d)", shmHeader_->writeIndex, readIndex);
    const ShmBuffer &buffer = shmBuffers_[readIndex];

    if (ppData)
        *ppData = buffer.pData;
    if (pDataSize)
        *pDataSize = *buffer.pDataSize;
    if (ppMeta)
        *ppMeta = buffer.pMeta;
    // metaSize     = *buffer.pMetaSize;
    if (pMetaSize)
        *pMetaSize = shmHeader_->metaSize;
    if (ppExtra)
        *ppExtra = buffer.pExtra;
    // extraSize    = *buffer.pExtraSize;
    if (pExtraSize)
        *pExtraSize = shmHeader_->extraSize;
    if (ppSolution)
        *ppSolution = buffer.pSolution;
    // solutionSize = *buffer.pSolutionSize;
    if (pSolutionSize)
        *pSolutionSize = shmHeader_->solutionSize;

    return true;
}

bool CameraSharedMemoryImpl::write(const unsigned char *pData, size_t dataSize,
                                   const unsigned char *pMeta, size_t metaSize,
                                   const unsigned char *pExtra, size_t extraSize,
                                   const unsigned char *pSolution, size_t solutionSize)
{
    std::lock_guard<std::mutex> lock(m_);

    if (!shmHeader_)
    {
        PLOGE("shmHeader_ is NULL");
        return false;
    }

    if (shmHeader_->writeIndex < 0) // first write
        shmHeader_->writeIndex = 0;
    ShmBuffer &buffer = shmBuffers_[shmHeader_->writeIndex];

    if (pData)
    {
        *buffer.pDataSize = dataSize;
        memcpy(buffer.pData, pData, dataSize);
    }

    if (pMeta)
    {
        *buffer.pMetaSize = metaSize;
        memcpy(buffer.pMeta, pMeta, metaSize);
    }

    if (pExtra)
    {
        *buffer.pExtraSize = extraSize;
        memcpy(buffer.pExtra, pExtra, extraSize);
    }

    if (pSolution)
    {
        *buffer.pSolutionSize = solutionSize;
        memcpy(buffer.pSolution, pSolution, solutionSize);
    }

    shmHeader_->writeIndex = (shmHeader_->writeIndex + 1) % shmHeader_->bufferCount;

    return true;
}

int CameraSharedMemoryImpl::createSignal(const std::string &name)
{
    PLOGI("name(%s)", name.c_str());

    int efd = eventfd(0, EFD_NONBLOCK);
    if (efd == -1)
    {
        PLOGE("Fail to create eventfd");
        return -1;
    }

    if (!attachSignal(efd, name))
    {
        PLOGE("Fail to attach");
        return -1;
    }

    PLOGI("eventfd(%d)", efd);
    return efd;
}

bool CameraSharedMemoryImpl::attachSignal(int fd, const std::string &name)
{
    PLOGI("fd(%d) name(%s)", fd, name.c_str());

    std::lock_guard<std::mutex> lock(m_);

    if (fcntl(fd, F_GETFD) == -1)
    {
        PLOGE("invalid fd %d", fd);
        return false;
    }

    signalFdMap_[name] = fd;
    PLOGI("attached fd(%d)", fd);
    return true;
}

bool CameraSharedMemoryImpl::notifySignal(void)
{
    std::lock_guard<std::mutex> lock(m_);

    PLOGD("eventValue_ %llu", (unsigned long long)eventValue_);
    for (const auto &[name, fd] : signalFdMap_)
    {
        if (::write(fd, &eventValue_, sizeof(eventValue_)) != sizeof(eventValue_))
        {
            PLOGE("efd write error, fd %d", fd);
        }
    }

    ++eventValue_;
    return true;
}

bool CameraSharedMemoryImpl::detachSignal(const std::string &name)
{
    PLOGI("name(%s)", name.c_str());

    std::lock_guard<std::mutex> lock(m_);

    if (signalFdMap_.find(name) != signalFdMap_.end())
    {
        PLOGI("detached fd(%d)", signalFdMap_[name]);
        ::close(signalFdMap_[name]);
        signalFdMap_.erase(name);
    }

    return true;
}

void CameraSharedMemoryImpl::releaseAllSignals(void)
{
    PLOGI("");

    std::lock_guard<std::mutex> lock(m_);

    for (auto it = signalFdMap_.begin(); it != signalFdMap_.end();)
    {
        PLOGI("released fd(%d), name(%s)", it->second, it->first.c_str());
        ::close(it->second);
        it = signalFdMap_.erase(it);
    }
}

bool CameraSharedMemoryImpl::waitForSignal(int timeoutMs, const std::string &name)
{
    std::unique_lock<std::mutex> lock(m_);

    if (signalFdMap_.find(name) == signalFdMap_.end())
    {
        PLOGE("unknown signal name(%s)", name.c_str());
        return false;
    }
    int efd = signalFdMap_[name];

    struct pollfd fds = {efd, POLLIN, 0};

    PLOGD("start waiting for eventfd (timeout %d ms)", timeoutMs);
    int ret = poll(&fds, 1, timeoutMs);
    if (ret == -1)
    {
        PLOGE("Poll failure: %s", strerror(errno));
        return false;
    }
    else if (ret == 0)
    {
        PLOGE("Timeout! No data available to read from eventfd");
        return false;
    }

    if (!(fds.revents & POLLIN))
    {
        PLOGE("POLLIN event did not occur!");
        return false;
    }

    // lock.unlock(); static issue impact: Medium. desc: Double unlock(LOCK)

    uint64_t value;
    for (int retry = 0; retry < 100; ++retry)
    {
        if (::read(efd, &value, sizeof(value)) == sizeof(value))
        {
            PLOGD("Read %llu from eventfd", (unsigned long long)value);
            return true;
        }
        PLOGE("Read failure, retrying...");

        lock.unlock(); // Unlock the mutex before sleeping
        usleep(1000);
        lock.lock(); // Re-lock the mutex after sleeping
    }

    PLOGE("Read retry failed!");
    return false;
}

int CameraSharedMemoryImpl::getWriteIndex(void)
{
    std::lock_guard<std::mutex> lock(m_);

    if (!shmHeader_)
    {
        PLOGE("shmHeader_ is NULL");
        return false;
    }

    PLOGD("writeIndex(%d)", shmHeader_->writeIndex);
    return shmHeader_->writeIndex;
}
