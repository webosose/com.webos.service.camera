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

#define LOG_TAG "IPCPosixSharedMemory"
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

// constants

#define SHMEM_HEADER_SIZE ((int)(6 * sizeof(int)))
#define SHMEM_LENGTH_SIZE ((int)sizeof(int))
#define SHMEM_UNIT_SIZE_MAX 66355200 // 7680*4320*2
#define SHMEM_EXTRA_SIZE_MAX ((int)sizeof(unsigned int))
#define SHMEM_META_SIZE_MAX 4096 // 1024*4
#define SHMEM_UNIT_NUM_MAX 8

typedef enum
{
    POSHMEM_COMM_MARK_NORMAL    = 0x0,
    POSHMEM_COMM_MARK_RESET     = 0x1,
    POSHMEM_COMM_MARK_TERMINATE = 0x2
} POSHMEM_MARK_T;

enum
{
    READ_FIRST,
    READ_LAST
};

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
        PLOGE("failed to create memory for shm handle");
        return PSHMEM_IS_NULL;
    }

    PLOGI("hShmem = %p, unitSize=%d, unitNum=%d\n", *phShmem, unitSize, unitNum);

    int shmemSize = 0;

    if (unitSize >= 0 && unitSize <= SHMEM_UNIT_SIZE_MAX && metaSize >= 0 &&
        metaSize <= SHMEM_META_SIZE_MAX && unitNum >= 0 && unitNum <= SHMEM_UNIT_NUM_MAX &&
        extraSize >= 0 && extraSize <= SHMEM_EXTRA_SIZE_MAX)
    {
        shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum +
                    (metaSize + SHMEM_LENGTH_SIZE) * unitNum + ((int)sizeof(int)) +
                    extraSize * unitNum;
    }
    else
    {
        PLOGE("shmemSize error\n");
    }

    // Create shared memory name
    pid_t pid            = getpid();
    int shm_fd           = -1;
    char poshm_name[100] = {};

    while (1)
    {
        int result = snprintf(poshm_name, sizeof(poshm_name), "/cam%d_poshm",
                              ((int)pid < INT_MAX) ? (int)pid++ : -1);
        if (result < 0 || static_cast<size_t>(result) >= sizeof(poshm_name))
        {
            PLOGE("snprintf failed");
            return PSHMEM_FAILED;
        }

        shm_fd = shm_open(poshm_name, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR | S_IRGRP);

        if (shm_fd > 0)
        {
            PLOGI("POSIX shared memory created.\n");
            *shmemname = poshm_name;
            break;
        }
        else if (shm_fd == -1)
        {
            if (errno == EEXIST)
            {
                PLOGI("POSIX shared memory name %s already exist \n", poshm_name);
                continue;
            }
            else
            {
                PLOGE("Create failed and error is %s\n", strerror(errno));
                free(*phShmem);
                *phShmem = nullptr;
                return PSHMEM_FAILED;
            }
        }
    }
    if (ftruncate(shm_fd, shmemSize) == -1)
    {
        PLOGE("Failed to set size of shared memory \n");
        close(shm_fd);
        free(*phShmem);
        *phShmem = nullptr;
        return PSHMEM_FAILED;
    }
    *fd = shm_fd;

    PLOGI("Posix shared memory created/opened successfully!\n");

    unsigned char *pSharedmem =
        (unsigned char *)mmap(NULL, shmemSize, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (pSharedmem == MAP_FAILED || pSharedmem == NULL)
    {
        PLOGE("mmap failed \n");
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
    }

    pShmemBuffer->length_buf = (unsigned int *)(pSharedmem + length_buf_offset);

    pShmemBuffer->data_buf = pSharedmem + data_buf_offset;

    pShmemBuffer->length_meta = (unsigned int *)(pSharedmem + length_meta_offset);

    pShmemBuffer->data_meta = pSharedmem + data_meta_offset;

    pShmemBuffer->extra_size = nullptr;

    pShmemBuffer->extra_buf = nullptr;

    struct stat sb;

    if (fstat(shm_fd, &sb) == -1)
    {
        PLOGE("Failed to get size of shared memory \n");
        munmap(pSharedmem, shmemSize);
        shm_unlink(poshm_name);
        close(shm_fd);
        free(*phShmem);
        *phShmem = nullptr;
        *fd      = -1;
        return PSHMEM_FAILED;
    }
    // shared momory size larger than total, we use extra data
    if (sb.st_size > (long)extra_size_offset)
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

    PLOGI("unitSize = %d, SHMEM_LENGTH_SIZE = %d, unit_num = %d\n", *pShmemBuffer->unit_size,
          SHMEM_LENGTH_SIZE, *pShmemBuffer->unit_num);

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::WriteShmemory(SHMEM_HANDLE hShmem, unsigned char *pData,
                                                    int dataSize, const char *pMeta, int metaSize,
                                                    unsigned char *pExtraData, int extraDataSize)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
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
        PLOGE("extraDataSize should be same with extrasize used when open\n");
        return PSHMEM_FAILED;
    }
    if (mark == POSHMEM_COMM_MARK_RESET)
    {
        PLOGW("warning - read process isn't reset yet!\n");
    }

    if ((dataSize == 0) || (dataSize > unit_size))
    {
        PLOGE("size error(%d > %d)!\n", dataSize, unit_size);
        return PSHMEM_FAILED;
    }

    // Once the writer writes the last buffer, it is made to point to the first
    // buffer again
    if (lwrite_index == unit_num)
    {
        PLOGE("Overflow write data(write_index = %d, unit_num = %d)!\n", lwrite_index,
              *shmem_buffer->unit_num);
        *shmem_buffer->write_index = 0;
        return PSHMEM_ERROR_RANGE_OUT; // ERROR_OVERFLOW
    }

    if (!(lwrite_index >= 0 && lwrite_index < SHMEM_UNIT_NUM_MAX &&
          (*shmem_buffer->meta_size) >= 0 && (*shmem_buffer->meta_size) <= SHMEM_META_SIZE_MAX &&
          (*shmem_buffer->unit_size) >= 0 && (*shmem_buffer->unit_size) <= SHMEM_UNIT_SIZE_MAX &&
          (*shmem_buffer->extra_size) >= 0 && (*shmem_buffer->extra_size) <= SHMEM_EXTRA_SIZE_MAX))
    {
        PLOGE("Overflow data(write_index = %d, unit_size = %d, meta_size = %d, extra_size=%d)!\n",
              lwrite_index, *shmem_buffer->unit_size, (*shmem_buffer->meta_size),
              (*shmem_buffer->extra_size));
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
        PLOGE("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
    }

    if (numBuffers != *shmem_buffer->unit_num)
    {
        PLOGE("posixshm buffer count mismatch\n");
        return PSHMEM_ERROR_COUNT_MISMATCH;
    }

    for (int i = 0; i < *shmem_buffer->unit_num; i++)
    {
        int size_temp = 0;
        (i < INT_MAX / (*shmem_buffer->unit_size)) ? size_temp = i * (*shmem_buffer->unit_size)
                                                   : PLOGE("cert violation\n");
        pBufs[i].start  = shmem_buffer->data_buf + size_temp;
        pBufs[i].length = (*shmem_buffer->unit_size > 0) ? (*shmem_buffer->unit_size) : 0;
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

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::WriteHeader(SHMEM_HANDLE hShmem, int index,
                                                  size_t bytesWritten)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
    }
    size_t sz_unit = (*shmem_buffer->unit_size > 0) ? (*shmem_buffer->unit_size) : 0;
    if ((bytesWritten == 0) || (bytesWritten > sz_unit))
    {
        PLOGE("size error(%zu > %d)!\n", bytesWritten, *shmem_buffer->unit_size);
        return PSHMEM_ERROR_RANGE_OUT;
    }

    *shmem_buffer->write_index                 = index;
    *(int *)(shmem_buffer->length_buf + index) = bytesWritten;

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::WriteMeta(SHMEM_HANDLE hShmem, const char *pMeta,
                                                size_t metaSize)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
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
            return PSHMEM_ERROR_RANGE_OUT;
        }
    }
    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::WriteExtra(SHMEM_HANDLE hShmem, unsigned char *extraData,
                                                 size_t extraBytes)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
    }
    size_t sz_extra = (*shmem_buffer->extra_size > 0) ? (*shmem_buffer->extra_size) : 0;
    if (extraBytes == 0 || extraBytes > sz_extra)
    {
        PLOGE("size error(%zu > %d)!\n\n", extraBytes, *shmem_buffer->extra_size);
        return PSHMEM_ERROR_RANGE_OUT;
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
        return PSHMEM_ERROR_RANGE_OUT;
    }

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::IncrementWriteIndex(SHMEM_HANDLE hShmem)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_buffer is NULL\n");
        return PSHMEM_IS_NULL;
    }

    // Increase the write index to match the read index of ReadShmem
    if (*shmem_buffer->write_index < INT_MAX)
    {
        *shmem_buffer->write_index += 1;
    }
    if (*shmem_buffer->write_index == *shmem_buffer->unit_num)
        *shmem_buffer->write_index = 0;

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::CloseShmemory(SHMEM_HANDLE *phShmem, int unitNum,
                                                    int unitSize, int metaSize, int extraSize,
                                                    std::string shmemname, int shmemfd)
{
    PLOGI("CloseShmemory start");

    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)*phShmem;
    if (!shmem_buffer)
    {
        PLOGE("shmem_bufer is NULL\n");
        return PSHMEM_IS_NULL;
    }

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

    void *shmem_addr = shmem_buffer->write_index;

    /* FIX-ME: We should try close shmemfd anyway, aren't we? */
    if (munmap(shmem_addr, shmemSize) == -1)
    {
        PLOGE("munmap failed!!\n");
        // return PSHMEM_ERROR_MUNMAP_FAIL;
    }

    if (shm_unlink(shmemname.c_str()) == -1)
    {
        PLOGE("shm_unlink failed!!\n");
        // return PSHMEM_ERROR_UNLINK_FAIL;
    }

    close(shmemfd);
    free(shmem_buffer);
    shmem_buffer = nullptr;

    PLOGI("CloseShmemory end");
    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T readShmemory(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
                             unsigned char **ppExtraData, int *pExtraSize, int readMode)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    int lread_index;
    unsigned char *read_addr;
    int size;
    static bool first_read;

    first_read = false;
    if (!shmem_buffer)
    {
        PLOGE("shmem buffer is NULL");
        return PSHMEM_FAILED;
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
                PLOGE("size error(%d)!\n", size);
                return PSHMEM_FAILED;
            }

            if (!(lread_index >= 0 && lread_index <= SHMEM_UNIT_NUM_MAX &&
                  (*shmem_buffer->unit_size) >= 0 &&
                  (*shmem_buffer->unit_size) <= SHMEM_UNIT_SIZE_MAX &&
                  (*shmem_buffer->extra_size) >= 0 &&
                  (*shmem_buffer->extra_size) <= SHMEM_EXTRA_SIZE_MAX))
            {
                PLOGE("size error(%d) lread_index(%d), unit_size(%d), extra_size(%d) !\n", size,
                      lread_index, *shmem_buffer->unit_size, *shmem_buffer->extra_size);
                return PSHMEM_FAILED;
            }

            read_addr = shmem_buffer->data_buf + (lread_index) * (*shmem_buffer->unit_size);
            *ppData   = read_addr;

            *pSize = size;

            if (NULL != ppExtraData && NULL != pExtraSize)
            {
                *ppExtraData =
                    shmem_buffer->extra_buf + (lread_index) * (*shmem_buffer->extra_size);
                *pExtraSize = *shmem_buffer->extra_size;
            }
        }

        break;
    } while (1);

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::ReadShmemory(SHMEM_HANDLE hShmem, unsigned char **ppData,
                                                   int *pSize)
{
    return readShmemory(hShmem, ppData, pSize, NULL, NULL, READ_FIRST);
}

int IPCPosixSharedMemory::GetWriteIndex(SHMEM_HANDLE hShmem)
{
    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)hShmem;
    return *shmem_buffer->write_index;
}
