#ifndef SRC_HAL_UTILS_CAMSHM_H_
#define SRC_HAL_UTILS_CAMSHM_H_
typedef enum _SHMEM_STATUS_T
{
    SHMEM_COMM_OK = 0x0,
    SHMEM_COMM_FAIL = -1,
    SHMEM_COMM_OVERFLOW = -2,
    SHMEM_COMM_NODATA = -3,
    SHMEM_COMM_TERMINATE = -4,
} SHMEM_STATUS_T;

typedef void * SHMEM_HANDLE;

extern SHMEM_STATUS_T CreateShmem(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize,
        int unitNum);
extern SHMEM_STATUS_T CreateShmemEx(SHMEM_HANDLE *phShmem, key_t *pShmemKey, int unitSize,
        int unitNum, int extraSize);
extern SHMEM_STATUS_T OpenShmem(SHMEM_HANDLE *phShmem, key_t shmemKey);
extern SHMEM_STATUS_T ReadShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize);
extern SHMEM_STATUS_T ReadShmemEx(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
        unsigned char **ppExtraData, int *pExtraSize);
extern SHMEM_STATUS_T ReadLastShmem(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize);
extern SHMEM_STATUS_T ReadLastShmemEx(SHMEM_HANDLE hShmem, unsigned char **ppData, int *pSize,
        unsigned char **ppExtraData, int *pExtraSize);
extern SHMEM_STATUS_T WriteShmem(SHMEM_HANDLE hShmem, unsigned char *pData, int dataSize);
extern SHMEM_STATUS_T WriteShmemEx(SHMEM_HANDLE hShmem, unsigned char *pData, int dataSize,
        unsigned char *pExtraData, int extraDataSize);
extern SHMEM_STATUS_T CloseShmem(SHMEM_HANDLE *phShmem);

#endif //SRC_HAL_UTILS_CAMSHM_H_
