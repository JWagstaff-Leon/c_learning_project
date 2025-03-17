#include "common_types.h"

#include <pthread.h>
#include <stdio.h>
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
    int       status;
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

eSTATUS print_string_to_buffer(
    char*       restrict buffer,
    const char* restrict string,
    size_t               buffer_size,
    int*        restrict out_printed_length)
{
    eSTATUS status;
    int     snprintf_status;

    if (NULL == buffer)
    {
        return STATUS_INVALID_ARG;
    }

    if (NULL == string)
    {
        return STATUS_INVALID_ARG;
    }

    snprintf_status = snprintf(buffer, buffer_size, "%s", string);
    if (snprintf_status < 0)
    {
        snprintf_status = 0;
        status          = STATUS_FAILURE;

        goto func_complete;
    }
    
    if (snprintf_status > buffer_size)
    {
        snprintf_status = buffer_size;
        status          = STATUS_NO_SPACE;

        goto func_complete;
    }
    
    status = STATUS_NO_SPACE;
    goto func_complete;
    
func_complete:   
    if (NULL != out_printed_length)
    {
        *out_printed_length = snprintf_status;
    }
    return status;
}
