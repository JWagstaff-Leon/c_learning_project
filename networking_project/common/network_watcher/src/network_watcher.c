#include "network_watcher.h"
#include "network_watcher_internal.h"

#include <assert.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "chat_connection.h"
#include "chat_connection_internal.h"
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
    void*                       user_arg,
    uint32_t                    fd_count)
{
    sNETWORK_WATCHER_CBLK* new_network_watcher;
    eSTATUS                status;
    int                    pipe_status;

    assert(NULL != out_new_network_watcher);

    new_network_watcher = (sNETWORK_WATCHER_CBLK*)generic_allocator(sizeof(sNETWORK_WATCHER_CBLK));
    if (NULL == new_network_watcher)
    {
        status = STATUS_ALLOC_FAILED;
        goto func_exit;
    }
    init_cblk(new_network_watcher);

    new_network_watcher->fds = generic_allocator(sizeof(struct pollfd) * (fd_count + 2));
    if (NULL == new_network_watcher->fds)
    {
        status = STATUS_ALLOC_FAILED;
        goto fds_alloc_fail;
    }

    new_network_watcher->connection_indecies = generic_allocator(sizeof(uint32_t) * fd_count);
    if (NULL == new_network_watcher->connection_indecies)
    {
        status = STATUS_ALLOC_FAILED;
        goto indecies_alloc_fail;
    }

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

    new_network_watcher->fd_count = fd_count;

    *out_new_network_watcher = new_network_watcher;
    status = STATUS_SUCCESS;
    goto func_exit;

thread_create_fail:
    close(new_network_watcher->close_pipe[PIPE_END_WRITE]);
    close(new_network_watcher->close_pipe[PIPE_END_READ]);

pipe2_create_fail:
    close(new_network_watcher->cancel_pipe[PIPE_END_WRITE]);
    close(new_network_watcher->cancel_pipe[PIPE_END_READ]);

pipe1_create_fail:
    generic_deallocator(new_network_watcher->connection_indecies);

indecies_alloc_fail:
    generic_deallocator(new_network_watcher->fds);

fds_alloc_fail:
    generic_deallocator(new_network_watcher);

func_exit:
    return status;
}


eSTATUS network_watcher_start_watch(
    NETWORK_WATCHER       restrict network_watcher,
    eNETWORK_WATCHER_MODE          mode,
    const void*           restrict fd_list,
    uint32_t                       fd_count)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    eSTATUS                status = STATUS_SUCCESS;
    int                    lock_status;
    uint32_t               fd_index;
    short                  poll_events;

    uint8_t  flush_buffer[128];

    assert(NULL != network_watcher);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    lock_status = pthread_mutex_trylock(master_cblk_ptr->watch_mutex);
    if (0 != lock_status)
    {
        status = STATUS_INVALID_STATE;
        goto func_exit;
    }
    if (!master_cblk_ptr->open)
    {
        status = STATUS_INVALID_STATE;
        goto func_exit;
    }

    if (fd_count > master_cblk_ptr->fd_count);
    {
        status = STATUS_INVALID_ARG;
        goto func_exit;
    }

    switch (mode)
    {
        case NETWORK_WATCHER_MODE_READ:
        {
            poll_events = POLLIN;
            break;
        }
        case NETWORK_WATCHER_MODE_WRITE:
        {
            poll_events = POLLOUT;
            break;
        }
        default:
        {
            status = STATUS_INVALID_ARG;
            goto func_exit;
        }
    }

    // Setup the pollfds for all the given fds
    for (fd_index = 0; fd_index < fd_count; fd_index++)
    {
        master_cblk_ptr->fds[fd_index].fd      = fd_list[fd_index];
        master_cblk_ptr->fds[fd_index].events  = poll_events;
        master_cblk_ptr->fds[fd_index].revents = 0;
    }

    // Clear out any remaining master control block pollfds
    for (fd_index = fd_count; fd_index < master_cblk_ptr->fd_count; fd_index++)
    {
        master_cblk_ptr->fds[fd_index].fd      = -1;
        master_cblk_ptr->fds[fd_index].events  = 0;
        master_cblk_ptr->fds[fd_index].revents = 0;
    }

    master_cblk_ptr->fds[master_cblk_ptr->fd_count].fd      = master_cblk_ptr->cancel_pipe[PIPE_END_READ];
    master_cblk_ptr->fds[master_cblk_ptr->fd_count].events  = POLLIN;
    master_cblk_ptr->fds[master_cblk_ptr->fd_count].revents = 0;

    master_cblk_ptr->fds[master_cblk_ptr->fd_count + 1].fd  = master_cblk_ptr->close_pipe[PIPE_END_READ];
    master_cblk_ptr->fds[master_cblk_ptr->fd_count + 1].events  = POLLIN;
    master_cblk_ptr->fds[master_cblk_ptr->fd_count + 1].revents = 0;

    // Flush the control pipes
    while (read(master_cblk_ptr->cancel_pipe[PIPE_END_READ],
                (void*)&flush_buffer[0],
                sizeof(flush_buffer))
           > 0);

    while (read(master_cblk_ptr->close_pipe[PIPE_END_READ],
                (void*)&flush_buffer[0],
                sizeof(flush_buffer))
           > 0);

    pthread_cond_signal(master_cblk_ptr->watch_condvar);

func_exit:
    pthread_mutex_unlock(master_cblk_ptr->watch_mutex);
    return status;
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
