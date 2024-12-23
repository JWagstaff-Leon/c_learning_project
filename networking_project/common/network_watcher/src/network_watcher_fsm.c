#include "network_watcher.h"
#include "network_watcher_internal.h"
#include "network_watcher_fsm.h"

#include <poll.h>


static bool do_watch(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    struct pollfd fds[NETWORK_WATCHER_MAX_FD_COUNT + 2];
    int           fd_index;

    sNETWORK_WATCHER_CBACK_DATA cback_data;
    cback_data.pollfds = &fds[0];
    cback_data.nfds    = NETWORK_WATCHER_MAX_FD_COUNT;

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


void* network_watcher_thread_entry(
    void* arg)
{
    sNETWORK_WATCHER_CBLK*      master_cblk_ptr;
    sNETWORK_WATCHER_MESSAGE    message;
    eSTATUS                     status;

    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_EVENT_IO_NETWORK_CBLK*)arg;
    
    while (master_cblk_ptr->open)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));

        while (master_cblk_ptr->watching)
        {
            master_cblk_ptr->watching = do_watch();
        }
    }

    pthread_cond_destroy(master_cblk_ptr->watch_condvar);
    pthread_mutex_destroy(master_cblk_ptr->watch_mutex);
    master_cblk_ptr->deallocator(master_cblk_ptr);
    return NULL;
}
