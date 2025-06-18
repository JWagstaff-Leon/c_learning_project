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
    void*                 arg,
    THREAD_ID*            out_new_thread)
{
    int       status;
    pthread_t new_thread_id;

    status = pthread_create(&new_thread_id,
                            NULL,
                            thread_entry,
                            arg);
    if (0 != status)
    {
        return STATUS_FAILURE;
    }

    if (NULL != out_new_thread)
    {
        *out_new_thread = new_thread_id;
    }
    return STATUS_SUCCESS;
}


eSTATUS generic_thread_set_kill_mode(
    eTHREAD_KILL_MODE mode)
{
    int cancel_type, old_type;
    int set_status;
    
    switch (mode)
    {
        case THREAD_KILL_DEFERRED:
        {
            cancel_type = PTHREAD_CANCEL_DEFERRED;
            break;
        }
        case THREAD_KILL_INSTANT:
        {
            cancel_type = PTHREAD_CANCEL_ASYNCHRONOUS;
            break;
        }
        default:
        {
            return STATUS_INVALID_ARG;
        }
    }

    set_status = pthread_setcanceltype(cancel_type, &old_type);
    if (0 != set_status)
    {
        return STATUS_FAILURE;
    }
    return STATUS_SUCCESS;
}


eSTATUS generic_kill_thread(
    THREAD_ID thread_id)
{
    int cancel_status = pthread_cancel(thread_id);
    if (0 != cancel_status)
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

    status = STATUS_SUCCESS;
    goto func_complete;

func_complete:
    if (NULL != out_printed_length)
    {
        *out_printed_length = snprintf_status + 1;
    }
    return status;
}
