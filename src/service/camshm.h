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

#include <sys/shm.h>

typedef enum
{
  SHMEM_COMM_OK = 0x0,
  SHMEM_COMM_FAIL = -1,
  SHMEM_COMM_OVERFLOW = -2,
  SHMEM_COMM_NODATA = -3,
  SHMEM_COMM_TERMINATE = -4,
} SHMEM_STATUS_T;

typedef void *SHMEM_HANDLE;

class IPCSharedMemory
{
public:
  static IPCSharedMemory& getInstance ()
  {
    static IPCSharedMemory sharedMemoryInstance;
    return sharedMemoryInstance;
  }
  SHMEM_STATUS_T CreateShmemory(SHMEM_HANDLE *, key_t *, int, int, int);
  SHMEM_STATUS_T WriteShmemory(SHMEM_HANDLE, unsigned char *, int, unsigned char *, int);
  SHMEM_STATUS_T CloseShmemory(SHMEM_HANDLE *);

  IPCSharedMemory (IPCSharedMemory const &)  = delete;
  void operator = (IPCSharedMemory const &)  = delete;

private:
  IPCSharedMemory (){}
};

#endif // CAMSHM_H_