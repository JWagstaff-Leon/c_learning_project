#include "common_types.h"

#include <pthread.h>
#include <stdlib.h>


void* generic_allocator(
    size_t bytes)
{
    return malloc(bytes);
}


void generic_deallocator(
    void* ptr)
{
    free(ptr);
}


eSTATUS generic_create_thread(
    fGENERIC_THREAD_ENTRY thread_entry,
    void* arg)
{
    int status;
    pthread_t unused;

    status = pthread_create(&unused,
                            NULL,
                            thread_entry,
                            arg);
    if (0 != status)
    {
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}
