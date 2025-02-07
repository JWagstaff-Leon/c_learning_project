#include "network_watcher.h"
#include "network_watcher_internal.h"

#include <poll.h>
#include <stdint.h>
#include <unistd.h>


static bool do_watch(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    uint32_t fd_index;
    uint8_t  flush_buffer[128];

    sNETWORK_WATCHER_CBACK_DATA cback_data;

    assert(NULL != master_cblk_ptr);

    poll(master_cblk_ptr->fds,
         master_cblk_ptr->fd_count + 2,
         -1);

    // Check if the close pipe has been written to
    if (master_cblk_ptr->fds[master_cblk_ptr->fd_count + 1].revents & POLLIN)
    {
        // Flush the whole pipe
        while (read(master_cblk_ptr->cancel_pipe[PIPE_END_READ],
                    (void*)&flush_buffer[0],
                    sizeof(flush_buffer))
               > 0);
        master_cblk_ptr->open = false;
        return false;
    }

    // Check if the cancel pipe has been written to
    if (master_cblk_ptr->fds[master_cblk_ptr->fd_count].revents & POLLIN)
    {
        // Flush the whole pipe
        while (read(master_cblk_ptr->cancel_pipe[PIPE_END_READ],
                    (void*)&flush_buffer[0],
                    sizeof(flush_buffer))
               > 0);
        master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                    NETWORK_WATCHER_EVENT_CANCELED,
                                    NULL);
        return false;
    }

    cback_data.ready.connection_indecies = master_cblk_ptr->connection_indecies;
    cback_data.ready.index_count         = 0;

    for (fd_index = 0; fd_index < master_cblk_ptr->fd_count; fd_index++)
    {
        if (master_cblk_ptr->fds[fd_index].revents & (POLLIN | POLLOUT))
        {
            assert(cback_data.ready.index_count < master_cblk_ptr->fd_count);
            master_cblk_ptr->connection_indecies[cback_data.ready.index_count++] = fd_index;
        }
    }

    if (cback_data.ready.index_count > 0)
    {
        master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                    NETWORK_WATCHER_EVENT_READY,
                                    &cback_data);
    }
    return cback_data.ready.index_count > 0;
}


static void fsm_cblk_close(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    fNETWORK_WATCHER_USER_CBACK user_cback;
    void*                       user_arg;

    assert(NULL != master_cblk_ptr);

    pthread_cond_destroy(master_cblk_ptr->watch_condvar);
    pthread_mutex_destroy(master_cblk_ptr->watch_mutex);

    close(master_cblk_ptr->cancel_pipe[PIPE_END_WRITE]);
    close(master_cblk_ptr->cancel_pipe[PIPE_END_READ]);

    close(master_cblk_ptr->close_pipe[PIPE_END_WRITE]);
    close(master_cblk_ptr->close_pipe[PIPE_END_READ]);

    generic_deallocator(master_cblk_ptr->fds);
    generic_deallocator(master_cblk_ptr->connection_indecies);

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    master_cblk_ptr->deallocator(master_cblk_ptr);

    user_cback(user_arg,
               NETWORK_WATCHER_EVENT_CLOSED,
               NULL);
}


void* network_watcher_thread_entry(
    void* arg)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    eSTATUS                status;

    assert(NULL != arg);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)arg;

    pthread_mutex_lock(master_cblk_ptr->watch_mutex);

    master_cblk_ptr->open = true;
    while (master_cblk_ptr->open)
    {
        pthread_cond_wait(master_cblk_ptr->watch_condvar,
                          master_cblk_ptr->watch_mutex);
        do
        {
            master_cblk_ptr->watching = do_watch();
        } while (master_cblk_ptr->watching && master_cblk_ptr->open);
    }

    pthread_mutex_unlock(master_cblk_ptr->watch_mutex);

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
