#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include "camsrv_smsync_use_case_runner.h"


ret_val_t run_as_child_process(ret_val_t (*task)(int, int), int iparam)
{
    int fd[2];
    ret_val_t val = {0, {0, 0}};

    // create pipe descriptors
    pipe(fd);

    int status_child;

    printf("parent pid: %d\n", getpid());


    int child = fork();
    if (child < 0)
    {
        printf("fork error\n");
        val = {-1, {-1, -1}};
        return val;
    }
    else if (child == 0)  // child
    {
        ret_val_t retval = {0, {0, 0}};

        pid_t child_pid = getpid();
        printf("child process: pid = %d\n", child_pid);


        if (task)
        {
	        retval = task(child_pid, iparam);
        }

        printf("child process: sleep 3 seconds ...\n");
        sleep(3);


        // writing only, no need for read-descriptor.
        close(fd[0]);

        // send the value on the write-descriptor.
        write(fd[1], &retval, sizeof(retval));

        // close the write descriptor
        close(fd[1]);

        _exit(0);
    }
    else  // parent
    {
        int ret = waitpid(child, &status_child, 0);
        if (ret != -1)
        {
            // read-only
            close(fd[1]);

            // read the data
	        if (0 == read(fd[0], &val, sizeof(val)))
            {
                printf("pipe empty\n");
            }

	        // close the read-descriptor
	        close(fd[0]);

            printf("child process finished: ret_val={%d, {%d, %d}}\n",
                val.value, val.info[0], val.info[1]);

            if (WIFEXITED(status_child) != 0)
            {
	            printf("+ normal termination.\n");
            }
        }
    }

    return val;
}
