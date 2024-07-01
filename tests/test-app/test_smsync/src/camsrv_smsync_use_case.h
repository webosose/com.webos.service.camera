#ifndef _CAMSERV_SHMEM_SYNC_USECASE_H_
#define _CAMSERV_SHMEM_SYNC_USECASE_H_

struct ret_val_t
{
    int value;
    int info[2];
};

// No sync
ret_val_t open_start_stop_close(int dummy_pid, int dummy_sig);

// sync default
ret_val_t open_pid_start_stop_close_pid(int ctx_pid, int dum_sig);

// sync user-specified
ret_val_t open_pid_sig_start_stop_close_pid(int ctx_pid, int ctx_sig);

// exception case: close without giving the input pid.
ret_val_t open_pid_start_stop_close(int ctx_pid, int dummy_sig);
ret_val_t open_pid_sig_start_stop_close(int ctx_pid, int ctx_sig);
ret_val_t close_with_pid(int handle, int pid);

// exception case: invalid pid is given
ret_val_t open_invalid_pid_close_invalid_pid(int dummy_pid, int dummy_sig);
ret_val_t open_invalid_pid_start_stop_close_invalid_pid(int dummy_pid, int dummy_sig);
ret_val_t open_invalid_pid_start_stop_close(int dummy_pid, int dummy_sig);

#endif