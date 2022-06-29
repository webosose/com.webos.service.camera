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

#include <sys/shm.h>
#include <string>
#include <stddef.h> // size_t
#include "camera_hal_if_types.h" // buffer_t

typedef enum
{
    PSHMEM_IS_OK                 = 0x0,
    PSHMEM_FAILED                = -1,
    PSHMEM_IS_NULL               = -2,
    PSHMEM_ERROR_COUNT_MISMATCH  = -3,
    PSHMEM_ERROR_RANGE_OUT       = -4,
    PSHMEM_ERROR_MUNMAP_FAIL     = -5,
    PSHMEM_ERROR_UNLINK_FAIL     = -6
} PSHMEM_STATUS_T;

typedef void *SHMEM_HANDLE;

class IPCPosixSharedMemory
{
public:
  static IPCPosixSharedMemory& getInstance ()
  {
    static IPCPosixSharedMemory sharedMemoryInstance;
    return sharedMemoryInstance;
  }
  PSHMEM_STATUS_T CreateShmemory(SHMEM_HANDLE *, int, int, int, int *, std::string *);
  PSHMEM_STATUS_T GetShmemoryBufferInfo(SHMEM_HANDLE, int, buffer_t[], buffer_t[]);
  PSHMEM_STATUS_T WriteHeader(SHMEM_HANDLE, int, size_t);
  PSHMEM_STATUS_T WriteExtra(SHMEM_HANDLE, unsigned char*, size_t);
  PSHMEM_STATUS_T CloseShmemory(SHMEM_HANDLE *, int, int, int, std::string, int);

  IPCPosixSharedMemory (IPCPosixSharedMemory const &)  = delete;
  void operator = (IPCPosixSharedMemory const &)  = delete;

private:
  IPCPosixSharedMemory (){}
};

#endif // CAMPOSHM_H_

