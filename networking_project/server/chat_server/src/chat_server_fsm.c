#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chat_connection.h"
#include "chat_event.h"
#include "common_types.h"
#include "message_queue.h"


static void process_complete_event_from(
    sCHAT_SERVER_CONNECTIONS* connections,
    uint32_t                  from_connection_index,
    const sCHAT_EVENT*        event)
{
    eSTATUS                 status;
    uint32_t                send_connection_index;
    sCHAT_SERVER_CONNECTION current_connection;
    sCHAT_EVENT             outgoing_event;
    int                     printed_length;

    assert(NULL != event);
    assert(NULL != connections);
    assert(from_connection_index < connections->size);

    sCHAT_SERVER_CONNECTION current_connection = connections->list[from_connection_index];
    switch(event->type)
    {
        case CHAT_EVENT_CHAT_MESSAGE:
        {
            if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == current_connection->state)
            {
                memcpy(&outgoing_event, event, sizeof(sCHAT_EVENT));
                outgoing_event.origin = from_connection_index;

                for (send_connection_index = 1;
                     send_connection_index < connections->size;
                     send_connection_index++)
                {
                    if (send_connection_index == from_connection_index)
                    {
                        continue;
                    }

                    status = chat_server_connection_queue_event(connections->list[send_connection_index],
                                                                &outgoing_event);
                    assert(STATUS_SUCCESS == status);
                }
            }
            break;
        }
        case CHAT_EVENT_USERNAME_REQUEST:
        {
            if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == current_connection->state)
            {
                outgoing_event.origin = CHAT_EVENT_ORIGIN_SERVER;
                outgoing_event.type   = CHAT_EVENT_USERNAME_SUBMIT;
                printed_length = snprintf((char*)&outgoing_event.data[0],
                                          CHAT_EVENT_MAX_DATA_SIZE,
                                          "%s",
                                          connections->list[0].name);
                assert(printed_length >= 0);
                outgoing_event.length = printed_length > CHAT_EVENT_MAX_DATA_SIZE
                                          ? CHAT_EVENT_MAX_DATA_SIZE
                                          : printed_length;

                status = chat_server_connection_queue_event(current_connection,
                                                            &outgoing_event);
                assert(STATUS_SUCCESS == status);
            }
            break;
        }
        case CHAT_EVENT_USERNAME_SUBMIT:
        {
            if (CHAT_SERVER_CONNECTION_STATE_SETUP == current_connection->state)
            {
                if (event->length <= CHAT_SERVER_CONNECTION_MAX_NAME_SIZE)
                {
                    // Compose name accepted message to joining user
                    outgoing_event.type   = CHAT_EVENT_USERNAME_ACCEPTED;
                    outgoing_event.origin = CHAT_EVENT_ORIGIN_SERVER;
                    outgoing_event.length = event->length; // FIXME set this to a real message
                    memcpy(outgoing_event->data,
                           event->data,
                           event->length);

                    status = chat_server_connection_queue_event(current_connection,
                                                                &outgoing_event);
                    assert(STATUS_SUCCESS == status);

                    current_connection->state = CHAT_SERVER_CONNECTION_STATE_ACTIVE;

                    // Compose message to active users about joining user
                    outgoing_event.type   = CHAT_EVENT_USER_JOIN;
                    outgoing_event.length = event->length; // FIXME set this to a real messgae

                    for (send_connection_index = 1;
                         send_connection_index < connections->size;
                         send_connection_index++)
                    {
                        if (send_connection_index == from_connection_index)
                        {
                            continue;
                        }

                        if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == connections->list[send_connection_index].state)
                        {
                            status = chat_server_connection_queue_event(&connections->list[send_connection_index],
                                                                        outgoing_event);
                            assert(STATUS_SUCCESS == status);
                        }
                    }
                }
                else // event->length > CHAT_SERVER_CONNECTION_MAX_NAME_SIZE
                {
                    outgoing_event.origin = CHAT_EVENT_ORIGIN_SERVER;
                    outgoing_event.type   = CHAT_EVENT_USERNAME_REJECTED;
                    printed_length = snprintf((char*)&outgoing_event.data,
                                              CHAT_EVENT_MAX_DATA_SIZE,
                                              "%s",
                                              &k_name_too_long_message[0]); // FIXME this canned message is in a different file
                    assert(printed_length >= 0);
                    outgoing_event.length = printed_length > CHAT_EVENT_MAX_DATA_SIZE
                                            ? CHAT_EVENT_MAX_DATA_SIZE
                                            : printed_length;
                    
                    status = chat_server_connection_queue_event(current_connection,
                                                                outgoing_event);
                    assert(STATUS_SUCCESS == status);
                }
            }
            break;
        }
        case CHAT_EVENT_USER_LIST:
        {
            // TODO implement this
            break;
        }

        // All these are no-op if sent to the server
        case CHAT_EVENT_UNDEFINED:
        case CHAT_EVENT_CONNECTION_FAILED:
        case CHAT_EVENT_OVERSIZED_CONTENT:
        case CHAT_EVENT_USERNAME_ACCEPTED:
        case CHAT_EVENT_USERNAME_REJECTED:
        case CHAT_EVENT_SERVER_SHUTDOWN:
        case CHAT_EVENT_USER_JOIN:
        case CHAT_EVENT_USER_LEAVE:
        default:
        {
            break;
        }
    }
}


static void init_processing(
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE new_message;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_OPEN:
        {
            status = chat_server_network_open(master_cblk_ptr);
            if(STATUS_SUCCESS != status)
            {
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_SERVER_EVENT_OPEN_FAILED,
                                            NULL);
                break;
            }

            master_cblk_ptr->connections.count = 1;

            status = chat_server_network_start_network_watch(master_cblk_ptr);
            if (STATUS_SUCCESS != status)
            {
                chat_server_network_stop_network_watch(master_cblk_ptr);
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_SERVER_EVENT_OPEN_FAILED,
                                            NULL);
                break;
            }

            master_cblk_ptr->state = CHAT_SERVER_STATE_OPEN;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_SERVER_EVENT_OPENED,
                                        NULL);
            break;
        }
        case CHAT_SERVER_MESSAGE_CLOSE:
        {
            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSED;
            break;
        }
        case CHAT_SERVER_MESSAGE_READ_READY:
        case CHAT_SERVER_MESSAGE_WRITE_READY:
        {
            // Expect, but ignore these
            break;
        }
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}


static void open_processing(
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE new_message;

    eCHAT_EVENT_IO_RESULT chat_event_io_result;
    sCHAT_EVENT           event_buffer;

    uint32_t result_index;
    uint32_t connection_index;

    sCHAT_SERVER_CONNECTION* relevant_connection;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_READ_READY:
        {
            for (result_index = 0; result_index < message->params.read_ready.index_count; result_index++)
            {
                connection_index    = message->params.read_ready.connection_indecies[result_index];
                relevant_connection = &master_cblk_ptr->connections.list[connection_index];

                chat_event_io_result = chat_event_io_read_from_fd(relevant_connection->io,
                                                                  relevant_connection->fd);
                switch (chat_event_io_result.event)
                {
                    case CHAT_EVENT_IO_RESULT_READ_FINISHED:
                    {
                        do
                        {
                            chat_event_io_result = chat_event_io_extract_read_event(relevant_connection->io,
                                                                                    &event_buffer);
                            if (CHAT_EVENT_IO_RESULT_EXTRACT_FINISHED & chat_event_io_result)
                            {
                                process_complete_event_from(&master_cblk_ptr->connections,
                                                            connection_index,
                                                            &event_buffer);
                            }
                        } while (CHAT_EVENT_IO_RESULT_EXTRACT_MORE & chat_event_io_result);
                        break;
                    }
                    case CHAT_EVENT_IO_RESULT_FD_CLOSED:
                    {
                        // TODO respond to a close
                    }
                    case CHAT_EVENT_IO_RESULT_FAILED:
                    {
                        // TODO respond to a failure
                    }
                    // REVIEW should this respond to flushing conditions?
                }
            }

            for (connection_index = 0, fd_index = 0;
                 connection_index < master_cblk_ptr->connections.size && fd_index < sizeof(fds_list) / sizeof(fds_list[0]),
                 connection_index++)
            {
                if (CHAT_SERVER_CONNECTION_STATE_DISCONNECTED != master_cblk_ptr->connections.list[connection_index].state)
                {
                    fds_list[fd_index++] = master_cblk_ptr->connections.list[connection_index].fd;
                }
            }

            generic_deallocator(message->params.read_ready.connection_indecies);
            status = chat_server_network_start_network_watch(master_cblk_ptr);
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_SERVER_MESSAGE_WRITE_READY:
        {
            for (fd_index = 0; fd_index < message->params.write_ready.index_count; fd_index++)
            {
                connection_index     = message->params.write_ready.connection_indecies[fd_index];
                relevant_connection  = master_cblk_ptr->connections[connection_index].connection;

                chat_event_io_result = chat_event_io_do_operation_on_fd(&relevant_connection->event_writer,
                                                                        relevant_connection->fd,
                                                                        &event_buffer);
                switch (chat_event_io_result.event)
                {
                    case CHAT_EVENT_IO_RESULT_WRITE_FINISHED:
                    {
                        // TODO nothing?
                    }
                    case CHAT_EVENT_IO_RESULT_FD_CLOSED:
                    {
                        // TODO respond to a close
                    }
                    case CHAT_EVENT_IO_RESULT_FAILED:
                    {
                        // TODO respond to a failure
                    }
                    // REVIEW should this respond to flushing conditions?
                }
            }

            for (connection_index = 1, fd_index = 0; // Start at 1 for write as there's no reason to write to incoming connection socket
                 connection_index < master_cblk_ptr->connections.size && fd_index < sizeof(fds_list) / sizeof(fds_list[0]),
                 connection_index++)
            {
                if (CHAT_SERVER_CONNECTION_STATE_DISCONNECTED != master_cblk_ptr->connections.list[connection_index].state)
                {
                    fds_list[fd_index++] = master_cblk_ptr->connections.list[connection_index].fd;
                }
            }

            status = network_watcher_start_watch(master_cblk_ptr->read_network_watcher,
                                                 NETWORK_WATCHER_MODE_WRITE,
                                                 &fds_list[0],
                                                 fd_index);
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_SERVER_MESSAGE_CLOSE:
        {
            chat_server_network_close(&master_cblk_ptr->connections);

            status = message_queue_destroy(master_cblk_ptr->message_queue);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSED;
            break;
        }
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}


static void dispatch_message(
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
{
    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (master_cblk_ptr->state)
    {
        case CHAT_SERVER_STATE_INIT:
        {
            init_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_SERVER_STATE_OPEN:
        {
            open_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_SERVER_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
            break;
        }
    }
}


static void fsm_cblk_init(
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    eSTATUS                  status;

    assert(NULL != master_cblk_ptr);

    status = chat_connections_create(&master_cblk_ptr->connections,
                                     chat_server_connections_cback,
                                     master_cblk_ptr,
                                     CHAT_SERVER_DEFAULT_MAX_CONNECTIONS);
    assert(STATUS_SUCCESS == status); // FIXME make this a proper, recoverable operation
}


static void fsm_cblk_close(
    sCHAT_SERVER_CBLK* master_cblk_ptr)
{
    eSTATUS                 status;
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    assert(NULL != master_cblk_ptr);

    status = network_watcher_close(master_cblk_ptr->write_network_watcher);
    assert(STATUS_SUCCESS == status);

    status = network_watcher_close(master_cblk_ptr->read_network_watcher);
    assert(STATUS_SUCCESS == status);

    chat_server_network_close(&master_cblk_ptr->connections);

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);

    free(master_cblk_ptr->connections.list);

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    master_cblk_ptr->deallocator(master_cblk_ptr);

    user_cback(user_arg,
               CHAT_SERVER_EVENT_CLOSED,
               NULL);
}


void* chat_server_thread_entry(
    void* arg)
{
    sCHAT_SERVER_CBLK *master_cblk_ptr;
    sCHAT_SERVER_MESSAGE message;
    eSTATUS status;

    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_SERVER_CBLK*)arg;

    fsm_cblk_init(master_cblk_ptr);

    while (CHAT_SERVER_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
        dispatch_message(&message, master_cblk_ptr);
    }

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
