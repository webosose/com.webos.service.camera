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

#include "ipc_shared_memory.h"
#include "camera_types.h"
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

#define SHMEM_COMM_DEBUG
#ifdef SHMEM_COMM_DEBUG
#define DEBUG_PRINT(fmt, args...) PMLOG_INFO(CONST_MODULE_SHM, fmt, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

// constants

#define CAMSHKEY 7010
#define SHMEM_HEADER_SIZE (6 * sizeof(int))
#define SHMEM_LENGTH_SIZE sizeof(int)

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

SHMEM_STATUS_T _OpenShmem(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize, int unitNum,
                          int extraSize, int nOpenMode);
SHMEM_STATUS_T _ReadShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                          unsigned char **ppExtraData, int *pExtraSize, int readMode);

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
    struct seminfo *__buf;
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

SHMEM_STATUS_T IPCSharedMemory::CreateShmemory(SHMEM_HANDLE *phShmem, key_t *pShmemKey,
                                               int unitSize, int metaSize, int unitNum,
                                               int extraSize)
{
    *phShmem                   = (SHMEM_HANDLE)calloc(1, sizeof(SHMEM_COMM_T));
    SHMEM_COMM_T *pShmemBuffer = (SHMEM_COMM_T *)*phShmem;
    if (!pShmemBuffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return SHMEM_FAILED;
    }

    DEBUG_PRINT("hShmem = %p, pKey = %p, unitSize=%d, metaSize=%d, unitNum=%d\n", *phShmem,
                pShmemKey, unitSize, metaSize, unitNum);

    key_t shmemKey;
    int shmemMode = 0666;
    for (shmemKey = CAMSHKEY; shmemKey < 0xFFFF; shmemKey++)
    {
        pShmemBuffer->shmem_id = shmget((key_t)shmemKey, 0, shmemMode);
        if ((pShmemBuffer->shmem_id == -1) && (errno == ENOENT))
            break;
    }
    *pShmemKey    = shmemKey;
    int shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum +
                    (metaSize + SHMEM_LENGTH_SIZE) * unitNum + sizeof(int) + (extraSize)*unitNum;
    shmemMode |= IPC_CREAT | IPC_EXCL;

    DEBUG_PRINT("shmem_key=%d\n", shmemKey);

    pShmemBuffer->shmem_id = shmget((key_t)shmemKey, shmemSize, shmemMode);
    if (pShmemBuffer->shmem_id == -1)
    {
        DEBUG_PRINT("Can't open shared memory: %s\n", strerror(errno));
        free(*phShmem);
        *phShmem = nullptr;
        return SHMEM_FAILED;
    }

    DEBUG_PRINT("shared memory created/opened successfully!\n");

    unsigned char *pSharedmem = (unsigned char *)shmat(pShmemBuffer->shmem_id, NULL, 0);
    pShmemBuffer->write_index = (int *)(pSharedmem + sizeof(int) * 0);
    pShmemBuffer->read_index  = (int *)(pSharedmem + sizeof(int) * 1);
    pShmemBuffer->unit_size   = (int *)(pSharedmem + sizeof(int) * 2);
    pShmemBuffer->meta_size   = (int *)(pSharedmem + sizeof(int) * 3);
    pShmemBuffer->unit_num    = (int *)(pSharedmem + sizeof(int) * 4);
    pShmemBuffer->mark        = (SHMEM_MARK_T *)(pSharedmem + sizeof(int) * 5);

    if ((pShmemBuffer->sema_id = semget(shmemKey, 1, shmemMode)) == -1)
    {
#ifdef SHMEM_COMM_DEBUG
        DEBUG_PRINT("Failed to create semaphore : %s\n", strerror(errno));
#endif
        if ((pShmemBuffer->sema_id = semget((key_t)shmemKey, 1, 0666)) == -1)
        {
            DEBUG_PRINT("Failed to get semaphore : %s\n", strerror(errno));
            free(*phShmem);
            *phShmem = nullptr;
            return SHMEM_FAILED;
        }
    }

    *pShmemBuffer->unit_size = unitSize;
    *pShmemBuffer->meta_size = metaSize;
    *pShmemBuffer->unit_num  = unitNum;

    size_t length_buf_offset = sizeof(int) * 6;

    size_t data_buf_offset = SHMEM_HEADER_SIZE + (SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

    size_t length_meta_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

    size_t data_meta_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
        (SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

    size_t extra_size_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
        ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

    size_t extra_buf_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
        ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) + sizeof(int);

    pShmemBuffer->length_buf = (unsigned int *)(pSharedmem + length_buf_offset);

    pShmemBuffer->data_buf = pSharedmem + data_buf_offset;

    pShmemBuffer->length_meta = (unsigned int *)(pSharedmem + length_meta_offset);

    pShmemBuffer->data_meta = pSharedmem + data_meta_offset;

    pShmemBuffer->extra_size = nullptr;

    pShmemBuffer->extra_buf = nullptr;

    struct shmid_ds shm_stat;
    if (-1 != shmctl(pShmemBuffer->shmem_id, IPC_STAT, &shm_stat))
    {
#ifdef SHMEM_COMM_DEBUG
        DEBUG_PRINT("shm_stat.shm_nattch=%d\n", (int)shm_stat.shm_nattch);
        if (shm_stat.shm_nattch == 1)
            DEBUG_PRINT("we are the first client\n");

        DEBUG_PRINT("shared memory size = %zd\n", shm_stat.shm_segsz);
#endif
        // shared momory size larger than total, we use extra data
        if (shm_stat.shm_segsz > extra_size_offset)
        {
            pShmemBuffer->extra_size = (int *)(pSharedmem + extra_size_offset);
            pShmemBuffer->extra_buf  = pSharedmem + extra_buf_offset;
        }
        else
        {
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

    DEBUG_PRINT("unitSize = %d, SHMEM_LENGTH_SIZE = %zd, unit_num = %d\n", *pShmemBuffer->unit_size,
                SHMEM_LENGTH_SIZE, *pShmemBuffer->unit_num);
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::WriteShmemory(SHMEM_HANDLE hShmem, unsigned char *pData,
                                              int dataSize, unsigned char *pMeta, int metaSize,
                                              unsigned char *pExtraData, int extraDataSize)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
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
        DEBUG_PRINT("extraDataSize should be same with extrasize used when open\n");
        return SHMEM_FAILED;
    }

    if (mark == SHMEM_COMM_MARK_RESET)
    {
        DEBUG_PRINT("warning - read process isn't reset yet!\n");
    }

    if ((dataSize == 0) || (dataSize > unit_size))
    {
        DEBUG_PRINT("size error(%d > %d)!\n", dataSize, unit_size);
        return SHMEM_FAILED;
    }

    // Once the writer writes the last buffer, it is made to point to the first
    // buffer again
    if (lwrite_index == unit_num)
    {
        DEBUG_PRINT("Overflow write data(write_index = %d, unit_num = %d)!\n", lwrite_index,
                    *shmem_buffer->unit_num);
        *shmem_buffer->write_index = 0;
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
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }

    if (numBuffers != *shmem_buffer->unit_num)
    {
        DEBUG_PRINT("shmem buffer count mismatch\n");
        return SHMEM_ERROR_COUNT_MISMATCH;
    }
    if (pBufs)
    {
        for (int i = 0; i < *shmem_buffer->unit_num; i++)
        {
            pBufs[i].start  = shmem_buffer->data_buf + i * (*shmem_buffer->unit_size);
            pBufs[i].length = *shmem_buffer->unit_size;
        }
    }

    if (pBufsExt)
    {
        for (int i = 0; i < *shmem_buffer->unit_num; i++)
        {
            pBufsExt[i].start  = shmem_buffer->extra_buf + i * (*shmem_buffer->extra_size);
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
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }
    if ((bytesWritten == 0) || (bytesWritten > (size_t)(*shmem_buffer->unit_size)))
    {
        DEBUG_PRINT("size error(%zu > %d)!\n", bytesWritten, *shmem_buffer->unit_size);
        return SHMEM_ERROR_RANGE_OUT;
    }

    *shmem_buffer->write_index                 = index;
    *(int *)(shmem_buffer->length_buf + index) = bytesWritten;

    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::WriteMeta(SHMEM_HANDLE hShmem, unsigned char *pMeta,
                                          size_t metaSize)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }

    int meta_size    = *shmem_buffer->meta_size;
    int lwrite_index = *shmem_buffer->write_index;

    if ((int)metaSize < meta_size)
    {
        *(int *)(shmem_buffer->length_meta + lwrite_index) = metaSize;
        memcpy(shmem_buffer->data_meta + lwrite_index * (*shmem_buffer->meta_size), pMeta,
               metaSize);
    }
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::WriteExtra(SHMEM_HANDLE hShmem, unsigned char *extraData,
                                           size_t extraBytes)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }
    if (extraBytes == 0 || extraBytes > (size_t)(*shmem_buffer->extra_size))
    {
        DEBUG_PRINT("size error(%zu > %d)!\n", extraBytes, *shmem_buffer->extra_size);
        return SHMEM_ERROR_RANGE_OUT;
    }
    unsigned char *addr =
        shmem_buffer->extra_buf + (*shmem_buffer->write_index) * (*shmem_buffer->extra_size);
    memcpy(addr, extraData, extraBytes);

    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::IncrementWriteIndex(SHMEM_HANDLE hShmem)
{
    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return SHMEM_IS_NULL;
    }

    // Increase the write index to match the read index of ReadShmem
    *shmem_buffer->write_index += 1;
    if (*shmem_buffer->write_index == *shmem_buffer->unit_num)
        *shmem_buffer->write_index = 0;

    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::CloseShmemory(SHMEM_HANDLE *phShmem)
{
    DEBUG_PRINT("CloseShmemory start");

    SHMEM_COMM_T *shmem_buffer = (SHMEM_COMM_T *)*phShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_bufer is NULL\n");
        return SHMEM_IS_NULL;
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
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::OpenShmem(SHMEM_HANDLE *phShmem, key_t shmemKey)
{
    return _OpenShmem(phShmem, &shmemKey, 0, 0, 0, MODE_OPEN);
}

static int resetShmem(SHMEM_COMM_T *shmem_buffer)
{
    union semun se;

    if (shmem_buffer == NULL)
    {
        DEBUG_PRINT("Invalid argument\n");
        return -1;
    }

    se.val = 0;
    return semctl(shmem_buffer->sema_id, 0, SETVAL, se);
}

SHMEM_STATUS_T _OpenShmem(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize, int unitNum,
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
        DEBUG_PRINT("pShmemBuffer is null\n");
        return SHMEM_FAILED;
    }

    DEBUG_PRINT("hShmem = %p, pKey = %p, nOpenMode=%d, unitSize=%d, unitNum=%d\n", *phShmem,
                pShmemKey, nOpenMode, unitSize, unitNum);

    if (nOpenMode == MODE_CREATE)
    {
        for (shmemKey = CAMSHKEY; shmemKey < 0xFFFF; shmemKey++)
        {
            pShmemBuffer->shmem_id = shmget((key_t)shmemKey, 0, 0666);
            if (pShmemBuffer->shmem_id == -1 && errno == ENOENT)
                break;
        }
        *pShmemKey = shmemKey;
        shmemSize  = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum + sizeof(int) +
                    extraSize * unitNum;
        shmemMode |= IPC_CREAT | IPC_EXCL;
    }
    else
    {
        shmemKey = *pShmemKey;
    }

    DEBUG_PRINT("shmem_key=%d\r\n", shmemKey);

    pShmemBuffer->shmem_id = shmget((key_t)shmemKey, shmemSize, shmemMode);
    if (pShmemBuffer->shmem_id == -1)
    {
        DEBUG_PRINT("Can't open shared memory: %s\n", strerror(errno));
        free(pShmemBuffer);
        return SHMEM_FAILED;
    }

    DEBUG_PRINT("shared memory created/opened successfully!\n");

    pSharedmem                = (unsigned char *)shmat(pShmemBuffer->shmem_id, NULL, 0);
    pShmemBuffer->write_index = (int *)(pSharedmem + sizeof(int) * 0);
    pShmemBuffer->read_index  = (int *)(pSharedmem + sizeof(int) * 1);
    pShmemBuffer->unit_size   = (int *)(pSharedmem + sizeof(int) * 2);
    pShmemBuffer->meta_size   = (int *)(pSharedmem + sizeof(int) * 3);
    pShmemBuffer->unit_num    = (int *)(pSharedmem + sizeof(int) * 4);
    pShmemBuffer->mark        = (SHMEM_MARK_T *)(pSharedmem + sizeof(int) * 5);

    if (nOpenMode == MODE_OPEN || (pShmemBuffer->sema_id = semget(shmemKey, 1, shmemMode)) == -1)
    {
#ifdef SHMEM_COMM_DEBUG
        if (nOpenMode == MODE_CREATE)
            DEBUG_PRINT("Failed to create semaphore : %s\n", strerror(errno));
#endif
        if ((pShmemBuffer->sema_id = semget((key_t)shmemKey, 1, 0666)) == -1)
        {
            DEBUG_PRINT("Failed to get semaphore : %s\n", strerror(errno));
            free(pShmemBuffer);
            return SHMEM_FAILED;
        }
    }

    if (nOpenMode == MODE_CREATE)
    {
        *pShmemBuffer->unit_size = unitSize;
        *pShmemBuffer->unit_num  = unitNum;
    }

    size_t length_buf_offset = sizeof(int) * 6;

    size_t data_buf_offset = SHMEM_HEADER_SIZE + (SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

    size_t length_meta_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

    size_t data_meta_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
        (SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

    size_t extra_size_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
        ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num);

    size_t extra_buf_offset =
        SHMEM_HEADER_SIZE +
        ((*pShmemBuffer->unit_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) +
        ((*pShmemBuffer->meta_size) + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num) + sizeof(int);

    pShmemBuffer->length_buf = (unsigned int *)(pSharedmem + length_buf_offset);

    pShmemBuffer->data_buf = pSharedmem + data_buf_offset;

    pShmemBuffer->length_meta = (unsigned int *)(pSharedmem + length_meta_offset);

    pShmemBuffer->data_meta = pSharedmem + data_meta_offset;

    pShmemBuffer->extra_size = NULL;
    pShmemBuffer->extra_buf  = NULL;

    if (shmctl(pShmemBuffer->shmem_id, IPC_STAT, &shm_stat) != -1)
    {
#ifdef SHMEM_COMM_DEBUG
        DEBUG_PRINT("shm_stat.shm_nattch=%d\n", (int)shm_stat.shm_nattch);
        if (shm_stat.shm_nattch == 1)
            DEBUG_PRINT("we are the first client\n");

        DEBUG_PRINT("shared memory size = %zd\n", shm_stat.shm_segsz);
#endif
        // shared momory size larger than total, we use extra data
        if (shm_stat.shm_segsz > extra_size_offset)
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

    DEBUG_PRINT("unitSize = %d, SHMEM_LENGTH_SIZE = %zd, unit_num = %d\n", *pShmemBuffer->unit_size,
                SHMEM_LENGTH_SIZE, *pShmemBuffer->unit_num);
    DEBUG_PRINT("shared memory opened successfully! : shmem_id=%d, sema_id=%d\n",
                pShmemBuffer->shmem_id, pShmemBuffer->sema_id);
    return SHMEM_IS_OK;
}

SHMEM_STATUS_T IPCSharedMemory::ReadShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize)
{
    return _ReadShmem(hShmem, ppData, pSize, NULL, NULL, READ_FIRST);
}

SHMEM_STATUS_T _ReadShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
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
        DEBUG_PRINT("shmem buffer is NULL");
        return SHMEM_FAILED;
    }
    lread_index = *shmem_buffer->write_index;

    do
    {
#ifdef SHMEM_COMM_DEBUG
        // int sem_count;
        // sem_count = semctl(shmem_buffer->sema_id, 0, GETVAL, 0);
        // DEBUG_PRINT("sem_count=%d\n", sem_count);
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
            size = *(int *)(shmem_buffer->length_buf + lread_index);

            if ((size == 0) || (size > *shmem_buffer->unit_size))
            {
                DEBUG_PRINT("size error(%d)!\n", size);
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
