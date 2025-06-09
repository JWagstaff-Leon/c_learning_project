#include "network_watcher.h"
#include "network_watcher_internal.h"
#include "network_watcher_fsm.h"

#include <assert.h>
#include <poll.h>
#include <stdint.h>
#include <unistd.h>

#include "common_types.h"
#include "message_queue.h"


static void setup_watch(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr,
    int                    fd,
    bNETWORK_WATCHER_MODE  mode)
{
    master_cblk_ptr->fds[0].fd = fd;

    master_cblk_ptr->fds[0].events = POLLHUP;

    if (mode & NETWORK_WATCHER_MODE_READ)
    {
        master_cblk_ptr->fds[0].events |= POLLIN;
    }
    if (mode & NETWORK_WATCHER_MODE_WRITE)
    {
        master_cblk_ptr->fds[0].events |= POLLOUT;
    }
}


static eSTATUS do_watch(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    int poll_status;

    poll_status = poll(master_cblk_ptr->fds, 2, 0);
    if (poll_status < 0)
    {
        return STATUS_FAILURE;
    }

    if (master_cblk_ptr->fds[1].revents & POLLIN)
    {
        return STATUS_CLOSED;
    }

    return STATUS_SUCCESS;
}


static void open_processing(
    sNETWORK_WATCHER_CBLK*          master_cblk_ptr,
    const sNETWORK_WATCHER_MESSAGE* message)
{
    eSTATUS status;

    sNETWORK_WATCHER_CBACK_DATA cback_data;

    uint8_t flush_buffer[128];

    switch (message->type)
    {
        case NETWORK_WATCHER_MESSAGE_WATCH:
        {
            setup_watch(master_cblk_ptr,
                        message->params.new_watch.fd,
                        message->params.new_watch.mode);

            status = do_watch(master_cblk_ptr);
            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    cback_data.watch_complete.modes_ready = 0;
                    if (master_cblk_ptr->fds[0].revents & POLLIN)
                    {
                        cback_data.watch_complete.modes_ready |= NETWORK_WATCHER_MODE_READ;
                    }
                    if (master_cblk_ptr->fds[0].revents & POLLOUT)
                    {
                        cback_data.watch_complete.modes_ready |= NETWORK_WATCHER_MODE_WRITE;
                    }

                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                NETWORK_WATCHER_EVENT_WATCH_COMPLETE,
                                                &cback_data);
                    break;
                }
                case STATUS_CLOSED:
                {
                    pthread_mutex_lock(&master_cblk_ptr->cancel_mutex);

                    while (read(master_cblk_ptr->cancel_pipe[PIPE_END_READ],
                                flush_buffer,
                                sizeof(flush_buffer)) > 0);

                    pthread_mutex_unlock(&master_cblk_ptr->cancel_mutex);

                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                NETWORK_WATCHER_EVENT_WATCH_CANCELLED,
                                                NULL);
                    break;
                }
                default:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                NETWORK_WATCHER_EVENT_WATCH_ERROR,
                                                NULL);
                    break;
                }
            }
            break;
        }
        case NETWORK_WATCHER_MESSAGE_CLOSE:
        {
            master_cblk_ptr->state = NETWORK_WATCHER_STATE_CLOSED;
            break;
        }
    }
}


static void dispatch_message(
    sNETWORK_WATCHER_CBLK*          master_cblk_ptr,
    const sNETWORK_WATCHER_MESSAGE* message)
{
    switch (master_cblk_ptr->state)
    {
        case NETWORK_WATCHER_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
            break;
        }
        case NETWORK_WATCHER_STATE_CLOSED:
        default:
        {
            assert(0); // Should never get here
            break;
        }
    }
}


static void fsm_cblk_init(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    // TODO this
}


static void fsm_cblk_close(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    fNETWORK_WATCHER_USER_CBACK user_cback;
    void*                       user_arg;

    assert(NULL != master_cblk_ptr);

    pthread_mutex_destroy(&master_cblk_ptr->cancel_mutex);

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    generic_deallocator(master_cblk_ptr);

    user_cback(user_arg,
               NETWORK_WATCHER_EVENT_CLOSED,
               NULL);
}


void* network_watcher_thread_entry(
    void* arg)
{
    sNETWORK_WATCHER_CBLK*   master_cblk_ptr;
    eSTATUS                  status;
    sNETWORK_WATCHER_MESSAGE message;

    uint8_t flush_buffer[128];

    assert(NULL != arg);
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)arg;

    fsm_cblk_init(master_cblk_ptr);

    while (NETWORK_WATCHER_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(sNETWORK_WATCHER_MESSAGE));
        assert(STATUS_SUCCESS == status);

        dispatch_message(master_cblk_ptr,
                         &message);
    }

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
