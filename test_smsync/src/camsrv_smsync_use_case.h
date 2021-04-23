#ifndef _CAMSERV_SHMEM_SYNC_USECASE_H_
#define _CAMSERV_SHMEM_SYNC_USECASE_H_


struct ret_val_t
{
   int value;
   int info;
};

// No sync
ret_val_t open_start_stop_close(int dummy_pid, int dummy_sig);

// sync default
ret_val_t open_pid_start_stop_close_pid(int ctx_pid, int dum_sig);

// sync user-specified
ret_val_t open_pid_sig_start_stop_close_pid(int ctx_pid, int ctx_sig);

// exception handle for the wrong input by users' mistake
ret_val_t close_exception_handler(int dummy_pid, int handle);


#endif