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

    pthread_mutex_init(&master_cblk_ptr->watch_mutex, NULL);
    pthread_mutex_init(&master_cblk_ptr->control_mutex, NULL);
}


eSTATUS network_watcher_create(
    NETWORK_WATCHER*               out_new_network_watcher,
    fNETWORK_WATCHER_USER_CBACK    user_cback,
    void*                          user_arg)
{
    sNETWORK_WATCHER_CBLK* new_network_watcher;
    eSTATUS                status;
    int                    pipe_status;

    assert(NULL != out_new_network_watcher);
    assert(NULL != user_cback);

    new_network_watcher = (sNETWORK_WATCHER_CBLK*)generic_allocator(sizeof(sNETWORK_WATCHER_CBLK));
    if (NULL == new_network_watcher)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }
    init_cblk(new_network_watcher);

    pipe_status = pipe(new_network_watcher->control_pipe);
    if (pipe_status < 0)
    {
        status = STATUS_FAILURE;
        goto fail_create_control_pipe;
    }
    new_network_watcher->control_fd.fd      = new_network_watcher->control_pipe[PIPE_END_READ];
    new_network_watcher->control_fd.events  = POLLIN;
    new_network_watcher->control_fd.revents = 0;

    status = generic_create_thread(network_watcher_thread_entry,
                                   new_network_watcher);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_thread;
    }

    new_network_watcher->user_cback = user_cback;
    new_network_watcher->user_arg   = user_arg;

    *out_new_network_watcher = new_network_watcher;
    return STATUS_SUCCESS;

fail_create_thread:
    close(new_network_watcher->control_pipe[PIPE_END_WRITE]);
    close(new_network_watcher->control_pipe[PIPE_END_READ]);

fail_create_control_pipe:
    generic_deallocator(new_network_watcher);

fail_alloc_cblk:
    return status;
}


eSTATUS network_watcher_start_watch(
    NETWORK_WATCHER         network_watcher,
    eNETWORK_WATCHER_MODE   mode,
    sNETWORK_WATCHER_WATCH* watches,
    uint32_t                watch_count)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    eSTATUS                status = STATUS_SUCCESS;
    int                    lock_status;

    uint32_t watch_index;
    short    poll_events;
    int*     next_fd_ptr;

    uint8_t flush_buffer[128];

    assert(NULL != network_watcher);
    assert(NULL != watches);

    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    if (watch_count <= 0)
    {
        return STATUS_INVALID_ARG;
    }

    lock_status = pthread_mutex_trylock(&master_cblk_ptr->control_mutex);
    if (0 != lock_status)
    {
        return STATUS_INVALID_STATE;
    }
    if (!master_cblk_ptr->open || master_cblk_ptr->watching)
    {
        status = STATUS_INVALID_STATE;
        goto func_exit;
    }

    switch (mode)
    {
        case NETWORK_WATCHER_MODE_READ:
        {
            poll_events = POLLIN | POLLHUP;
            break;
        }
        case NETWORK_WATCHER_MODE_WRITE:
        {
            poll_events = POLLOUT | POLLHUP;
            break;
        }
        default:
        {
            status = STATUS_INVALID_ARG;
            goto func_exit;
        }
    }

    pthread_mutex_lock(&master_cblk_ptr->watch_mutex);

    // Deallocation is done here so that back to back watches of the same or lower count don't reallocate
    if (NULL == master_cblk_ptr->watches || 0 == master_cblk_ptr->watches_size || watch_count >= master_cblk_ptr->watches_size)
    {
        generic_deallocator(master_cblk_ptr->watches);
        master_cblk_ptr->watches      = NULL;
        master_cblk_ptr->watch_count  = 0;
        master_cblk_ptr->watches_size = 0;

        master_cblk_ptr->watches = generic_allocator(sizeof(struct pollfd) * (watch_count + 1));
        if (NULL == master_cblk_ptr->watches)
        {
            status = STATUS_ALLOC_FAILED;
            goto func_exit;
        }

        master_cblk_ptr->watches_size = watch_count;
    }
    master_cblk_ptr->watch_count = watch_count;

    // Setup the pollfds for all the given fds
    for (watch_index = 0; watch_index < watch_count; watch_index++)
    {
        watches[watch_index].ready = false;

        next_fd_ptr = watches[watch_index].fd_ptr;
        if (NULL != next_fd_ptr && watches[watch_index].active)
        {
            master_cblk_ptr->watches[watch_index].fd      = *next_fd_ptr;
            master_cblk_ptr->watches[watch_index].events  = poll_events;
            master_cblk_ptr->watches[watch_index].revents = 0;
        }
        else
        {
            master_cblk_ptr->watches[watch_index].fd      = -1;
            master_cblk_ptr->watches[watch_index].events  = 0;
            master_cblk_ptr->watches[watch_index].revents = 0;
        }
    }

    master_cblk_ptr->watches[watch_count].fd      = master_cblk_ptr->control_pipe[PIPE_END_READ];
    master_cblk_ptr->watches[watch_count].events  = POLLIN;
    master_cblk_ptr->watches[watch_count].revents = 0;

    pthread_mutex_unlock(&master_cblk_ptr->watch_mutex);

    write_status = write(master_cblk_ptr->control_pipe[PIPE_END_WRITE], "", sizeof(""));
    if (write_status < 1)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    master_cblk_ptr->watching = true;
    status = STATUS_SUCCESS;
func_exit:
    pthread_mutex_unlock(&master_cblk_ptr->control_mutex);
    return status;
}


eSTATUS network_watcher_stop_watch(
    NETWORK_WATCHER network_watcher)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    eSTATUS                status;
    ssize_t                write_status;

    assert(NULL != network_watcher);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    pthread_mutex_lock(&master_cblk_ptr->control_mutex);

    write_status = write(master_cblk_ptr->control_pipe[PIPE_END_WRITE], "", sizeof(""));
    if (write_status < 1)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }
    master_cblk_ptr->watching = false;

    status = STATUS_SUCCESS;
func_exit:
    pthread_mutex_unlock(&master_cblk_ptr->polling_mutex);
    return status;
}


eSTATUS network_watcher_close(
    NETWORK_WATCHER network_watcher)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    eSTATUS                status;
    ssize_t                write_status;

    assert(NULL != network_watcher);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    pthread_mutex_lock(&master_cblk_ptr->control_mutex);

    write_status = write(master_cblk_ptr->control_pipe[PIPE_END_WRITE], "", sizeof(""));
    if (write_status < 1)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }
    master_cblk_ptr->open = false;

    status = STATUS_SUCCESS;
func_exit:
    pthread_mutex_unlock(&master_cblk_ptr->polling_mutex);
    return status;
}
