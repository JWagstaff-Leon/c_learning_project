#include "network_watcher.h"
#include "network_watcher_internal.h"

#include <poll.h>
#include <stdint.h>
#include <unistd.h>


static bool do_watch(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    struct pollfd fds[NETWORK_WATCHER_MAX_FD_COUNT + 2];
    int           fd_index;
    uint8_t       read_buffer[128];

    uNETWORK_WATCHER_CBACK_DATA cback_data;
    cback_data.read_ready.pollfds = &fds[0];
    cback_data.read_ready.nfds    = NETWORK_WATCHER_MAX_FD_COUNT;

    assert(NULL != master_cblk_ptr);

    for (fd_index = 0; fd_index < NETWORK_WATCHER_MAX_FD_COUNT; fd_index++)
    {
        fds[fd_index].fd     = master_cblk_ptr->fds[fd_index];
        fds[fd_index].events = POLLIN;
    }
    fds[NETWORK_WATCHER_CANCEL_PIPE_INDEX].fd     = master_cblk_ptr->cancel_pipe[PIPE_END_READ];
    fds[NETWORK_WATCHER_CANCEL_PIPE_INDEX].events = POLLIN;

    fds[NETWORK_WATCHER_CLOSE_PIPE_INDEX].fd     = master_cblk_ptr->close_pipe[PIPE_END_READ];
    fds[NETWORK_WATCHER_CLOSE_PIPE_INDEX].events = POLLIN;

    poll(fds, NETWORK_WATCHER_MAX_FD_COUNT + 2, -1);

    // Check if the close pipe has been written to
    if (fds[NETWORK_WATCHER_CLOSE_PIPE_INDEX].revents & POLLIN)
    {
        master_cblk_ptr->open = false;
        return false;
    }

    // Check if the cancel pipe has been written to
    if (fds[NETWORK_WATCHER_CANCEL_PIPE_INDEX].revents & POLLIN)
    {
        // Consume the whole buffer
        while (read(master_cblk_ptr->cancel_pipe[PIPE_END_READ],
                    (void*)&read_buffer[0],
                    sizeof(read_buffer))
               > 0);
        master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                    NETWORK_WATCHER_EVENT_CANCELED,
                                    NULL);
        return false;
    }

    for (fd_index = 0; fd_index < NETWORK_WATCHER_MAX_FD_COUNT; fd_index++)
    {
        if (fds[fd_index].revents & POLLIN)
        {
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        NETWORK_WATCHER_EVENT_READ_READY,
                                        &cback_data);
            return false;
        }
    }

    return true;
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
    sNETWORK_WATCHER_CBLK*      master_cblk_ptr;
    sNETWORK_WATCHER_MESSAGE    message;
    eSTATUS                     status;

    assert(NULL != arg);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)arg;
    
    pthread_mutex_lock(master_cblk_ptr->watch_mutex);
    while (master_cblk_ptr->open)
    {
        pthread_cond_wait(master_cblk_ptr->watch_condvar,
                          master_cblk_ptr->watch_mutex);

        while (master_cblk_ptr->watching && master_cblk_ptr->open)
        {
            master_cblk_ptr->watching = do_watch();
        }
    }
    pthread_mutex_unlock(master_cblk_ptr->watch_mutex);

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
