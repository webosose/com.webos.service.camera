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

typedef enum
{
  POSHMEM_COMM_OK        = 0x0,
  POSHMEM_COMM_FAIL      = -1,
  POSHMEM_COMM_OVERFLOW  = -2,
  POSHMEM_COMM_NODATA    = -3,
  POSHMEM_COMM_TERMINATE = -4,
} POSHMEM_STATUS_T;

typedef void *SHMEM_HANDLE;

class IPCPosixSharedMemory
{
public:
  static IPCPosixSharedMemory& getInstance ()
  {
    static IPCPosixSharedMemory sharedMemoryInstance;
    return sharedMemoryInstance;
  }
  POSHMEM_STATUS_T CreatePosixShmemory(SHMEM_HANDLE *, int, int, int, int *);
  POSHMEM_STATUS_T WritePosixShmemory(SHMEM_HANDLE, unsigned char *, int, unsigned char *, int);
  POSHMEM_STATUS_T ClosePosixShmemory(SHMEM_HANDLE *, int, int, int);

  IPCPosixSharedMemory (IPCPosixSharedMemory const &)  = delete;
  void operator = (IPCPosixSharedMemory const &)  = delete;

private:
  IPCPosixSharedMemory (){}
};

#endif // CAMPOSHM_H_

