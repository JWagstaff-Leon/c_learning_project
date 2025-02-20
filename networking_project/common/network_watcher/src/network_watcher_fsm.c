#include "network_watcher.h"
#include "network_watcher_internal.h"
#include "network_watcher_fsm.h"

#include <assert.h>
#include <poll.h>
#include <stdint.h>
#include <unistd.h>

#include "common_types.h"
#include "message_queue.h"


static eSTATUS setup_watch(
    sNETWORK_WATCHER_CBLK*  master_cblk_ptr,
    eNETWORK_WATCHER_MODE   mode,
    sNETWORK_WATCHER_WATCH* watches,
    uint32_t                watch_count)
{
    eSTATUS status = STATUS_SUCCESS;

    uint32_t watch_index;
    short    poll_events;
    int*     next_fd_ptr;

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
    }


    // Deallocation is done here so that back to back watches of the same or lower count don't reallocate
    if (NULL == master_cblk_ptr->fds || 0 == master_cblk_ptr->fds_size || watch_count > master_cblk_ptr->fds_size)
    {
        generic_deallocator(master_cblk_ptr->fds);
        master_cblk_ptr->fds          = NULL;
        master_cblk_ptr->fds_size     = 0;
        master_cblk_ptr->watch_count  = 0;

        master_cblk_ptr->fds = generic_allocator(sizeof(struct pollfd) * watch_count);
        if (NULL == master_cblk_ptr->fds)
        {
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        NETWORK_WATCHER_EVENT_WATCH_ERROR,
                                        NULL);
            return STATUS_ALLOC_FAILED;
        }

        master_cblk_ptr->fds_size = watch_count;
    }
    master_cblk_ptr->watch_count = watch_count;
    master_cblk_ptr->watches     = watches;

    // Setup the pollfds for all the given fds
    for (watch_index = 0; watch_index < watch_count; watch_index++)
    {
        watches[watch_index].ready = false;

        next_fd_ptr = watches[watch_index].fd_ptr;
        if (NULL != next_fd_ptr && watches[watch_index].active)
        {
            master_cblk_ptr->fds[watch_index].fd      = *next_fd_ptr;
            master_cblk_ptr->fds[watch_index].events  = poll_events;
            master_cblk_ptr->fds[watch_index].revents = 0;
        }
        else
        {
            master_cblk_ptr->fds[watch_index].fd      = -1;
            master_cblk_ptr->fds[watch_index].events  = 0;
            master_cblk_ptr->fds[watch_index].revents = 0;
        }
    }

    return STATUS_SUCCESS;
}


static eSTATUS do_watch(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    int                      poll_status;
    uint32_t                 fd_index;
    eSTATUS                  status;
    sNETWORK_WATCHER_MESSAGE message;

    assert(NULL != master_cblk_ptr);

    poll_status = poll(master_cblk_ptr->fds,
                       master_cblk_ptr->watch_count,
                       master_cblk_ptr->poll_timeout);

    if (poll_status < 0)
    {
        return STATUS_FAILURE;
    }

    if (0 == poll_status)
    {
        message.type = NETWORK_WATCHER_MESSAGE_CONTINUE_WATCH;
        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
        return STATUS_INCOMPLETE;
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

    return STATUS_SUCCESS;
}


static void open_processing(
    sNETWORK_WATCHER_CBLK*          master_cblk_ptr,
    const sNETWORK_WATCHER_MESSAGE* message)
{
    eSTATUS status;

    switch (message->type)
    {
        case NETWORK_WATCHER_MESSAGE_NEW_WATCH:
        {
            status = setup_watch(master_cblk_ptr, message);
            if (STATUS_SUCCESS != status)
            {
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            NETWORK_WATCHER_EVENT_WATCH_ERROR,
                                            NULL);
                break;
            }

            status = do_watch(master_cblk_ptr);
            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                    NETWORK_WATCHER_EVENT_WATCH_COMPLETE,
                                    NULL);
                    break;
                }
                case STATUS_INCOMPLETE:
                {
                    master_cblk_ptr->state = NETWORK_WATCHER_STATE_ACTIVE;
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
        case NETWORK_WATCHER_MESSAGE_CANCEL:
        {
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        NETWORK_WATCHER_EVENT_CANCELLED,
                                        NULL);
            break;
        }
        case NETWORK_WATCHER_MESSAGE_CLOSE:
        {
            master_cblk_ptr->state = NETWORK_WATCHER_STATE_CLOSED;
            break;
        }
    }
}


static void active_processing(
    sNETWORK_WATCHER_CBLK*          master_cblk_ptr,
    const sNETWORK_WATCHER_MESSAGE* message)
{
    eSTATUS status;

    sNETWORK_WATCHER_MESSAGE new_message;

    switch (message->type)
    {
        case NETWORK_WATCHER_MESSAGE_CONTINUE_WATCH:
        {
            status = do_watch(master_cblk_ptr);
            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    master_cblk_ptr->state = NETWORK_WATCHER_STATE_OPEN;
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                NETWORK_WATCHER_EVENT_WATCH_COMPLETE,
                                                NULL);
                    break;
                }
                case STATUS_INCOMPLETE:
                {
                    new_message.type = NETWORK_WATCHER_MESSAGE_CONTINUE_WATCH;

                    status = message_queue_put(master_cblk_ptr->message_queue,
                                               &new_message,
                                               sizeof(new_message));
                    assert(STATUS_SUCCESS == status);
                    break;
                }
                default:
                {
                    master_cblk_ptr->state = NETWORK_WATCHER_STATE_OPEN;
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                NETWORK_WATCHER_EVENT_WATCH_ERROR,
                                                NULL);
                    break;
                }
            }
            break;
        }
        case NETWORK_WATCHER_MESSAGE_CANCEL:
        {
            master_cblk_ptr->state = NETWORK_WATCHER_STATE_OPEN;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        NETWORK_WATCHER_EVENT_CANCELLED,
                                        NULL);
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
        case NETWORK_WATCHER_STATE_ACTIVE:
        {
            active_processing(master_cblk_ptr, message);
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
