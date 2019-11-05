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

typedef enum _SHMEM_STATUS_T
{
  SHMEM_COMM_OK = 0x0,
  SHMEM_COMM_FAIL = -1,
  SHMEM_COMM_OVERFLOW = -2,
  SHMEM_COMM_NODATA = -3,
  SHMEM_COMM_TERMINATE = -4,
} SHMEM_STATUS_T;

typedef void *SHMEM_HANDLE;

extern SHMEM_STATUS_T CreateShmemEx(SHMEM_HANDLE *, key_t *, int, int, int);
extern SHMEM_STATUS_T WriteShmemEx(SHMEM_HANDLE, unsigned char *, int, unsigned char *, int);
extern SHMEM_STATUS_T CloseShmem(SHMEM_HANDLE *);

#endif // CAMSHM_H_