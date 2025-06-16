#include "chat_connection.h"
#include "chat_connection_internal.h"

#include <assert.h>
#include <poll.h>
#include <stdio.h>
#include <sys/socket.h>

#include "chat_event.h"
#include "chat_event_io.h"
#include "common_types.h"
#include "message_queue.h"
#include "network_watcher.h"


static eSTATUS read_ready(
    sCHAT_CONNECTION_CBLK* master_cblk_ptr)
{
    sCHAT_CONNECTION_CBACK_DATA cback_data;

    bCHAT_EVENT_IO_RESULT main_io_result;
    bCHAT_EVENT_IO_RESULT extract_result;

    main_io_result = chat_event_io_read_from_fd(master_cblk_ptr->io, master_cblk_ptr->fd);

    if (main_io_result & CHAT_EVENT_IO_RESULT_FAILED)
    {
        assert(0);
        // REVIEW do something here? Logging?
        return STATUS_FAILURE;
    }

    if (main_io_result & CHAT_EVENT_IO_RESULT_FD_CLOSED)
    {
        return STATUS_CLOSED;
    }

    if (main_io_result & CHAT_EVENT_IO_RESULT_READ_FINISHED)
    {
        do
        {
            extract_result = chat_event_io_extract_read_event(master_cblk_ptr->io,
                                                              &cback_data.incoming_event.event);
            if (extract_result & CHAT_EVENT_IO_RESULT_EXTRACT_FINISHED)
            {
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_CONNECTION_EVENT_INCOMING_EVENT,
                                            &cback_data);
            }
        } while (extract_result & CHAT_EVENT_IO_RESULT_EXTRACT_MORE);
    }

    return STATUS_SUCCESS;
}


static eSTATUS write_ready(
    sCHAT_CONNECTION_CBLK* master_cblk_ptr)
{
    sCHAT_EVENT event_buffer;

    eSTATUS               status;
    bCHAT_EVENT_IO_RESULT main_io_result;

    do
    {
        main_io_result = chat_event_io_write_to_fd(master_cblk_ptr->io, master_cblk_ptr->fd);

        if (main_io_result & CHAT_EVENT_IO_RESULT_FAILED)
        {
            assert(0);
            // REVIEW do something here? Logging?
            break;
        }

        if (main_io_result & CHAT_EVENT_IO_RESULT_FD_CLOSED)
        {
            return STATUS_CLOSED;
        }

        if (CHAT_EVENT_IO_RESULT_NOT_POPULATED  & main_io_result ||
            CHAT_EVENT_IO_RESULT_WRITE_FINISHED & main_io_result)
        {
            if (message_queue_get_count(master_cblk_ptr->event_queue) == 0)
            {
                break;
            }

            status = message_queue_get(master_cblk_ptr->event_queue,
                                       &event_buffer,
                                       sizeof(event_buffer));
            assert(STATUS_SUCCESS == status);

            main_io_result = chat_event_io_populate_writer(master_cblk_ptr->io, &event_buffer);
        }
    } while (main_io_result & CHAT_EVENT_IO_RESULT_INCOMPLETE |
             main_io_result & CHAT_EVENT_IO_RESULT_POPULATE_SUCCESS);

    return STATUS_SUCCESS;
}


static void reading_processing(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    eSTATUS status;

    switch (message->type)
    {
        case CHAT_CONNECTION_MESSAGE_TYPE_WATCH_COMPLETE:
        {
            if (message->params.watch_complete.modes_ready & NETWORK_WATCHER_MODE_READ)
            {
                status = read_ready(master_cblk_ptr);
                if (STATUS_CLOSED == status)
                {
                    master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSED;
                    break;
                }
            }

            status = network_watcher_start_watch(master_cblk_ptr->network_watcher,
                                                 NETWORK_WATCHER_MODE_READ,
                                                 master_cblk_ptr->fd);
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_QUEUE_EVENT:
        {
            status = network_watcher_cancel_watch(master_cblk_ptr->network_watcher);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CONNECTION_STATE_CANCELLING;

            status = message_queue_put(master_cblk_ptr->event_queue,
                                       &message->params.queue_event.event,
                                       sizeof(message->params.queue_event.event));
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_CLOSE:
        {
            master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSING;

            status = network_watcher_cancel_watch(master_cblk_ptr->network_watcher);
            assert(STATUS_SUCCESS == status);

            break;
        }
    }
}


static void reading_writing_processing(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    eSTATUS               status;
    bNETWORK_WATCHER_MODE watch_mode;

    switch (message->type)
    {
        case CHAT_CONNECTION_MESSAGE_TYPE_WATCH_COMPLETE:
        {
            if (message->params.watch_complete.modes_ready & NETWORK_WATCHER_MODE_WRITE)
            {
                status = write_ready(master_cblk_ptr);
                if (STATUS_CLOSED == status)
                {
                    master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSED;
                    break;
                }
            }

            if (message->params.watch_complete.modes_ready & NETWORK_WATCHER_MODE_READ)
            {
                status = read_ready(master_cblk_ptr);
                if (STATUS_CLOSED == status)
                {
                    master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSED;
                    break;
                }
            }

            watch_mode = NETWORK_WATCHER_MODE_READ;

            if (message_queue_get_count(master_cblk_ptr->event_queue) > 0) // REVIEW should this also check if the writer isn't done?
            {
                watch_mode |= NETWORK_WATCHER_MODE_WRITE;
            }
            else
            {
                master_cblk_ptr->state = CHAT_CONNECTION_STATE_READING;
            }

            status = network_watcher_start_watch(master_cblk_ptr->network_watcher,
                                                 watch_mode,
                                                 master_cblk_ptr->fd);
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_QUEUE_EVENT:
        {
            status = message_queue_put(master_cblk_ptr->event_queue,
                                       &message->params.queue_event.event,
                                       sizeof(message->params.queue_event.event));
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_CLOSE:
        {
            master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSING;

            status = network_watcher_cancel_watch(master_cblk_ptr->network_watcher);
            assert(STATUS_SUCCESS == status);

            break;
        }
    }
}

static void cancelling_processing(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    eSTATUS               status;
    bNETWORK_WATCHER_MODE watch_mode;

    switch (message->type)
    {
        case CHAT_CONNECTION_MESSAGE_TYPE_WATCH_COMPLETE:
        {
            if (message->params.watch_complete.modes_ready & NETWORK_WATCHER_MODE_WRITE)
            {
                status = write_ready(master_cblk_ptr);
                if (STATUS_CLOSED == status)
                {
                    master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSED;
                    break;
                }
            }

            if (message->params.watch_complete.modes_ready & NETWORK_WATCHER_MODE_READ)
            {
                status = read_ready(master_cblk_ptr);
                if (STATUS_CLOSED == status)
                {
                    master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSED;
                    break;
                }
            }

            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_WATCH_CANCELLED:
        {
            watch_mode = NETWORK_WATCHER_MODE_READ;

            if (message_queue_get_count(master_cblk_ptr->event_queue) > 0)
            {
                watch_mode             |= NETWORK_WATCHER_MODE_WRITE;
                master_cblk_ptr->state  = CHAT_CONNECTION_STATE_READING_WRITING;
            }
            else
            {
                master_cblk_ptr->state - CHAT_CONNECTION_STATE_READING;
            }

            status = network_watcher_start_watch(master_cblk_ptr->network_watcher,
                                                 watch_mode,
                                                 master_cblk_ptr->fd);
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_QUEUE_EVENT:
        {
            status = message_queue_put(master_cblk_ptr->event_queue,
                                       &message->params.queue_event.event,
                                       sizeof(message->params.queue_event.event));
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_CLOSE:
        {
            master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSING;
            break;
        }
    }
}


static void closing_processing(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    eSTATUS               status;
    bNETWORK_WATCHER_MODE watch_mode;

    switch (message->type)
    {
        case CHAT_CONNECTION_MESSAGE_TYPE_WATCH_COMPLETE:
        {
            if (message->params.watch_complete.modes_ready & NETWORK_WATCHER_MODE_WRITE)
            {
                status = write_ready(master_cblk_ptr);
                if (STATUS_CLOSED == status)
                {
                    master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSED;
                    break;
                }
            }

            if (message_queue_get_count(master_cblk_ptr->event_queue) <= 0)
            {
                master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSED;
                break;
            }

            watch_mode = NETWORK_WATCHER_MODE_WRITE;

            status = network_watcher_start_watch(master_cblk_ptr->network_watcher,
                                                 watch_mode,
                                                 master_cblk_ptr->fd);
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_WATCH_CANCELLED:
        {
            if (message_queue_get_count(master_cblk_ptr->event_queue) <= 0)
            {
                master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSED;
                break;
            }

            watch_mode = NETWORK_WATCHER_MODE_WRITE;

            status = network_watcher_start_watch(master_cblk_ptr->network_watcher,
                                                 watch_mode,
                                                 master_cblk_ptr->fd);
            assert(STATUS_SUCCESS == status);
            break;
        }
    }
}


static void dispatch_message(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    switch (master_cblk_ptr->state)
    {
        case CHAT_CONNECTION_STATE_READING:
        {
            reading_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CONNECTION_STATE_READING_WRITING:
        {
            reading_writing_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CONNECTION_STATE_CANCELLING:
        {
            cancelling_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CONNECTION_STATE_CLOSING:
        {
            closing_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CONNECTION_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}


static void fsm_cblk_init(
    sCHAT_CONNECTION_CBLK* master_cblk_ptr)
{
    eSTATUS status;

    status = network_watcher_start_watch(master_cblk_ptr->network_watcher,
                                         NETWORK_WATCHER_MODE_READ,
                                         master_cblk_ptr->fd);
    assert(STATUS_SUCCESS == status);
    master_cblk_ptr->state = CHAT_CONNECTION_STATE_READING;
}


static void fsm_cblk_close(
    sCHAT_CONNECTION_CBLK* master_cblk_ptr)
{
    eSTATUS status;

    fCHAT_CONNECTION_USER_CBACK user_cback;
    void*                       user_arg;

    status = network_watcher_close(master_cblk_ptr->network_watcher);
    assert(STATUS_SUCCESS == status);

    status = message_queue_destroy(master_cblk_ptr->event_queue);
    assert(STATUS_SUCCESS == status);

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);

    status = chat_event_io_close(master_cblk_ptr->io);
    assert(STATUS_SUCCESS == status);

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    generic_deallocator(master_cblk_ptr);

    user_cback(user_arg,
               CHAT_CONNECTION_EVENT_CLOSED,
               NULL);
}


void* chat_connection_thread_entry(
    void* arg)
{
    sCHAT_CONNECTION_CBLK*   master_cblk_ptr;
    sCHAT_CONNECTION_MESSAGE message;
    eSTATUS                  status;

    master_cblk_ptr = (sCHAT_CONNECTION_CBLK*)arg;
    fsm_cblk_init(master_cblk_ptr);

    while (CHAT_CONNECTION_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        dispatch_message(master_cblk_ptr, &message);
    }

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
