#include "network_watcher.h"
#include "network_watcher_internal.h"

#include <assert.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"


static void init_cblk(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);
    memset(master_cblk_ptr, 0, sizeof(sNETWORK_WATCHER_CBLK));

    master_cblk_ptr->open     = true;
    master_cblk_ptr->watching = true;

    pthread_mutex_init(&master_cblk_ptr->watch_mutex, NULL);
    pthread_cond_init(&master_cblk_ptr->watch_condvar, NULL);
}


eSTATUS network_watcher_create(
    NETWORK_WATCHER*            out_new_network_watcher,
    sMODULE_PARAMETERS          network_watcher_params,
    fNETWORK_WATCHER_USER_CBACK user_cback,
    void*                       user_arg)
{
    sNETWORK_WATCHER_CBLK* new_network_watcher;
    eSTATUS                status;
    int                    pipe_status;

    assert(NULL != out_new_network_watcher);

    assert(NULL != network_watcher_params.allocator);
    assert(NULL != network_watcher_params.deallocator);
    assert(NULL != network_watcher_params.thread_creator);

    new_network_watcher = (sNETWORK_WATCHER_CBLK*)network_watcher_params.allocator(sizeof(sNETWORK_WATCHER_CBLK));
    if (NULL == new_network_watcher)
    {
        return STATUS_ALLOC_FAILED;
    }

    pipe_status = pipe(new_network_watcher->cancel_pipe);
    if (pipe_status < 0)
    {
        network_watcher_params.deallocator(new_network_watcher);
        return status;
    }

    pipe_status = pipe(new_network_watcher->close_pipe);
    if (pipe_status < 0)
    {
        close(new_network_watcher->cancel_pipe[PIPE_END_WRITE]);
        close(new_network_watcher->cancel_pipe[PIPE_END_READ]);

        network_watcher_params.deallocator(new_network_watcher);
        return status;
    }

    status = network_watcher_params.thread_creator(network_watcher_thread_entry,
                                                   new_network_watcher);
    if (STATUS_SUCCESS != status)
    {
        close(new_network_watcher->cancel_pipe[PIPE_END_WRITE]);
        close(new_network_watcher->cancel_pipe[PIPE_END_READ]);

        close(new_network_watcher->close_pipe[PIPE_END_WRITE]);
        close(new_network_watcher->close_pipe[PIPE_END_READ]);

        network_watcher_params.deallocator(new_network_watcher);
        return status;
    }

    *out_new_network_watcher = new_network_watcher;
    return STATUS_SUCCESS;
}


eSTATUS network_watcher_start_watch(
    NETWORK_WATCHER network_watcher)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    eSTATUS                status;
    int                    lock_status;

    assert(NULL != network_watcher);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    lock_status = pthread_mutex_trylock(master_cblk_ptr->watch_mutex);
    if (0 != lock_status)
    {
        return STATUS_INVALID_STATE;
    }

    pthread_cond_signal(master_cblk_ptr->watch_condvar);
    pthread_mutex_unlock(master_cblk_ptr->watch_mutex);

    return STATUS_SUCCESS;
}


eSTATUS network_watcher_stop_watch(
    NETWORK_WATCHER network_watcher)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    ssize_t                write_status;

    assert(NULL != network_watcher);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    write_status = write(master_cblk_ptr->cancel_pipe[PIPE_END_WRITE], "", sizeof(""));
    if (write_status < 1)
    {
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}


eSTATUS network_watcher_close(
    NETWORK_WATCHER network_watcher)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    ssize_t                write_status;

    assert(NULL != network_watcher);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    write_status = write(master_cblk_ptr->close_pipe[PIPE_END_WRITE], "", sizeof(""));
    if (write_status < 1)
    {
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}
