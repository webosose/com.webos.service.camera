#ifndef V4L2_USRPTR_HANDLE_H_
#define V4L2_USRPTR_HANDLE_H_

#include "camera_hal_if_types.h"
#include <stddef.h> // size_t

struct buffer
{
    void *start;
    size_t length;
};

typedef struct
{
    int shm_key;
    void *hshm;
    size_t unit_size;
    int num_units;
    struct buffer *buffers;
} usrptr_handle_t;

usrptr_handle_t* create_usrptr_handle(stream_format_t, int);
int write_usrptr_header(usrptr_handle_t*, int, size_t);
void destroy_usrptr_handle(usrptr_handle_t**);


#endif /* V4L2_USRPTR_HANDLE_H_ */
