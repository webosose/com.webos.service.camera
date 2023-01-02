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

#include "cam_posixshm.h"
#include "PmLogLib.h"
#include "camera_types.h"
#include "camera_constants.h"
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

#define SHMEM_HEADER_SIZE (5 * sizeof(int))
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
 4 bytes             : unit_num
 4 bytes             : mark
 4 bytes  *unit_num  : length data
 unit_size*unit_num  : data
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
    int *unit_num;
    POSHMEM_MARK_T *mark;

    unsigned int *length_buf;
    unsigned char *data_buf;

    int *extra_size;
    unsigned char *extra_buf;
} POSHMEM_COMM_T;

PSHMEM_STATUS_T IPCPosixSharedMemory::CreateShmemory(SHMEM_HANDLE *phShmem, int unitSize,
                                                     int unitNum, int extraSize, int *fd,
                                                     std::string *shmemname)
{
    *phShmem                     = (SHMEM_HANDLE)calloc(1, sizeof(POSHMEM_COMM_T));
    POSHMEM_COMM_T *pShmemBuffer = (POSHMEM_COMM_T *)*phShmem;
    if (pShmemBuffer == nullptr) {
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
    pShmemBuffer->unit_num    = (int *)(pSharedmem + sizeof(int) * 3);
    pShmemBuffer->mark        = (POSHMEM_MARK_T *)(pSharedmem + sizeof(int) * 4);
    pShmemBuffer->length_buf  = (unsigned int *)(pSharedmem + sizeof(int) * 5);

    *pShmemBuffer->unit_size = unitSize;
    *pShmemBuffer->unit_num  = unitNum;

    pShmemBuffer->data_buf =
        pSharedmem + SHMEM_HEADER_SIZE + SHMEM_LENGTH_SIZE * (*pShmemBuffer->unit_num);

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
    if ((long unsigned int)sb.st_size >
        (SHMEM_HEADER_SIZE +
         (*pShmemBuffer->unit_size + SHMEM_LENGTH_SIZE) * (*pShmemBuffer->unit_num)))
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

    //Increase the write index to match the read index of ReadShmem
    *shmem_buffer->write_index += 1;
    if (*shmem_buffer->write_index == *shmem_buffer->unit_num)
        *shmem_buffer->write_index = 0;

    return PSHMEM_IS_OK;
}

PSHMEM_STATUS_T IPCPosixSharedMemory::CloseShmemory(SHMEM_HANDLE *phShmem, int unitNum,
                                                    int unitSize, int extraSize,
                                                    std::string shmemname, int shmemfd)
{
    DEBUG_PRINT("CloseShmemory start");

    POSHMEM_COMM_T *shmem_buffer = (POSHMEM_COMM_T *)*phShmem;
    if (!shmem_buffer)
    {
        DEBUG_PRINT("shmem_bufer is NULL\n");
        return PSHMEM_IS_NULL;
    }

    int shmemSize = SHMEM_HEADER_SIZE + (unitSize + SHMEM_LENGTH_SIZE) * unitNum + sizeof(int) +
                    extraSize * unitNum;

    void *shmem_addr = shmem_buffer->write_index;

    /* We should try close shmemfd anyway!! */
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
