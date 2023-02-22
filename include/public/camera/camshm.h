// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef CAMSHM_H_
#define CAMSHM_H_

#include "camera_hal_types.h" // buffer_t
#include <stddef.h>           // size_t
#include <sys/shm.h>

typedef enum
{
    SHMEM_IS_OK                = 0x0,
    SHMEM_FAILED               = -1,
    SHMEM_IS_NULL              = -2,
    SHMEM_ERROR_COUNT_MISMATCH = -3,
    SHMEM_ERROR_RANGE_OUT      = -4,
} SHMEM_STATUS_T;

typedef void *SHMEM_HANDLE;

class IPCSharedMemory
{
public:
    static IPCSharedMemory &getInstance()
    {
        static IPCSharedMemory sharedMemoryInstance;
        return sharedMemoryInstance;
    }
    SHMEM_STATUS_T CreateShmemory(SHMEM_HANDLE *, key_t *, int, int, int, int);
    SHMEM_STATUS_T WriteShmemory(SHMEM_HANDLE, unsigned char *, int, unsigned char *, int,
                                 unsigned char *, int);
    SHMEM_STATUS_T GetShmemoryBufferInfo(SHMEM_HANDLE, int, buffer_t[], buffer_t[]);
    SHMEM_STATUS_T WriteHeader(SHMEM_HANDLE, int, size_t);
    SHMEM_STATUS_T WriteMeta(SHMEM_HANDLE, unsigned char *, size_t);
    SHMEM_STATUS_T WriteExtra(SHMEM_HANDLE, unsigned char *, size_t);
    SHMEM_STATUS_T IncrementWriteIndex(SHMEM_HANDLE);
    SHMEM_STATUS_T CloseShmemory(SHMEM_HANDLE *);

    SHMEM_STATUS_T OpenShmem(SHMEM_HANDLE *phShmem, key_t shmemKey);
    SHMEM_STATUS_T ReadShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize);

    IPCSharedMemory(IPCSharedMemory const &) = delete;
    void operator=(IPCSharedMemory const &)  = delete;

private:
    IPCSharedMemory() {}
};

#endif // CAMSHM_H_
