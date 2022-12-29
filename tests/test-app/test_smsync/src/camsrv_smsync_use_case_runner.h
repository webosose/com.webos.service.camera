#ifndef _CAMSERV_SHMEM_SYNC_USECASE_RUNNER_PROCESS_H_
#define _CAMSERV_SHMEM_SYNC_USECASE_RUNNER_PROCESS_H_

#include "camsrv_smsync_use_case.h"

ret_val_t run_as_child_process(ret_val_t (*task)(int, int), int iparam);

#endif