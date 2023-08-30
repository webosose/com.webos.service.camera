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

#include "ipc_posix_shared_memory.h"
#include "PmLogLib.h"
#include "camera_constants.h"
#include "camera_types.h"
#include "luna-service2/lunaservice.h"
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef SHMEM_COMM_DEBUG
#define DEBUG_PRINT(fmt, args...)                                                                  \
    printf("\x1b[1;40;32m[SHM_API:%s] " fmt "\x1b[0m\r\n", __FUNCTION__, ##args)
#else
#define DEBUG_PRINT(fmt, args...)
#endif

// constants

#define SHMEM_HEADER_SIZE (6 * sizeof(int))
#define SHMEM_LENGTH_SIZE sizeof(int)

typedef enum
{
    POSHMEM_COMM_MARK_NORMAL    = 0x0,
    POSHMEM_COMM_MARK_RESET     = 0x1,
    POSHMEM_COMM_MARK_TERMINATE = 0x2
} POSHMEM_MARK_T;

/* shared memory structure
 4 bytes             : write_index
 4 bytes             : read_index
 4 bytes             : unit_size
 4 bytes             : meta_size
 4 bytes             : unit_num
 4 bytes             : mark
 4 bytes  *unit_num  : length data
 unit_size*unit_num  : data
 4 bytes  *unit_num  : length meta
 meta_size*unit_num  : meta
 4 bytes             : extra_size
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
    POSHMEM_MARK_T *mark;

    unsigned int *length_buf;
    unsigned char *data_buf;

    unsigned int *length_meta;
    unsigned char *data_meta;

    int *extra_size;
    unsigned char *extra_buf;
} POSHMEM_COMM_T;

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

PSHMEM_STATUS_T IPCPosixSharedMemory::CreateShmemory(SHMEM_HANDLE *phShmem, int unitSize,
                                                     int metaSize, int unitNum, int extraSize,
                                                     int *fd, std::string *shmemname)
{
    *phShmem                     = (SHMEM_HANDLE)calloc(1, sizeof(POSHMEM_COMM_T));
    POSHMEM_COMM_T *pShmemBuffer = (POSHMEM_COMM_T *)*phShmem;
    if (pShmemBuffer == nullptr)
    {
        DEBUG_PRINT("failed to create memory for shm handle");
        return PSHMEM_IS_NULL;
    }

    DEBUG_PRINT("hShmem = %p, unitSize=%d, unitNum=%d\n", *phShmem, unitSize, unitNum);

    int shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum + sizeof(int) +
                    extraSize * unitNum;
    // Create shared memory name
    pid_t pid            = getpid();
    int shm_fd           = -1;
    char poshm_name[100] = {};

    while (1)
    {
        snprintf(poshm_name, sizeof(poshm_name), "/cam%d_poshm", (int)pid++);

        shm_fd = shm_open(poshm_name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);

        if (shm_fd > 0)
        {
            DEBUG_PRINT("POSIX shared memory created.\n");
            *shmemname = poshm_name;
            break;
        }
        else if (shm_fd == -1 && errno == EEXIST)
        {
            DEBUG_PRINT("POSIX shared memory name %s already exist \n", poshm_name);
            continue;
        }
        else
        {
            DEBUG_PRINT("Create failed and error is %s\n", std::strerror(errno));
            free(*phShmem);
            *phShmem = nullptr;
            return PSHMEM_FAILED;
        }
    }
    if (ftruncate(shm_fd, shmemSize) == -1)
    {
        DEBUG_PRINT("Failed to set size of shared memory \n");
        close(shm_fd);
        free(*phShmem);
        *phShmem = nullptr;
        return PSHMEM_FAILED;
    }
    *fd = shm_fd;

    DEBUG_PRINT("Posix shared memory created/opened successfully!\n");

    unsigned char *pSharedmem =
        (unsigned char *)mmap(NULL, shmemSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (pSharedmem == MAP_FAILED || pSharedmem == NULL)
    {
        DEBUG_PRINT("mmap failed \n");
        shm_unlink(poshm_name);
        close(shm_fd);
        free(*phShmem);
        *phShmem = nullptr;
        *fd      = -1;
        return PSHMEM_FAILED;
    }

    pShmemBuffer->write_index = (int *)(pSharedmem);
    pShmemBuffer->read_index  = (int *)(pSharedmem + sizeof(int));
    pShmemBuffer->unit_size   = (int *)(pSharedmem + sizeof(int) * 2);
    pShmemBuffer->meta_size   = (int *)(pSharedmem + sizeof(int) * 3);
    pShmemBuffer->unit_num    = (int *)(pSharedmem + sizeof(int) * 4);
    pShmemBuffer->mark        = (POSHMEM_MARK_T *)(pSharedmem + sizeof(int) * 5);

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

    struct stat sb;

    if (fstat(shm_fd, &sb) == -1)
    {
        DEBUG_PRINT("Failed to get size of shared memory \n");
        munmap(pSharedmem, shmemSize);
        shm_unlink(poshm_name);
        close(shm_fd);
        free(*phShmem);
        *phShmem = nullptr;
        *fd      = -1;
        return PSHMEM_FAILED;
    }
    // shared momory size larger than total, we use extra data
    if ((unsigned long)sb.st_size > extra_size_offset)
    {
        pShmemBuffer->extra_size = (int *)(pSharedmem + extra_size_offset);
        pShmemBuffer->extra_buf  = pSharedmem + extra_buf_offset;
    }
    else
    {
        pShmemBuffer->extra_size = nullptr;
        pShmemBuffer->extra_buf  = nullptr;
    }

    if (pShmemBuffer->extra_size)
    {
        *pShmemBuffer->extra_size = extraSize;
    }

    *pShmemBuffer->mark = POSHMEM_COMM_MARK_NORMAL;
    // Until the writter starts to write both write index and read index are
    // set to -1 . So the reader can get to know that the writter has not
    // started to write yet
    *pShmemBuffer->write_index = -1;
    *pShmemBuffer->read_index  = -1;

    DEBUG_PRINT("unitSize = %d, SHMEM_LENGTH_SIZE = %d, unit_num = %d\n", *pShmemBuffer->unit_size,
                SHMEM_LENGTH_SIZE, *pShmemBuffer->unit_num);

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::WriteShmemory(SHMEM_HANDLE hShmem, unsigned char *pData,
                                                    int dataSize, unsigned char *pMeta,
                                                    int metaSize, unsigned char *pExtraData,
                                                    int extraDataSize)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
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
        return PSHMEM_FAILED;
    }
    if (mark == POSHMEM_COMM_MARK_RESET)
    {
        DEBUG_PRINT("warning - read process isn't reset yet!\n");
    }

    if ((dataSize == 0) || (dataSize > unit_size))
    {
        DEBUG_PRINT("size error(%d > %d)!\n", dataSize, unit_size);
        return PSHMEM_FAILED;
    }

    // Once the writer writes the last buffer, it is made to point to the first
    // buffer again
    if (lwrite_index == unit_num)
    {
        PLOGI("Overflow write data(write_index = %d, unit_num = %d)!\n", lwrite_index,
              *shmem_buffer->unit_num);
        *shmem_buffer->write_index = 0;
        return PSHMEM_ERROR_RANGE_OUT; // ERROR_OVERFLOW
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

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::GetShmemoryBufferInfo(SHMEM_HANDLE hShmem, int numBuffers,
                                                            buffer_t pBufs[], buffer_t pBufsExt[])
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
    }

    if (numBuffers != *shmem_buffer->unit_num)
    {
        DEBUG_PRINT("posixshm buffer count mismatch\n");
        return PSHMEM_ERROR_COUNT_MISMATCH;
    }

    for (int i = 0; i < *shmem_buffer->unit_num; i++)
    {
        pBufs[i].start  = shmem_buffer->data_buf + i * (*shmem_buffer->unit_size);
        pBufs[i].length = *shmem_buffer->unit_size;
    }

    if (pBufsExt)
    {
        for (int i = 0; i < *shmem_buffer->unit_num; i++)
        {
            pBufsExt[i].start  = shmem_buffer->extra_buf + i * (*shmem_buffer->extra_size);
            pBufsExt[i].length = *shmem_buffer->extra_size;
        }
    }

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::WriteHeader(SHMEM_HANDLE hShmem, int index,
                                                  size_t bytesWritten)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
    }
    if ((bytesWritten == 0) || (bytesWritten > (size_t)(*shmem_buffer->unit_size)))
    {
        DEBUG_PRINT("size error(%lu > %lu)!\n", bytesWritten, *shmem_buffer->unit_size);
        return PSHMEM_ERROR_RANGE_OUT;
    }

    *shmem_buffer->write_index                 = index;
    *(int *)(shmem_buffer->length_buf + index) = bytesWritten;

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::WriteMeta(SHMEM_HANDLE hShmem, unsigned char *pMeta,
                                                size_t metaSize)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
    }

    int meta_size    = *shmem_buffer->meta_size;
    int lwrite_index = *shmem_buffer->write_index;

    if ((int)metaSize < meta_size)
    {
        *(int *)(shmem_buffer->length_meta + lwrite_index) = metaSize;
        memcpy(shmem_buffer->data_meta + lwrite_index * (*shmem_buffer->meta_size), pMeta,
               metaSize);
    }
    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::WriteExtra(SHMEM_HANDLE hShmem, unsigned char *extraData,
                                                 size_t extraBytes)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
    }
    if (extraBytes == 0 || extraBytes > (size_t)(*shmem_buffer->extra_size))
    {
        DEBUG_PRINT("size error(%lu > %lu)!\n\n", extraBytes, *shmem_buffer->extra_size);
        return PSHMEM_ERROR_RANGE_OUT;
    }
    unsigned char *addr =
        shmem_buffer->extra_buf + (*shmem_buffer->write_index) * (*shmem_buffer->extra_size);
    memcpy(addr, extraData, extraBytes);

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::IncrementWriteIndex(SHMEM_HANDLE hShmem)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
    }

    // Increase the write index to match the read index of ReadShmem
    *shmem_buffer->write_index += 1;
    if (*shmem_buffer->write_index == *shmem_buffer->unit_num)
        *shmem_buffer->write_index = 0;

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::CloseShmemory(SHMEM_HANDLE *phShmem, int unitNum,
                                                    int unitSize, int metaSize, int extraSize,
                                                    std::string shmemname, int shmemfd)
{
    DEBUG_PRINT("CloseShmemory start");

    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)*phShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_bufer is NULL\n");
        return PSHMEM_IS_NULL;
    }

    int shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum +
                    (metaSize + SHMEM_LENGTH_SIZE) * unitNum + sizeof(int) + (extraSize)*unitNum;

    void *shmem_addr = shmem_buffer->write_index;

    /* FIX-ME: We should try close shmemfd anyway, aren't we? */
    if (munmap(shmem_addr, shmemSize) == -1)
    {
        DEBUG_PRINT("munmap failed!!\n");
        // return PSHMEM_ERROR_MUNMAP_FAIL;
    }

    if (shm_unlink(shmemname.c_str()) == -1)
    {
        DEBUG_PRINT("shm_unlink failed!!\n");
        // return PSHMEM_ERROR_UNLINK_FAIL;
    }

    close(shmemfd);
    free(shmem_buffer);
    shmem_buffer = nullptr;

    DEBUG_PRINT("CloseShmemory end");
    return PSHMEM_IS_OK;
}
