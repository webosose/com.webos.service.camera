#include "usrptr_handle.h"
#include "camshm_0-cpy.h"
#include "camera_hal_types.h" // HAL_LOG_INFO


usrptr_handle_t * create_usrptr_handle(stream_format_t stream_format, int num_bufs)
{
    usrptr_handle_t *husrptr = nullptr;
    int shmkey = -1;
    int buf_size = 0;
    SHMEM_STATUS_T ret = SHMEM_IS_OK;

    buf_size = stream_format.stream_height * stream_format.stream_width * 2; // max: size of YUY2

    husrptr = (usrptr_handle_t*)calloc(1, sizeof(usrptr_handle_t));
    if (husrptr == nullptr)
    {
        HAL_LOG_INFO(CONST_MODULE_HAL, "user pointer handle allocation failed!!\n");
        return nullptr;
    }

    ret = IPCSharedMemory::getInstance().CreateShmemory(
                          &husrptr->hshm, (key_t*)&shmkey, buf_size, num_bufs, 0);
    if (SHMEM_IS_OK != ret)
    {
        HAL_LOG_INFO(CONST_MODULE_HAL, "shmem creation failed!!\n");
        free(husrptr);
        return nullptr;
    }

    struct buffer *usrptr_bufs = (struct buffer*)calloc(num_bufs, sizeof(struct buffer));
    if (!usrptr_bufs)
    {
        HAL_LOG_INFO(CONST_MODULE_HAL, "usrptr_bufs allocation failed!!\n");
        IPCSharedMemory::getInstance().CloseShmemory(&husrptr->hshm);
        free(husrptr);
        return nullptr;
    }

    ret = IPCSharedMemory::getInstance().GetShmemoryBufferInfo(
                                husrptr->hshm, num_bufs, usrptr_bufs, nullptr);
    if (SHMEM_IS_OK != ret)
    {
        HAL_LOG_INFO(CONST_MODULE_HAL, "fail to get shmem buffer info!!\n");
        IPCSharedMemory::getInstance().CloseShmemory(&husrptr->hshm);
        free(usrptr_bufs);
        free(husrptr);
        return nullptr;
    }

    husrptr->shm_key = shmkey;
    husrptr->unit_size = buf_size;
    husrptr->num_units = num_bufs;
    husrptr->buffers = usrptr_bufs;

    // debug
    HAL_LOG_INFO(CONST_MODULE_HAL, "shm_key : %d\n", shmkey);
    HAL_LOG_INFO(CONST_MODULE_HAL, "unit size : %d, num units : %d\n", buf_size, num_bufs);
    for (int i = 0; i < num_bufs; i++)
    {
        HAL_LOG_INFO(CONST_MODULE_HAL, "buffers[%d] addr : %p, length : %u\n", i, husrptr->buffers[i].start, husrptr->buffers[i].length);
    }

    return husrptr;
}

int write_usrptr_header(usrptr_handle_t *husrptr, int index, size_t bytes_written)
{
    return IPCSharedMemory::getInstance().WriteHeader(husrptr->hshm, index, (unsigned int)bytes_written);
}

void destroy_usrptr_handle(usrptr_handle_t **husrptr)
{
    if (*husrptr != nullptr)
    {
        IPCSharedMemory::getInstance().CloseShmemory(&((*husrptr)->hshm));
        free((*husrptr)->buffers);
        free(*husrptr);
        *husrptr = nullptr;
        HAL_LOG_INFO(CONST_MODULE_HAL, "user pointer handle freed\n");
    }
}

