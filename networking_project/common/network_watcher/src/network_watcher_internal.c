#include "network_watcher.h"
#include "network_watcher_internal.h"

#include <poll.h>
#include <stdint.h>
#include <unistd.h>


static void do_watch(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    uint32_t fd_index;
    uint8_t  flush_buffer[128];

    assert(NULL != master_cblk_ptr);

    poll(master_cblk_ptr->fds,
         master_cblk_ptr->watch_count + 2,
         -1);

    // Check if the control pipe has been written to
    if (master_cblk_ptr->fds[master_cblk_ptr->watch_count].revents & POLLIN)
    {
        pthread_mutex_lock(&master_cblk_ptr->control_mutex);

        // Flush the whole pipe
        while (read(master_cblk_ptr->control_pipe[PIPE_END_READ],
                    (void*)&flush_buffer[0],
                    sizeof(flush_buffer))
               > 0);

        if (!master_cblk_ptr->open || !master_cblk_ptr->watching)
        {
            pthread_mutex_unlock(&master_cblk_ptr->control_mutex);
            return;
        }
        pthread_mutex_unlock(&master_cblk_ptr->control_mutex);
    }


    for (fd_index = 0; fd_index < master_cblk_ptr->watch_count; fd_index++)
    {
        if (master_cblk_ptr->fds[fd_index].revents & (POLLIN | POLLOUT))
        {
            master_cblk_ptr->watches[fd_index].ready = true;
        }

        if (master_cblk_ptr->fds[fd_index].revents & POLLHUP)
        {
            master_cblk_ptr->watches[fd_index].closed = true;
        }
    }

    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                NETWORK_WATCHER_EVENT_WATCH_COMPLETE,
                                &cback_data);
}


static void fsm_cblk_close(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    fNETWORK_WATCHER_USER_CBACK user_cback;
    void*                       user_arg;

    assert(NULL != master_cblk_ptr);

    close(master_cblk_ptr->control_pipe[PIPE_END_WRITE]);
    close(master_cblk_ptr->control_pipe[PIPE_END_READ]);

    pthread_mutex_destroy(&master_cblk_ptr->control_mutex);
    pthread_mutex_destroy(&master_cblk_ptr->watch_mutex);

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

    uint8_t flush_buffer[128];

    assert(NULL != arg);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)arg;

    pthread_mutex_lock(&master_cblk_ptr->control_mutex);
    master_cblk_ptr->open = true;
    while (master_cblk_ptr->open)
    {
        master_cblk_ptr->watching = false;
        pthread_mutex_unlock(&master_cblk_ptr->control_mutex);

        // Only poll the control pipe
        poll(&master_cblk_ptr->control_fd,
                1,
                -1);

        pthread_mutex_lock(&master_cblk_ptr->control_mutex);

        // Flush the whole pipe
        while (read(master_cblk_ptr->control_pipe[PIPE_END_READ],
                    (void*)&flush_buffer[0],
                    sizeof(flush_buffer))
               > 0);

        if (!master_cblk_ptr->open || !master_cblk_ptr->watching)
        {
            // The lock will unlock at the start of the next loop, or after the loop in the case of a close
            continue;
        }
        pthread_mutex_unlock(&master_cblk_ptr->control_mutex);

        pthread_mutex_lock(&master_cblk_ptr->watch_mutex);
        do_watch();
        pthread_mutex_unlock(&master_cblk_ptr->watch_mutex);
        
        pthread_mutex_lock(&master_cblk_ptr->control_mutex);
    }
    pthread_mutex_unlock(&master_cblk_ptr->control_mutex);

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
