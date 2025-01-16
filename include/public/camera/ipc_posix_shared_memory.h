// Copyright (c) 2021 LG Electronics, Inc.
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

#ifndef CAMPOSHM_H_
#define CAMPOSHM_H_

#include "camera_hal_types.h"
#include <stddef.h>
#include <string>
#include <sys/shm.h>

typedef enum
{
    PSHMEM_IS_OK                = 0x0,
    PSHMEM_FAILED               = -1,
    PSHMEM_IS_NULL              = -2,
    PSHMEM_ERROR_COUNT_MISMATCH = -3,
    PSHMEM_ERROR_RANGE_OUT      = -4,
    PSHMEM_ERROR_MUNMAP_FAIL    = -5,
    PSHMEM_ERROR_UNLINK_FAIL    = -6
} PSHMEM_STATUS_T;

typedef void *PSHMEM_HANDLE;

class IPCPosixSharedMemory
{
public:
    static IPCPosixSharedMemory &getInstance()
    {
        static IPCPosixSharedMemory sharedMemoryInstance;
        return sharedMemoryInstance;
    }
    PSHMEM_STATUS_T CreateShmemory(PSHMEM_HANDLE *, int, int, int, int, int *, std::string *);
    PSHMEM_STATUS_T WriteShmemory(PSHMEM_HANDLE, unsigned char *, int, const char *, int,
                                  unsigned char *, int);
    PSHMEM_STATUS_T GetShmemoryBufferInfo(PSHMEM_HANDLE, int, buffer_t[], buffer_t[]);
    PSHMEM_STATUS_T WriteHeader(PSHMEM_HANDLE, int, size_t);
    PSHMEM_STATUS_T WriteMeta(PSHMEM_HANDLE, const char *, size_t);
    PSHMEM_STATUS_T WriteExtra(PSHMEM_HANDLE, unsigned char *, size_t);
    PSHMEM_STATUS_T IncrementWriteIndex(PSHMEM_HANDLE);
    PSHMEM_STATUS_T CloseShmemory(PSHMEM_HANDLE *, int, int, int, int, std::string, int);
    PSHMEM_STATUS_T ReadShmemory(PSHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize);
    int GetWriteIndex(PSHMEM_HANDLE);

    IPCPosixSharedMemory(IPCPosixSharedMemory const &) = delete;
    void operator=(IPCPosixSharedMemory const &)       = delete;

private:
    IPCPosixSharedMemory() {}
};

#endif // CAMPOSHM_H_
