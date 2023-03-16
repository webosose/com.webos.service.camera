// Copyright (c) 2019-2023 LG Electronics, Inc.
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

#ifndef CAMSHM_ZEROCOPY_H_
#define CAMSHM_ZEROCOPY_H_

#include <sys/shm.h>
#include <stddef.h> // size_t
#include "usrptr_handle.h" // struct buffer

typedef enum
{
  SHMEM_IS_OK                  = 0x0,
  SHMEM_FAILED                 = -1,
  SHMEM_IS_NULL                = -2,
  SHMEM_ERROR_COUNT_MISMATCH   = -3,
  SHMEM_ERROR_RANGE_OUT        = -4,
} SHMEM_STATUS_T;

typedef void *SHMEM_HANDLE;

class IPCSharedMemory0Copy
{
public:
  static IPCSharedMemory0Copy& getInstance ()
  {
    static IPCSharedMemory0Copy sharedMemoryInstance;
    return sharedMemoryInstance;
  }
  SHMEM_STATUS_T CreateShmemory(SHMEM_HANDLE *, key_t *, int, int, int);
  SHMEM_STATUS_T GetShmemoryBufferInfo(SHMEM_HANDLE, int, struct buffer[], struct buffer []);
  SHMEM_STATUS_T WriteHeader(SHMEM_HANDLE, int, size_t);
  SHMEM_STATUS_T CloseShmemory(SHMEM_HANDLE *);

  IPCSharedMemory0Copy (IPCSharedMemory0Copy const &)  = delete;
  void operator = (IPCSharedMemory0Copy const &)  = delete;

private:
  IPCSharedMemory0Copy (){}
};

#endif // CAMSHM_ZEROCOPY_H_
