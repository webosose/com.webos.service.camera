// Copyright (c) 2019-2021 LG Electronics, Inc.
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

#include "camshm.h"
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "camera_types.h"

#define SHMEM_COMM_DEBUG
#ifdef SHMEM_COMM_DEBUG
#define DEBUG_PRINT(fmt, args...)                                                                  \
  PMLOG_INFO(CONST_MODULE_SHM, fmt, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

// constants

#define CAMSHKEY 7010
#define SHMEM_HEADER_SIZE (5 * sizeof(int))
#define SHMEM_LENGTH_SIZE sizeof(int)

enum
{
    MODE_OPEN, MODE_CREATE
};

enum
{
    READ_FIRST, READ_LAST
};

typedef enum
{
  SHMEM_COMM_MARK_NORMAL = 0x0,
  SHMEM_COMM_MARK_RESET = 0x1,
  SHMEM_COMM_MARK_TERMINATE = 0x2
} SHMEM_MARK_T;

/* shared memory structure
 4 bytes          : write_index
 4 bytes          : read_index
 4 bytes          : unit_size
 4 bytes          : unit_num
 4 bytes          : mark
 4 bytes  *unit_num : length data
 unit_size*unit_num : data
 4 bytes         : extra_size
 extra_size*unit_num : extra data
 */

typedef struct
{
  int shmem_id;
  int sema_id;

  /*shared memory overhead*/
  int *write_index;
  int *read_index;
  int *unit_size;
  int *unit_num;
  SHMEM_MARK_T *mark;

  unsigned int *length_buf;
  unsigned char *data_buf;

  int *extra_size;
  unsigned char *extra_buf;
} SHMEM_COMM_T;

SHMEM_STATUS_T IPCSharedMemory::CreateShmemory(SHMEM_HANDLE *phShmem, key_t *pShmemKey,
                                               int unitSize, int unitNum, int extraSize)
{
  *phShmem = (SHMEM_HANDLE)calloc(1, sizeof(SHMEM_COMM_T));
  SHMEM_COMM_T *pShmemBuffer = (SHMEM_COMM_T *)*phShmem;

  DEBUG_PRINT("hShmem = %p, pKey = %p, unitSize=%d, unitNum=%d\n", *phShmem,
              pShmemKey, unitSize, unitNum);

  key_t shmemKey;
  int shmemMode = 0666;
  for (shmemKey = CAMSHKEY; shmemKey < 0xFFFF; shmemKey++)
  {
    pShmemBuffer->shmem_id = shmget((key_t)shmemKey, 0, shmemMode);
    if ( (pShmemBuffer->shmem_id == -1) && (errno == ENOENT) )
      break;
  }
  *pShmemKey = shmemKey;
  int shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum + sizeof(int) +
                extraSize * unitNum;
  shmemMode |= IPC_CREAT | IPC_EXCL;

  DEBUG_PRINT("shmem_key=%d\n", shmemKey);

  pShmemBuffer->shmem_id = shmget((key_t)shmemKey, shmemSize, shmemMode);
  if (pShmemBuffer->shmem_id == -1)
  {
    DEBUG_PRINT("Can't open shared memory: %s\n", strerror(errno));
    free(pShmemBuffer);
    return SHMEM_COMM_FAIL;
  }

  DEBUG_PRINT("shared memory created/opened successfully!\n");

  unsigned char *pSharedmem = (unsigned char *)shmat(pShmemBuffer->shmem_id, NULL, 0);
  pShmemBuffer->write_index = (int *)(pSharedmem);
  pShmemBuffer->read_index = (int *)(pSharedmem + sizeof(int));
  pShmemBuffer->unit_size = (int *)(pSharedmem + sizeof(int) * 2);
  pShmemBuffer->unit_num = (int *)(pSharedmem + sizeof(int) * 3);
  pShmemBuffer->mark = (SHMEM_MARK_T *)(pSharedmem + sizeof(int) * 4);
  pShmemBuffer->length_buf = (unsigned int *)(pSharedmem + sizeof(int) * 5);

  if ((pShmemBuffer->sema_id = semget(shmemKey, 1, shmemMode)) == -1)
  {
#ifdef SHMEM_COMM_DEBUG
      DEBUG_PRINT("Failed to create semaphore : %s\n", strerror(errno));
#endif
    if ((pShmemBuffer->sema_id = semget((key_t)shmemKey, 1, 0666)) == -1)
    {
      DEBUG_PRINT("Failed to get semaphore : %s\n", strerror(errno));
      free(pShmemBuffer);
      return SHMEM_COMM_FAIL;
    }
  }

  *pShmemBuffer->unit_size = unitSize;
  *pShmemBuffer->unit_num = unitNum;

  pShmemBuffer->data_buf =
      pSharedmem + SHMEM_HEADER_SIZE + SHMEM_LENGTH_SIZE * (*pShmemBuffer->unit_num);

  struct shmid_ds shm_stat;
  if (-1 != shmctl(pShmemBuffer->shmem_id, IPC_STAT, &shm_stat))
  {
#ifdef SHMEM_COMM_DEBUG
    DEBUG_PRINT("shm_stat.shm_nattch=%d\n", (int)shm_stat.shm_nattch);
    if (shm_stat.shm_nattch == 1)
      DEBUG_PRINT("we are the first client\n");

    DEBUG_PRINT("shared memory size = %d\n", shm_stat.shm_segsz);
#endif
    // shared momory size larger than total, we use extra data
    if (shm_stat.shm_segsz > (SHMEM_HEADER_SIZE + (*pShmemBuffer->unit_size + SHMEM_LENGTH_SIZE) *
                                                     (*pShmemBuffer->unit_num)))
    {
      pShmemBuffer->extra_size =
          (int *)(pSharedmem + SHMEM_HEADER_SIZE +
                  (*pShmemBuffer->unit_size + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num));
      pShmemBuffer->extra_buf =
          (pSharedmem + SHMEM_HEADER_SIZE +
           (*pShmemBuffer->unit_size + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
           sizeof(int));
    }
    else
    {
      pShmemBuffer->extra_size = nullptr;
      pShmemBuffer->extra_buf = nullptr;
    }
  }

  if (pShmemBuffer->extra_size)
  {
    *pShmemBuffer->extra_size = extraSize;
  }

  *pShmemBuffer->mark = SHMEM_COMM_MARK_NORMAL;
  // Until the writter starts to write both write index and read index are
  // set to -1 . So the reader can get to know that the writter has not
  // started to write yet
  *pShmemBuffer->write_index = -1;
  *pShmemBuffer->read_index = -1;

  DEBUG_PRINT("unitSize = %d, SHMEM_LENGTH_SIZE = %d, unit_num = %d\n", *pShmemBuffer->unit_size,
              SHMEM_LENGTH_SIZE, *pShmemBuffer->unit_num);
  return SHMEM_COMM_OK;
}

SHMEM_STATUS_T IPCSharedMemory::WriteShmemory(SHMEM_HANDLE hShmem, unsigned char *pData, int dataSize,
                                              unsigned char *pExtraData, int extraDataSize)
{
  SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
  if (!shmem_buffer)
  {
    DEBUG_PRINT("shmem_buffer is NULL\n");
    return SHMEM_COMM_FAIL;
  }

  if (*shmem_buffer->write_index == -1)
  {
    *shmem_buffer->write_index = 0;
  }

  int mark = *shmem_buffer->mark;
  int unit_size = *shmem_buffer->unit_size;
  int unit_num = *shmem_buffer->unit_num;
  int lwrite_index = *shmem_buffer->write_index;
  if (extraDataSize > 0 && extraDataSize != *shmem_buffer->extra_size)
  {
    DEBUG_PRINT("extraDataSize should be same with extrasize used when open\n");
    return SHMEM_COMM_FAIL;
  }

  if (mark == SHMEM_COMM_MARK_RESET)
  {
    DEBUG_PRINT("warning - read process isn't reset yet!\n");
  }

  if ((dataSize == 0) || (dataSize > unit_size))
  {
    DEBUG_PRINT("size error(%d > %d)!\n", dataSize, unit_size);
    return SHMEM_COMM_FAIL;
  }

  // Once the writer writes the last buffer, it is made to point to the first
  // buffer again
  if (lwrite_index == unit_num)
  {
    DEBUG_PRINT("Overflow write data(write_index = %d, unit_num = %d)!\n", lwrite_index,
                *shmem_buffer->unit_num);
    *shmem_buffer->write_index = 0;
    return SHMEM_COMM_OVERFLOW;
  }

  *(int *)(shmem_buffer->length_buf + lwrite_index) = dataSize;
  memcpy(shmem_buffer->data_buf + lwrite_index * (*shmem_buffer->unit_size), pData, dataSize);

  if (pExtraData && extraDataSize > 0)
  {
    memcpy(shmem_buffer->extra_buf + lwrite_index * (*shmem_buffer->extra_size), pExtraData,
           extraDataSize);
  }

  *shmem_buffer->write_index += 1;
  if (*shmem_buffer->write_index == *shmem_buffer->unit_num)
    *shmem_buffer->write_index = 0;
  return SHMEM_COMM_OK;
}

SHMEM_STATUS_T IPCSharedMemory::CloseShmemory(SHMEM_HANDLE *phShmem)
{
  DEBUG_PRINT("CloseShmemory start");

  SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)*phShmem;
  if (!shmem_buffer)
  {
    DEBUG_PRINT("shmem_bufer is NULL\n");
    return SHMEM_COMM_FAIL;
  }

  void *shmem_addr = shmem_buffer->write_index;
  shmdt(shmem_addr);

  struct shmid_ds shm_stat;
  if (-1 != shmctl(shmem_buffer->shmem_id, IPC_STAT, &shm_stat))
  {
    DEBUG_PRINT("shm_stat.shm_nattch=%d\n", (int)shm_stat.shm_nattch);

    if (shm_stat.shm_nattch == 0)
    {
      DEBUG_PRINT("This is the only attached client\n");
      shmctl(shmem_buffer->shmem_id, IPC_RMID, NULL);
    }
  }

  free(shmem_buffer);
  shmem_buffer = nullptr;
  DEBUG_PRINT("CloseShmemory end");
  return SHMEM_COMM_OK;
}

SHMEM_STATUS_T IPCSharedMemory::OpenShmem(SHMEM_HANDLE *phShmem, key_t shmemKey)
{
    return _OpenShmem(phShmem, &shmemKey, 0, 0, 0, MODE_OPEN);
}

SHMEM_STATUS_T IPCSharedMemory::_OpenShmem(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize, int unitNum,
        int extraSize, int nOpenMode)
{
    SHMEM_COMM_T *pShmemBuffer;
    unsigned char *pSharedmem;
    key_t shmemKey;
    int shmemSize = 0;
    int shmemMode = 0666;
    struct shmid_ds shm_stat;

    *phShmem = (SHMEM_HANDLE) malloc(sizeof(SHMEM_COMM_T));
    pShmemBuffer = (SHMEM_COMM_T *) *phShmem;

    DEBUG_PRINT("hShmem = %p, pKey = %p, nOpenMode=%d, unitSize=%d, unitNum=%d\n",
            *phShmem, pShmemKey, nOpenMode, unitSize, unitNum);

    if (nOpenMode == MODE_CREATE)
    {
        for (shmemKey = CAMSHKEY; shmemKey < 0xFFFF; shmemKey++)
        {
            pShmemBuffer->shmem_id = shmget((key_t) shmemKey, 0, 0666);
            if (pShmemBuffer->shmem_id == -1 && errno == ENOENT)
                break;
        }
        *pShmemKey = shmemKey;
        shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum + sizeof(int)
                + extraSize * unitNum;
        shmemMode |= IPC_CREAT | IPC_EXCL;
    }
    else
    {
        shmemKey = *pShmemKey;
    }

    DEBUG_PRINT("shmem_key=%d\r\n", shmemKey);

    pShmemBuffer->shmem_id = shmget((key_t) shmemKey, shmemSize, shmemMode);
    if (pShmemBuffer->shmem_id == -1)
    {
        DEBUG_PRINT("Can't open shared memory: %s\n", strerror(errno));
        free(pShmemBuffer);
        return SHMEM_COMM_FAIL;
    }

    DEBUG_PRINT("shared memory created/opened successfully!\n");

    pSharedmem = (unsigned char *) shmat(pShmemBuffer->shmem_id, NULL, 0);
    pShmemBuffer->write_index = (int *) (pSharedmem);
    pShmemBuffer->read_index = (int *) (pSharedmem + sizeof(int));
    pShmemBuffer->unit_size = (int *) (pSharedmem + sizeof(int) * 2);
    pShmemBuffer->unit_num = (int *) (pSharedmem + sizeof(int) * 3);
    pShmemBuffer->mark = (SHMEM_MARK_T *) (pSharedmem + sizeof(int) * 4);
    pShmemBuffer->length_buf = (unsigned int *) (pSharedmem + sizeof(int) * 5);

    if (nOpenMode == MODE_CREATE)
    {
        *pShmemBuffer->unit_size = unitSize;
        *pShmemBuffer->unit_num = unitNum;
    }

    pShmemBuffer->data_buf = pSharedmem + SHMEM_HEADER_SIZE
            + SHMEM_LENGTH_SIZE * (*pShmemBuffer->unit_num);

    if (shmctl(pShmemBuffer->shmem_id, IPC_STAT, &shm_stat) != -1)
    {
#ifdef SHMEM_COMM_DEBUG
        DEBUG_PRINT("shm_stat.shm_nattch=%d\n", (int)shm_stat.shm_nattch);
        if (shm_stat.shm_nattch == 1)
        DEBUG_PRINT("we are the first client\n");

        DEBUG_PRINT("shared memory size = %d\n", shm_stat.shm_segsz);
#endif
        // shared momory size larger than total, we use extra data
        if (shm_stat.shm_segsz
                > SHMEM_HEADER_SIZE
                        + (*pShmemBuffer->unit_size + SHMEM_LENGTH_SIZE)
                                * (*pShmemBuffer->unit_num))
        {
            pShmemBuffer->extra_size = (int *) (pSharedmem + SHMEM_HEADER_SIZE
                    + (*pShmemBuffer->unit_size + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num));
            pShmemBuffer->extra_buf = (pSharedmem + SHMEM_HEADER_SIZE
                    + (*pShmemBuffer->unit_size + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num)
                    + sizeof(int));
        }
        else
        {
            pShmemBuffer->extra_size = NULL;
            pShmemBuffer->extra_buf = NULL;
        }
    }

    if (nOpenMode == MODE_CREATE && pShmemBuffer->extra_size != NULL)
    {
        *pShmemBuffer->extra_size = extraSize;
    }

    *pShmemBuffer->mark = SHMEM_COMM_MARK_NORMAL;
    //Until the writter starts to write both write index and read index are
    //set to -1 . So the reader can get to know that the writter has not
    //started to write yet
    *pShmemBuffer->write_index = -1;
    *pShmemBuffer->read_index = -1;

    DEBUG_PRINT("unitSize = %d, SHMEM_LENGTH_SIZE = %d, unit_num = %d\n", *pShmemBuffer->unit_size, SHMEM_LENGTH_SIZE, *pShmemBuffer->unit_num); DEBUG_PRINT("shared memory opened successfully! : shmem_id=%d, sema_id=%d\n",
            pShmemBuffer->shmem_id, pShmemBuffer->sema_id);
    return SHMEM_COMM_OK;
}

SHMEM_STATUS_T IPCSharedMemory::ReadShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize)
{
    return _ReadShmem(hShmem, ppData, pSize, NULL, NULL, READ_FIRST);
}

SHMEM_STATUS_T IPCSharedMemory::_ReadShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
        unsigned char **ppExtraData, int *pExtraSize, int readMode)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *) hShmem;
    int lread_index;
    unsigned char *read_addr;
    int size;
    static bool first_read;

    first_read = false;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem buffer is NULL");
        return SHMEM_COMM_FAIL;
    }
    lread_index = *shmem_buffer->write_index;

    do
    {
#ifdef SHMEM_COMM_DEBUG
        int sem_count;
        sem_count = semctl(shmem_buffer->sema_id, 0, GETVAL, 0);
        //DEBUG_PRINT("sem_count=%d\n", sem_count);
#endif
        if (-1 != *shmem_buffer->write_index)
        {
            if (*shmem_buffer->write_index == 0)
            {
                if (0 == first_read)
                {
                    first_read = 1;
                    continue;
                }
                else
                {
                    lread_index = *shmem_buffer->unit_num - 1;
                }
            }
            else
            {
                lread_index = *shmem_buffer->write_index - 1;
            }
            size = *(int*) (shmem_buffer->length_buf + lread_index);

            if ((size == 0) || (size > *shmem_buffer->unit_size))
            {
                DEBUG_PRINT("size error(%d)!\n", size);
                return SHMEM_COMM_FAIL;
            }

            read_addr = shmem_buffer->data_buf + (lread_index) * (*shmem_buffer->unit_size);
            *ppData = read_addr;
            *pSize = size;
            if (NULL != ppExtraData && NULL != pExtraSize)
            {
                *ppExtraData = shmem_buffer->extra_buf
                        + (lread_index) * (*shmem_buffer->extra_size);
                *pExtraSize = *shmem_buffer->extra_size;
            }
        }

        break;
    } while (1);

    return SHMEM_COMM_OK;
}
