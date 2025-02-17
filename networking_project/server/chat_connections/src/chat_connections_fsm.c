#include "chat_connections.h"
#include "chat_connections_internal.h"
#include "chat_connections_fsm.h"

#include <assert.h>
#include <stddef.h>

#include "message_queue.h"


static eSTATUS fsm_cblk_init(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr)
{
    // TODO this
    return STAUTS_SUCCESS;
}


static void fsm_cblk_close(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr)
{
    fCHAT_CONNECTIONS_USER_CBACK user_cback;
    void*                        user_arg;

    assert(NULL != master_cblk_ptr);

    // TODO close the cblk

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    // TODO free the cblk

    user_cback(user_arg,
               CHAT_CONNECTIONS_EVENT_CLOSED,
               NULL);
}


static void process_event_from(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr,
    uint32_t                from_connection_index,
    const sCHAT_EVENT*      event)
{
    eSTATUS           status;
    uint32_t          send_connection_index;
    sCHAT_CONNECTION* current_connection;
    sCHAT_EVENT       outgoing_event;
    int               printed_length;

    assert(NULL != master_cblk_ptr);
    assert(NULL != event);
    assert(from_connection_index < master_cblk_ptr->max_connections);

    current_connection = master_cblk_ptr->connections[from_connection_index];
    switch(event->type)
    {
        case CHAT_EVENT_CHAT_MESSAGE:
        {
            if (CHAT_CONNECTION_STATE_ACTIVE == current_connection->state)
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


static void open_processing(
    sCHAT_CONNECTIONS_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTIONS_MESSAGE* message)
{
    eCHAT_EVENT_IO_RESULT chat_event_io_result;
    sCHAT_EVENT           event_buffer;

    sCHAT_CONNECTION* relevant_connection;

    uint32_t result_index;
    uint32_t connection_index;

    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_CONNECTIONS_MESSAGE_READ_READY:
        {
            for (result_index = 0; result_index < message->params.read_ready.index_count; result_index++)
            {
                connection_index    = message->params.read_ready.connection_indecies[result_index];
                relevant_connection = &master_cblk_ptr->connections[connection_index];

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
                                process_event_from(master_cblk_ptr,
                                                   relevant_connection,
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
        case CHAT_CONNECTIONS_MESSAGE_WRITE_READY:
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
    }
}


void dispatch_message(
    sCHAT_CONNECTIONS_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTIONS_MESSAGE* message)
{
    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (master_cblk_ptr->state)
    {
        case CHAT_CONNECTIONS_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
            break;
        }
    }
}


void* chat_connections_thread_entry(
    void* arg)
{
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr;
    eSTATUS                 status;

    sCHAT_CONNECTIONS_MESSAGE message;
    
    assert(NULL != arg);
    master_cblk_ptr = arg;

    status = fsm_cblk_init(master_cblk_ptr);
    assert(STATUS_SUCCESS == status);

    while (CHAT_CONNECTIONS_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
        dispatch_message(master_cblk_ptr, &message);
    }

    fsm_close_cblk(master_cblk_ptr);
    return NULL;
}
