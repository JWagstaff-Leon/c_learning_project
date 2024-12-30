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

    master_cblk_ptr->open     = false;
    master_cblk_ptr->watching = false;

    pthread_mutex_init(&master_cblk_ptr->watch_mutex, NULL);
    pthread_cond_init(&master_cblk_ptr->watch_condvar, NULL);
}


eSTATUS network_watcher_create(
    NETWORK_WATCHER*            out_new_network_watcher,
    fNETWORK_WATCHER_USER_CBACK user_cback,
    void*                       user_arg)
{
    sNETWORK_WATCHER_CBLK* new_network_watcher;
    eSTATUS                status;
    int                    pipe_status;

    assert(NULL != out_new_network_watcher);

    new_network_watcher = (sNETWORK_WATCHER_CBLK*)generic_allocator(sizeof(sNETWORK_WATCHER_CBLK));
    if (NULL == new_network_watcher)
    {
        return STATUS_ALLOC_FAILED;
    }
    init_cblk(new_network_watcher);

    pipe_status = pipe(new_network_watcher->cancel_pipe);
    if (pipe_status < 0)
    {
        status = STATUS_FAILURE;
        goto pipe1_create_fail;
    }

    pipe_status = pipe(new_network_watcher->close_pipe);
    if (pipe_status < 0)
    {
        status = STATUS_FAILURE;
        goto pipe2_create_fail;
    }

    status = generic_create_thread(network_watcher_thread_entry,
                                   new_network_watcher);
    if (STATUS_SUCCESS != status)
    {
        goto thread_create_fail;
    }

    *out_new_network_watcher = new_network_watcher;
    status = STATUS_SUCCESS;
    goto success;

thread_create_fail:
    close(new_network_watcher->close_pipe[PIPE_END_WRITE]);
    close(new_network_watcher->close_pipe[PIPE_END_READ]);

pipe2_create_fail:
    close(new_network_watcher->cancel_pipe[PIPE_END_WRITE]);
    close(new_network_watcher->cancel_pipe[PIPE_END_READ]);

pipe1_create_fail:
    generic_deallocator(new_network_watcher);
    
success:
    return status;
}


eSTATUS network_watcher_start_watch(
    NETWORK_WATCHER       restrict network_watcher,
    eNETWORK_WATCHER_MODE          mode,
    int*                  restrict fd_list,
    uint32_t                       fd_count)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    eSTATUS                status;
    int                    lock_status;
    uint32_t               fd_index;
    short                  poll_events;

    assert(NULL != network_watcher);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    lock_status = pthread_mutex_trylock(master_cblk_ptr->watch_mutex);
    if (0 != lock_status)
    {
        return STATUS_INVALID_STATE;
    }
    if (!master_cblk_ptr->open)
    {
        return STATUS_INVALID_STATE;
    }

    master_cblk_ptr->fds = generic_allocate_memory(sizeof(struct pollfd) * (fd_count + 2));
    if (NULL == master_cblk_ptr->fds)
    {
        return STATUS_ALLOC_FAILED;
    }

    master_cblk_ptr->connection_indecies = generic_allocate_memory(sizeof(uint32_t) * fd_count);
    if (NULL == master_cblk_ptr->connection_indecies)
    {
        generic_deallocator(master_cblk_ptr->fds);
        return STATUS_ALLOC_FAILED;
    }

    switch (mode)
    {
        case NETOWRK_WATCHER_MODE_READ:
        {
            poll_events = POLLIN;
            break;
        }
        case NETOWRK_WATCHER_MODE_WRITE:
        {
            poll_events = POLLOUT;
            break;
        }
        default:
        {
            poll_events = 0;
            break;
        }
    }

    master_cblk_ptr->fd_count = fd_count;

    for (fd_index = 0; fd_index < fd_count; fd_index++)
    {
        master_cblk_ptr->fds[fd_index].fd      = fd_list[fd_index];
        master_cblk_ptr->fds[fd_index].events  = poll_events;
        master_cblk_ptr->fds[fd_index].revents = 0;
    }

    master_cblk_ptr->fds[fd_count + fd_index].fd = master_cblk_ptr->cancel_pipe[PIPE_END_READ];
    master_cblk_ptr->fds[fd_count].events        = POLLIN;
    master_cblk_ptr->fds[fd_count].revents       = 0;

    master_cblk_ptr->fds[fd_count + fd_index + 1].fd = master_cblk_ptr->close_pipe[PIPE_END_READ];
    master_cblk_ptr->fds[fd_count].events            = POLLIN;
    master_cblk_ptr->fds[fd_count].revents           = 0;

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
