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

#define LOG_TAG "IPCSharedMemory"
#include "ipc_shared_memory.h"
#include "camera_types.h"
#include <algorithm>
#include <errno.h>
#include <fcntl.h>
#include <iomanip>
#include <random>
#include <stdbool.h>
#include <stdexcept>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_set>
// constants

#define CAMSHKEY 7010
#define SIGNED_INT_MAX 2147483647
#define TEN_DIGIT_START_VALUE 1000000000
#define SHMEM_HEADER_SIZE ((int)(6 * sizeof(int)))
#define SHMEM_LENGTH_SIZE ((int)sizeof(int))
#define SHMEM_UNIT_SIZE_MAX 66355200 // 7680*4320*2
#define SHMEM_EXTRA_SIZE_MAX ((int)sizeof(unsigned int))
#define SHMEM_META_SIZE_MAX 4096 // 1024*4
#define SHMEM_UNIT_NUM_MAX 8

typedef enum
{
    SHMEM_COMM_MARK_NORMAL    = 0x0,
    SHMEM_COMM_MARK_RESET     = 0x1,
    SHMEM_COMM_MARK_TERMINATE = 0x2
} SHMEM_MARK_T;

/* shared memory structure
 4 bytes          : write_index
 4 bytes          : read_index
 4 bytes          : unit_size
 4 bytes          : meta_size
 4 bytes          : unit_num
 4 bytes          : mark
 4 bytes  *unit_num : length data
 unit_size*unit_num : data
 4 bytes  *unit_num : length meta
 meta_size*unit_num : meta
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
    int *meta_size;
    int *unit_num;
    SHMEM_MARK_T *mark;

    unsigned int *length_buf;
    unsigned char *data_buf;

    unsigned int *length_meta;
    unsigned char *data_meta;

    int *extra_size;
    unsigned char *extra_buf;
} SHMEM_COMM_T;

SHMEM_STATUS_T openShmem(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize, int unitNum,
                         int extraSize, int nOpenMode);
SHMEM_STATUS_T readShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                         unsigned char **ppExtraData, int *pExtraSize, int readMode);

key_t getShmemKey(void);

enum
{
    MODE_OPEN,
    MODE_CREATE
};

enum
{
    READ_FIRST,
    READ_LAST
};

// structure define

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
    struct seminfo *__buf; /* buffer for IPC_INFO (Linux-specific) */
};

//  << Shmem shape : frame_count : 8, extra_size : sizeof(int)) >>
//      +---------+---------+----------------
//      |         | 4 bytes | write_index
//      |         +---------+----------------
//      |         | 4 bytes | read_index
//      |HEADER   +---------+----------------
//      |24 bytes | 4 bytes | unit_size
//      |         +---------+----------------
//      |         | 4 bytes | meta_size
//      |         +---------+----------------
//      |         | 4 bytes | unit_num
//      |         +---------+----------------
//      |         | 4 bytes | mark
//      +---------+---------+---------------- (length_buf)
//      |         | 4 bytes | frame_size[0]
//      |         +---------+----------------
//      |LENGTH   | 4 bytes | ...
//      |32 bytes +---------+----------------
//      |         | 4 bytes | frame_size[7]
//      +---------+---------+---------------- (data_buf)
//      |         | x bytes | frame_buf[0]
//      |         +---------+----------------
//      |DATA     | x bytes | ...
//      |x*8 bytes+---------+----------------
//      |         | x bytes | frame_buf[7]
//      +---------+---------+---------------- (length_meta)
//      |         | 4 bytes | meta_size[0]
//      |         +---------+----------------
//      |LENGTH   | 4 bytes | ...
//      |32 bytes +---------+----------------
//      |         | 4 bytes | meta_size[7]
//      +---------+---------+---------------- (data_meta)
//      |         | y bytes | meta_buf[0]
//      |         +---------+----------------
//      |META     | y bytes | ...
//      |y*8 bytes+---------+----------------
//      |         | y bytes | meta_buf[7]
//      +---------+---------+----------------
//      |EXTRA SZ | 4 bytes | extra_size
//      +---------+---------+---------------- (extra_buf)
//      |         | 4 bytes | extra_buf[0]
//      |         +---------+----------------
//      |EXTRA BUF| 4 bytes | ...
//      |4*8 bytes+---------+----------------
//      |         | 4 bytes | extra_buf[7]
//      +---------+---------+----------------
//
// TOTAL = HEADER(24) +
//         LENGTH(sizeof(int) * unit_num) + DATA(unit_size * unit_num) +
//         LENGTH(sizeof(int) * unit_num) + DATA(meta_size * unit_num) +
//         EXTRA_SZ(sizeof(int)) + EXTRA_BUF(extra_size * unit_num))

unsigned long long random10()
{
    static std::string digits = "0123456789";
    static std::mt19937 rng(std::random_device{}());

    std::shuffle(digits.begin(), digits.end(), rng);
    return std::stoull(digits);
}

key_t getShmemKey()
{
    int mode                           = 0666;
    key_t shmemKey                     = 0;
    unsigned long long randomLongValue = 0;
    static std::unordered_set<unsigned long long> history;

    for (int count = CAMSHKEY; count < 0xFFFF; count++)
    {
        randomLongValue = random10();
        if (history.insert(randomLongValue).second)
        {
            if (randomLongValue > TEN_DIGIT_START_VALUE && randomLongValue < SIGNED_INT_MAX)
            {
                shmemKey = (key_t)randomLongValue;
                PLOGI("random key generation value =%llu", randomLongValue);
                if (shmget(shmemKey, 0, mode) == -1)
                    break;
            }
        }
    }
    return shmemKey;
}

SHMEM_STATUS_T IPCSharedMemory::CreateShmemory(SHMEM_HANDLE *phShmem, key_t *pShmemKey,
                                               int unitSize, int metaSize, int unitNum,
                                               int extraSize)
{
    *phShmem                   = (SHMEM_HANDLE)calloc(1, sizeof(SHMEM_COMM_T));
    SHMEM_COMM_T *pShmemBuffer = (SHMEM_COMM_T *)*phShmem;
    if (!pShmemBuffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return SHMEM_FAILED;
    }

    PLOGI("hShmem = %p, pKey = %p, unitSize=%d, metaSize=%d, unitNum=%d\n", *phShmem, pShmemKey,
          unitSize, metaSize, unitNum);

    int shmemMode  = 0666;
    key_t shmemKey = getShmemKey();

    *pShmemKey    = shmemKey;
    int shmemSize = 0;
    if (unitSize >= 0 && unitSize <= SHMEM_UNIT_SIZE_MAX && metaSize >= 0 &&
        metaSize <= SHMEM_META_SIZE_MAX && unitNum >= 0 && unitNum <= SHMEM_UNIT_NUM_MAX &&
        extraSize >= 0 && extraSize <= SHMEM_EXTRA_SIZE_MAX)
    {
        shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum +
                    (metaSize + SHMEM_LENGTH_SIZE) * unitNum + ((int)sizeof(int)) +
                    (extraSize)*unitNum;
    }
    else
    {
        PLOGE("shmemSize error\n");
    }

    shmemMode |= IPC_CREAT | IPC_EXCL;

    PLOGI("shmem_key=%d\n", shmemKey);

    pShmemBuffer->shmem_id = shmget((key_t)shmemKey, shmemSize, shmemMode);
    if (pShmemBuffer->shmem_id == -1)
    {
        PLOGE("Can't open shared memory: %s\n", strerror(errno));
        free(*phShmem);
        *phShmem = nullptr;
        return SHMEM_FAILED;
    }

    PLOGI("shared memory created/opened successfully!\n");

    unsigned char *pSharedmem = (unsigned char *)shmat(pShmemBuffer->shmem_id, NULL, 0);
    if (pSharedmem == NULL)
    {
        PLOGE("pSharedmem is NULL\n");
        free(*phShmem);
        *phShmem = nullptr;
        return SHMEM_FAILED;
    }
    pShmemBuffer->write_index = (int *)(pSharedmem + sizeof(int) * 0);
    pShmemBuffer->read_index  = (int *)(pSharedmem + sizeof(int) * 1);
    pShmemBuffer->unit_size   = (int *)(pSharedmem + sizeof(int) * 2);
    pShmemBuffer->meta_size   = (int *)(pSharedmem + sizeof(int) * 3);
    pShmemBuffer->unit_num    = (int *)(pSharedmem + sizeof(int) * 4);
    pShmemBuffer->mark        = (SHMEM_MARK_T *)(pSharedmem + sizeof(int) * 5);

    if ((pShmemBuffer->sema_id = semget(shmemKey, 1, shmemMode)) == -1)
    {
        PLOGW("Failed to create semaphore : %s\n", strerror(errno));
        if ((pShmemBuffer->sema_id = semget((key_t)shmemKey, 1, 0666)) == -1)
        {
            PLOGE("Failed to get semaphore : %s\n", strerror(errno));
            free(*phShmem);
            *phShmem = nullptr;
            return SHMEM_FAILED;
        }
    }

    *pShmemBuffer->unit_size = unitSize;
    *pShmemBuffer->meta_size = metaSize;
    *pShmemBuffer->unit_num  = unitNum;

    int length_buf_offset  = (int)(sizeof(int) * 6);
    int data_buf_offset    = 0;
    int length_meta_offset = 0;
    int data_meta_offset   = 0;
    int extra_size_offset  = 0;
    int extra_buf_offset   = 0;

    if ((*pShmemBuffer->unit_size) >= 0 && (*pShmemBuffer->unit_size) <= SHMEM_UNIT_SIZE_MAX &&
        (*pShmemBuffer->meta_size) >= 0 && (*pShmemBuffer->meta_size) <= SHMEM_META_SIZE_MAX &&
        (*pShmemBuffer->unit_num) >= 0 && (*pShmemBuffer->unit_num) <= SHMEM_UNIT_NUM_MAX)
    {
        data_buf_offset = SHMEM_HEADER_SIZE + (SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

        length_meta_offset = SHMEM_HEADER_SIZE + ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) *
                                                     (*pShmemBuffer->unit_num);

        data_meta_offset =
            SHMEM_HEADER_SIZE +
            ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
            (SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

        extra_size_offset =
            SHMEM_HEADER_SIZE +
            ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
            ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

        extra_buf_offset =
            SHMEM_HEADER_SIZE +
            ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
            ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
            ((int)sizeof(int));
    }
    else
    {
        PLOGE("range out\n");
        free(*phShmem);
        *phShmem = nullptr;
        return SHMEM_ERROR_RANGE_OUT;
    }

    pShmemBuffer->length_buf = (unsigned int *)(pSharedmem + length_buf_offset);

    pShmemBuffer->data_buf = pSharedmem + data_buf_offset;

    pShmemBuffer->length_meta = (unsigned int *)(pSharedmem + length_meta_offset);

    pShmemBuffer->data_meta = pSharedmem + data_meta_offset;

    pShmemBuffer->extra_size = nullptr;

    pShmemBuffer->extra_buf = nullptr;

    struct shmid_ds shm_stat;
    if (-1 != shmctl(pShmemBuffer->shmem_id, IPC_STAT, &shm_stat))
    {
        PLOGD("shm_stat.shm_nattch=%lu\n", shm_stat.shm_nattch);
        if (shm_stat.shm_nattch == 1)
            PLOGD("we are the first client\n");

        PLOGD("shared memory size = %zd\n", shm_stat.shm_segsz);
        // shared momory size larger than total, we use extra data
        size_t sz_extra = (extra_size_offset > 0) ? ((unsigned long)extra_size_offset) : 0;
        if (shm_stat.shm_segsz > sz_extra)
        {
            pShmemBuffer->extra_size = (int *)(pSharedmem + extra_size_offset);
            pShmemBuffer->extra_buf  = pSharedmem + extra_buf_offset;
        }
        else
        {
            //[TODO] Execute close sequence and return fail
            pShmemBuffer->extra_size = nullptr;
            pShmemBuffer->extra_buf  = nullptr;
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
    *pShmemBuffer->read_index  = -1;

    PLOGI("unitSize = %d, SHMEM_LENGTH_SIZE = %d, unit_num = %d\n", *pShmemBuffer->unit_size,
          SHMEM_LENGTH_SIZE, *pShmemBuffer->unit_num);
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::WriteShmemory(SHMEM_HANDLE hShmem, unsigned char *pData,
                                              int dataSize, const char *pMeta, int metaSize,
                                              unsigned char *pExtraData, int extraDataSize)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return SHMEM_FAILED;
    }

    if (*shmem_buffer->write_index == -1)
    {
        *shmem_buffer->write_index = 0;
    }

    int mark         = *shmem_buffer->mark;
    int unit_size    = *shmem_buffer->unit_size;
    int meta_size    = *shmem_buffer->meta_size;
    int unit_num     = *shmem_buffer->unit_num;
    int lwrite_index = *shmem_buffer->write_index;
    if (extraDataSize > 0 && extraDataSize != *shmem_buffer->extra_size)
    {
        PLOGE("extraDataSize should be same with extrasize used when open\n");
        return SHMEM_FAILED;
    }

    if (mark == SHMEM_COMM_MARK_RESET)
    {
        PLOGW("warning - read process isn't reset yet!\n");
    }

    if ((dataSize == 0) || (dataSize > unit_size))
    {
        PLOGE("size error(%d > %d)!\n", dataSize, unit_size);
        return SHMEM_FAILED;
    }

    // Once the writer writes the last buffer, it is made to point to the first
    // buffer again
    if (lwrite_index == unit_num)
    {
        PLOGE("Overflow write data(write_index = %d, unit_num = %d)!\n", lwrite_index,
              *shmem_buffer->unit_num);
        *shmem_buffer->write_index = 0;
        return SHMEM_ERROR_RANGE_OUT; // ERROR_OVERFLOW
    }

    if (!(lwrite_index >= 0 && lwrite_index < SHMEM_UNIT_NUM_MAX &&
          (*shmem_buffer->meta_size) >= 0 && (*shmem_buffer->meta_size) <= SHMEM_META_SIZE_MAX &&
          (*shmem_buffer->unit_size) >= 0 && (*shmem_buffer->unit_size) <= SHMEM_UNIT_SIZE_MAX &&
          (*shmem_buffer->extra_size) >= 0 && (*shmem_buffer->extra_size) <= SHMEM_EXTRA_SIZE_MAX))
    {
        PLOGE("Overflow data(write_index = %d, unit_size = %d, meta_size = %d, extra_size=%d)!\n",
              lwrite_index, *shmem_buffer->unit_size, (*shmem_buffer->meta_size),
              (*shmem_buffer->extra_size));
        return SHMEM_ERROR_RANGE_OUT; // ERROR_OVERFLOW
    }

    *(int *)(shmem_buffer->length_buf + lwrite_index) = dataSize;
    memcpy(shmem_buffer->data_buf + lwrite_index * (*shmem_buffer->unit_size), pData, dataSize);

    if (metaSize < meta_size)
    {
        *(int *)(shmem_buffer->length_meta + lwrite_index) = metaSize;
        memcpy(shmem_buffer->data_meta + lwrite_index * (*shmem_buffer->meta_size), pMeta,
               metaSize);
    }

    if (pExtraData && extraDataSize > 0)
    {
        memcpy(shmem_buffer->extra_buf + lwrite_index * (*shmem_buffer->extra_size), pExtraData,
               extraDataSize);
    }

    *shmem_buffer->write_index += 1;
    if (*shmem_buffer->write_index == *shmem_buffer->unit_num)
        *shmem_buffer->write_index = 0;
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::GetShmemoryBufferInfo(SHMEM_HANDLE hShmem, int numBuffers,
                                                      buffer_t pBufs[], buffer_t pBufsExt[])
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }

    if (numBuffers != *shmem_buffer->unit_num)
    {
        PLOGE("shmem buffer count mismatch\n");
        return SHMEM_ERROR_COUNT_MISMATCH;
    }
    if (pBufs)
    {
        for (int i = 0; i < *shmem_buffer->unit_num; i++)
        {
            int size_temp = 0;
            (i < INT_MAX / (*shmem_buffer->unit_size)) ? size_temp = i * (*shmem_buffer->unit_size)
                                                       : PLOGE("cert violation\n");
            pBufs[i].start  = shmem_buffer->data_buf + size_temp;
            pBufs[i].length = (*shmem_buffer->unit_size > 0) ? (*shmem_buffer->unit_size) : 0;
        }
    }

    if (pBufsExt)
    {
        for (int i = 0; i < *shmem_buffer->unit_num; i++)
        {
            int size_temp = 0;
            (i < INT_MAX / (*shmem_buffer->extra_size))
                ? size_temp = i * (*shmem_buffer->extra_size)
                : PLOGE("cert violation\n");
            pBufsExt[i].start  = shmem_buffer->extra_buf + size_temp;
            pBufsExt[i].length = *shmem_buffer->extra_size;
        }
    }
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::WriteHeader(SHMEM_HANDLE hShmem, int index, size_t bytesWritten)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }
    size_t sz_unit = (*shmem_buffer->unit_size > 0) ? (*shmem_buffer->unit_size) : 0;
    if ((bytesWritten == 0) || (bytesWritten > sz_unit))
    {
        PLOGE("size error(%zu > %d)!\n", bytesWritten, *shmem_buffer->unit_size);
        return SHMEM_ERROR_RANGE_OUT;
    }

    *shmem_buffer->write_index                 = index;
    *(int *)(shmem_buffer->length_buf + index) = bytesWritten;

    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::WriteMeta(SHMEM_HANDLE hShmem, const char *pMeta, size_t metaSize)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }

    size_t meta_size = (*shmem_buffer->meta_size > 0) ? (*shmem_buffer->meta_size) : 0;
    int lwrite_index = *shmem_buffer->write_index;

    if (metaSize < meta_size)
    {
        *(int *)(shmem_buffer->length_meta + lwrite_index) = metaSize;

        if (lwrite_index >= 0 && lwrite_index < SHMEM_UNIT_NUM_MAX &&
            (*shmem_buffer->meta_size) >= 0 && (*shmem_buffer->meta_size) <= SHMEM_META_SIZE_MAX)
        {
            memcpy(shmem_buffer->data_meta + lwrite_index * (*shmem_buffer->meta_size), pMeta,
                   metaSize);
        }
        else
        {
            PLOGE("cert violation\n");
            return SHMEM_ERROR_RANGE_OUT;
        }
    }
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::WriteExtra(SHMEM_HANDLE hShmem, unsigned char *extraData,
                                           size_t extraBytes)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }
    size_t sz_extra = (*shmem_buffer->extra_size > 0) ? (*shmem_buffer->extra_size) : 0;
    if (extraBytes == 0 || extraBytes > sz_extra)
    {
        PLOGE("size error(%zu > %d)!\n", extraBytes, *shmem_buffer->extra_size);
        return SHMEM_ERROR_RANGE_OUT;
    }

    if ((*shmem_buffer->write_index) >= 0 && (*shmem_buffer->write_index) < SHMEM_UNIT_NUM_MAX &&
        (*shmem_buffer->extra_size) >= 0 && (*shmem_buffer->extra_size) <= SHMEM_EXTRA_SIZE_MAX)
    {
        unsigned char *addr =
            shmem_buffer->extra_buf + (*shmem_buffer->write_index) * (*shmem_buffer->extra_size);
        memcpy(addr, extraData, extraBytes);
    }
    else
    {
        PLOGE("overflow error\n");
        return SHMEM_ERROR_RANGE_OUT;
    }

    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::IncrementWriteIndex(SHMEM_HANDLE hShmem)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }

    // Increase the write index to match the read index of ReadShmem
    if (*shmem_buffer->write_index < INT_MAX)
    {
        *shmem_buffer->write_index += 1;
    }

    if (*shmem_buffer->write_index == *shmem_buffer->unit_num)
        *shmem_buffer->write_index = 0;

    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::CloseShmemory(SHMEM_HANDLE *phShmem)
{
    PLOGI("CloseShmemory start");

    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)*phShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_bufer is NULL\n");
        return SHMEM_IS_NULL;
    }

    void *shmem_addr = shmem_buffer->write_index;
    shmdt(shmem_addr);

    struct shmid_ds shm_stat;
    if (-1 != shmctl(shmem_buffer->shmem_id, IPC_STAT, &shm_stat))
    {
        PLOGI("shm_stat.shm_nattch=%lu\n", shm_stat.shm_nattch);

        if (shm_stat.shm_nattch == 0)
        {
            PLOGI("This is the only attached client\n");
            shmctl(shmem_buffer->shmem_id, IPC_RMID, NULL);
        }
    }

    free(shmem_buffer);
    shmem_buffer = nullptr;
    PLOGI("CloseShmemory end");
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::OpenShmem(SHMEM_HANDLE *phShmem, key_t shmemKey)
{
    return openShmem(phShmem, &shmemKey, 0, 0, 0, MODE_OPEN);
}

static int resetShmem(SHMEM_COMM_T *shmem_buffer)
{
    union semun se;

    if (shmem_buffer == NULL)
    {
        PLOGE("Invalid argument\n");
        return -1;
    }

    se.val = 0;
    return semctl(shmem_buffer->sema_id, 0, SETVAL, se);
}

SHMEM_STATUS_T openShmem(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize, int unitNum,
                         int extraSize, int nOpenMode)
{
    SHMEM_COMM_T *pShmemBuffer;
    unsigned char *pSharedmem;
    key_t shmemKey;
    int shmemSize = 0;
    int shmemMode = 0666;
    struct shmid_ds shm_stat;

    *phShmem     = (SHMEM_HANDLE)calloc(1, sizeof(SHMEM_COMM_T));
    pShmemBuffer = (SHMEM_COMM_T *)*phShmem;
    if (pShmemBuffer == NULL)
    {
        PLOGE("pShmemBuffer is null\n");
        return SHMEM_FAILED;
    }

    PLOGI("hShmem = %p, pKey = %p, nOpenMode=%d, unitSize=%d, unitNum=%d\n", *phShmem, pShmemKey,
          nOpenMode, unitSize, unitNum);

    if (nOpenMode == MODE_CREATE)
    {
        shmemKey   = getShmemKey();
        *pShmemKey = shmemKey;

        if (unitSize >= 0 && unitSize <= SHMEM_UNIT_SIZE_MAX && unitNum >= 0 &&
            unitNum <= SHMEM_UNIT_NUM_MAX && extraSize >= 0 && extraSize <= SHMEM_EXTRA_SIZE_MAX)
        {
            shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum +
                        ((int)sizeof(int)) + extraSize * unitNum;
        }
        else
        {
            PLOGE("shmemSize error\n");
        }

        shmemMode |= IPC_CREAT | IPC_EXCL;
    }
    else
    {
        shmemKey = *pShmemKey;
    }

    PLOGI("shmem_key=%d\n", shmemKey);

    pShmemBuffer->shmem_id = shmget((key_t)shmemKey, shmemSize, shmemMode);
    if (pShmemBuffer->shmem_id == -1)
    {
        PLOGE("Can't open shared memory: %s\n", strerror(errno));
        free(pShmemBuffer);
        return SHMEM_FAILED;
    }

    PLOGI("shared memory created/opened successfully!\n");

    pSharedmem = (unsigned char *)shmat(pShmemBuffer->shmem_id, NULL, 0);
    if (pSharedmem == NULL)
    {
        PLOGE("pSharedmem is NULL\n");
        free(pShmemBuffer);
        return SHMEM_FAILED;
    }
    pShmemBuffer->write_index = (int *)(pSharedmem + sizeof(int) * 0);
    pShmemBuffer->read_index  = (int *)(pSharedmem + sizeof(int) * 1);
    pShmemBuffer->unit_size   = (int *)(pSharedmem + sizeof(int) * 2);
    pShmemBuffer->meta_size   = (int *)(pSharedmem + sizeof(int) * 3);
    pShmemBuffer->unit_num    = (int *)(pSharedmem + sizeof(int) * 4);
    pShmemBuffer->mark        = (SHMEM_MARK_T *)(pSharedmem + sizeof(int) * 5);

    if (nOpenMode == MODE_OPEN || (pShmemBuffer->sema_id = semget(shmemKey, 1, shmemMode)) == -1)
    {
        if (nOpenMode == MODE_CREATE)
            PLOGW("Failed to create semaphore : %s\n", strerror(errno));

        if ((pShmemBuffer->sema_id = semget((key_t)shmemKey, 1, 0666)) == -1)
        {
            PLOGE("Failed to get semaphore : %s\n", strerror(errno));
            free(pShmemBuffer);
            return SHMEM_FAILED;
        }
    }

    if (nOpenMode == MODE_CREATE)
    {
        *pShmemBuffer->unit_size = unitSize;
        *pShmemBuffer->unit_num  = unitNum;
    }

    int length_buf_offset  = (int)(sizeof(int) * 6);
    int data_buf_offset    = 0;
    int length_meta_offset = 0;
    int data_meta_offset   = 0;
    int extra_size_offset  = 0;
    int extra_buf_offset   = 0;

    if ((*pShmemBuffer->unit_size) >= 0 && (*pShmemBuffer->unit_size) <= SHMEM_UNIT_SIZE_MAX &&
        (*pShmemBuffer->meta_size) >= 0 && (*pShmemBuffer->meta_size) <= SHMEM_META_SIZE_MAX &&
        (*pShmemBuffer->unit_num) >= 0 && (*pShmemBuffer->unit_num) <= SHMEM_UNIT_NUM_MAX)
    {
        data_buf_offset = SHMEM_HEADER_SIZE + (SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

        length_meta_offset = SHMEM_HEADER_SIZE + ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) *
                                                     (*pShmemBuffer->unit_num);

        data_meta_offset =
            SHMEM_HEADER_SIZE +
            ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
            (SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

        extra_size_offset =
            SHMEM_HEADER_SIZE +
            ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
            ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

        extra_buf_offset =
            SHMEM_HEADER_SIZE +
            ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
            ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
            ((int)sizeof(int));
    }
    else
    {
        PLOGE("range out\n");
        free(pShmemBuffer);
        return SHMEM_ERROR_RANGE_OUT;
    }

    pShmemBuffer->length_buf = (unsigned int *)(pSharedmem + length_buf_offset);

    pShmemBuffer->data_buf = pSharedmem + data_buf_offset;

    pShmemBuffer->length_meta = (unsigned int *)(pSharedmem + length_meta_offset);

    pShmemBuffer->data_meta = pSharedmem + data_meta_offset;

    pShmemBuffer->extra_size = NULL;
    pShmemBuffer->extra_buf  = NULL;

    if (shmctl(pShmemBuffer->shmem_id, IPC_STAT, &shm_stat) != -1)
    {
        PLOGD("shm_stat.shm_nattch=%lu\n", shm_stat.shm_nattch);
        if (shm_stat.shm_nattch == 1)
            PLOGD("we are the first client\n");

        PLOGD("shared memory size = %zd\n", shm_stat.shm_segsz);
        // shared momory size larger than total, we use extra data
        size_t sz_extra_offset = (extra_size_offset > 0) ? extra_size_offset : 0;
        if (shm_stat.shm_segsz > sz_extra_offset)
        {
            pShmemBuffer->extra_size = (int *)(pSharedmem + extra_size_offset);
            pShmemBuffer->extra_buf  = pSharedmem + extra_buf_offset;
        }
    }

    if (nOpenMode == MODE_CREATE && pShmemBuffer->extra_size != NULL)
    {
        *pShmemBuffer->extra_size = extraSize;
    }

    *pShmemBuffer->mark = SHMEM_COMM_MARK_NORMAL;
    // Until the writter starts to write both write index and read index are
    // set to -1 . So the reader can get to know that the writter has not
    // started to write yet
    *pShmemBuffer->write_index = -1;
    *pShmemBuffer->read_index  = -1;

    resetShmem(pShmemBuffer);

    PLOGI("unitSize = %d, SHMEM_LENGTH_SIZE = %d, unit_num = %d\n", *pShmemBuffer->unit_size,
          SHMEM_LENGTH_SIZE, *pShmemBuffer->unit_num);
    PLOGI("shared memory opened successfully! : shmem_id=%d, sema_id=%d\n", pShmemBuffer->shmem_id,
          pShmemBuffer->sema_id);
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::ReadShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize)
{
    return readShmem(hShmem, ppData, pSize, NULL, NULL, READ_FIRST);
}

SHMEM_STATUS_T readShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                         unsigned char **ppExtraData, int *pExtraSize, int readMode)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    int lread_index;
    unsigned char *read_addr;
    int size;
    static bool first_read;

    first_read = false;
    if (!shmem_buffer)
    {
        PLOGE("shmem buffer is NULL");
        return SHMEM_FAILED;
    }
    lread_index = *shmem_buffer->write_index;

    do
    {
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
                    if (*shmem_buffer->unit_num > INT_MIN + 1)
                    {
                        lread_index = *shmem_buffer->unit_num - 1;
                    }
                }
            }
            else
            {
                lread_index = *shmem_buffer->write_index - 1;
            }
            size = *(int *)(shmem_buffer->length_buf + lread_index);

            if ((size == 0) || (size > *shmem_buffer->unit_size))
            {
                PLOGE("size error(%d) lread_index(%d), unit_size(%d), extra_size(%d) !\n", size,
                      lread_index, *shmem_buffer->unit_size, *shmem_buffer->extra_size);
                return SHMEM_FAILED;
            }

            if (!(lread_index >= 0 && lread_index <= SHMEM_UNIT_NUM_MAX &&
                  (*shmem_buffer->unit_size) >= 0 &&
                  (*shmem_buffer->unit_size) <= SHMEM_UNIT_SIZE_MAX &&
                  (*shmem_buffer->extra_size) >= 0 &&
                  (*shmem_buffer->extra_size) <= SHMEM_EXTRA_SIZE_MAX))
            {
                PLOGE("size error(%d) lread_index(%d), unit_size(%d), extra_size(%d) !\n", size,
                      lread_index, *shmem_buffer->unit_size, *shmem_buffer->extra_size);
                return SHMEM_FAILED;
            }

            read_addr = shmem_buffer->data_buf + (lread_index) * (*shmem_buffer->unit_size);
            *ppData   = read_addr;
            *pSize    = size;

            if (NULL != ppExtraData && NULL != pExtraSize)
            {
                *ppExtraData =
                    shmem_buffer->extra_buf + (lread_index) * (*shmem_buffer->extra_size);
                *pExtraSize = *shmem_buffer->extra_size;
            }
        }

        break;
    } while (1);

    return SHMEM_IS_OK;
}

int IPCSharedMemory::GetWriteIndex(SHMEM_HANDLE hShmem)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    return *shmem_buffer->write_index;
}
