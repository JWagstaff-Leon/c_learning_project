#include "chat_connection.h"
#include "chat_connection_internal.h"

#include <assert.h>
#include <poll.h>
#include <stdio.h>

#include "chat_event.h"
#include "chat_event_io.h"
#include "common_types.h"
#include "message_queue.h"


static void watch_complete(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    uCHAT_CONNECTION_CBACK_DATA cback_data;

    bCHAT_EVENT_IO_RESULT main_io_result;
    bCHAT_EVENT_IO_RESULT extract_result;

    if (message->params.watch_complete.revents & POLLOUT)
    {
        // Write into the IO
        // If complete, start another write if the buffer has more to write
    }
    if (message->params.watch_complete.revents & POLLIN)
    {
        do
        {
            main_io_result = chat_event_io_read_from_fd(master_cblk_ptr->io,
                                                        master_cblk_ptr->fd);

            if (main_io_result & CHAT_EVENT_IO_RESULT_FAILED)
            {
                assert(0);
                // REVIEW do something here? Logging?
                break;
            }
            if (main_io_result & CHAT_EVENT_IO_RESULT_FD_CLOSED)
            {
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_CONNECTION_EVENT_CONNECTION_CLOSED,
                                            NULL);
                break;
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
                                                    cback_data);
                    }
                } while (extract_result & CHAT_EVENT_IO_RESULT_EXTRACT_MORE);
            }
        } while (main_io_result & CHAT_EVENT_IO_RESULT_READ_FINISHED);
    }
}


static void open_processing(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    eSTATUS status;
    int     snprintf_status;

    sCHAT_EVENT           event_buffer;
    eNETWORK_WATCHER_MODE watch_mode;

    switch (message->type)
    {
        case CHAT_CONNECTION_MESSAGE_TYPE_WATCH_COMPLETE:
        {
            watch_complete(master_cblk_ptr, message);

            if (message_queue_get_count(master_cblk_ptr->event_queue) > 0)
            {
                watch_mode = NETWORK_WATCHER_MODE_BOTH;
            }
            else
            {
                watch_mode = NETWORK_WATCHER_MODE_READ;
            }
            status = network_watcher_start_watch(master_cblk_ptr->network_watcher,
                                                 watch_mode,
                                                 master_cblk_ptr->fd);
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_QUEUE_EVENT:
        {
            // TODO check if cancelling the watch is needed, and transition states if so

            status = message_queue_put(master_cblk_ptr->event_queue,
                                       &event_buffer,
                                       sizeof(event_buffer));
            assert(STATUS_SUCCESS == status);

            break;
        }
    }
}


static void cancelling_processing(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    switch (message->type)
    {
        case CHAT_CONNECTION_MESSAGE_TYPE_WATCH_COMPLETE:
        {
            watch_complete(master_cblk_ptr, message);
            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_WATCH_CANCELLED:
        {
            master_cblk_ptr->state = CHAT_CONNECTION_STATE_OPEN;
            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_QUEUE_EVENT:
        {
            break;
        }
        case CHAT_CONNECTION_MESSAGE_TYPE_CLOSE:
        {
            break;
        }
    }
}


static void closing_processing(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    switch (message->type)
    {
        // TODO this
    }
}


static void dispatch_message(
    sCHAT_CONNECTION_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTION_MESSAGE* message)
{
    switch (master_cblk_ptr->state)
    {
        case CHAT_CONNECTION_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
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
    // TODO this
}


static void fsm_cblk_close(
    sCHAT_CONNECTION_CBLK* master_cblk_ptr)
{
    // TODO this
}


void* chat_connection_thread_entry(
    void* arg)
{
    sCHAT_CONNECTION_CBLK    master_cblk_ptr;
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
