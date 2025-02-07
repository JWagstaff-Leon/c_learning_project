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


static void process_complete_event(
    const sCHAT_EVENT* event,
    CHAT_CONNECTION*   connections,
    uint32_t           connections_size,
    uint32_t           connection_index)
{
    uint32_t                        send_connection_index;
    eSTATUS                         status;
    sCHAT_SERVER_CONNECTION_MESSAGE message;
    sCHAT_SERVER_CONNECTION         current_connection;

    assert(NULL != event);

    assert(connection_index < connections_size);
    current_connection = connections[connection_index];
    assert(NULL != current_connection);

    switch(event->type)
    {
        case CHAT_EVENT_CHAT_MESSAGE:
        {
            if (CHAT_CONNECTION_STATE_ACTIVE == current_connection->state)
            {
                for (send_connection_index = 1;
                     send_connection_index < connections->size;
                     send_connection_index++)
                {
                    if (send_connection_index == event->origin)
                    {
                        continue;
                    }

                    status = chat_connection_event_io_write()
                    // TODO kept this around so that it can be moved into the chat_connection method for event_io writing
                    // message.type                    = CHAT_SERVER_CONNECTION_MESSAGE_SEND_EVENT;
                    // message.params.send_event.event = event;

                    // status = message_queue_put(connections->list[send_connection_index].message_queue,
                    //                            message,
                    //                            sizeof(message));
                    // assert(STATUS_SUCCESS == status);
                }
            }
            break;
        }
        case CHAT_EVENT_USERNAME_REQUEST:
        {
            if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == current_connection->state)
            {
                message.params.send_event.event.origin = CHAT_EVENT_ORIGIN_SERVER;
                message.params.send_event.event.type   = CHAT_EVENT_USERNAME_SUBMIT;
                // TODO do this to all snprintf's while applicable:
                // Use snprintf return to determine event length
                snprintf((char*)&message.params.send_event.event.data[0],
                         CHAT_EVENT_MAX_DATA_SIZE,
                         "%s",
                         connections->list[0].name);
                message.params.send_event.event.length = strnlen(&message.params.send_event.event.data[0],
                                                                 CHAT_EVENT_MAX_DATA_SIZE);

                send(current_connection->pollfd.fd,
                     &message.params.send_event.event,
                     CHAT_EVENT_HEADER_SIZE + message.params.send_event.event.length,
                     0);
            }
            break;
        }
        case CHAT_EVENT_USERNAME_SUBMIT:
        {
            if (CHAT_SERVER_CONNECTION_STATE_IN_SETUP == current_connection->state)
            {
                if (event->length <= CHAT_SERVER_CONNECTION_MAX_NAME_SIZE)
                {
                    message.type = CHAT_SERVER_CONNECTION_MESSAGE_SEND_EVENT;

                    // Compose name accepted message to joining user
                    message.params.send_event.event.type   = CHAT_EVENT_USERNAME_ACCEPTED;
                    message.params.send_event.event.origin = CHAT_EVENT_ORIGIN_SERVER;
                    message.params.send_event.event.length = event->length;
                    memcpy(message.params.send_event.event->data,
                           event->data,
                           event->length);

                    status = message_queue_put(current_connection->message_queue,
                                               message,
                                               sizeof(message));
                    assert(STATUS_SUCCESS == status);

                    current_connection->state = CHAT_SERVER_CONNECTION_STATE_ACTIVE;

                    // Compose message to active users about joining user
                    message.params.send_event.event.type   = CHAT_EVENT_USER_JOIN;
                    message.params.send_event.event.length = event->length;

                    for (send_connection_index = 1;
                         send_connection_index < connections->size;
                         send_connection_index++)
                    {
                        if (send_connection_index == connection_index)
                        {
                            continue;
                        }

                        if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == connections->list[send_connection_index].state)
                        {
                            status = message_queue_put(connections->list[send_connection_index].message_queue,
                                                       message,
                                                       sizeof(message));
                            assert(STATUS_SUCCESS == status);
                        }
                    }
                }
                else
                {
                    message.type = CHAT_SERVER_CONNECTION_MESSAGE_SEND_EVENT;

                    message.params.send_event.event.origin = CHAT_EVENT_ORIGIN_SERVER;
                    message.params.send_event.event.type   = CHAT_EVENT_USERNAME_REJECTED;
                    snprintf((char*)&message.params.send_event.event.data,
                             CHAT_EVENT_MAX_DATA_SIZE,
                             "%s",
                             &k_name_too_long_message[0]);
                    message.params.send_event.event.length = strnlen(message.params.send_event.event,
                                                                     CHAT_EVENT_MAX_DATA_SIZE);

                    status = message_queue_put(current_connection->message_queue,
                                               message,
                                               sizeof(message));
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

    sCHAT_EVENT           event_buffer;
    CHAT_CONNECTION       relevant_connection;
    sCHAT_EVENT_IO_RESULT chat_event_io_result;

    uint32_t connection_index;
    uint32_t fd_index;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_READ_READY:
        {
            for (fd_index = 0; fd_index < message->params.read_ready.index_count; fd_index++)
            {
                connection_index    = message->params.read_ready.connection_indecies[fd_index];
                relevant_connection = master_cblk_ptr->connections[connection_index];
                
                chat_event_io_result = chat_connection_event_io_read(relevant_connection);
                switch (chat_event_io_result.event)
                {
                    case CHAT_EVENT_IO_EVENT_READ_FINISHED:
                    {
                        chat_event_io_result.data.read_finished.read_event.origin = connection_index;
                        process_complete_event(&event_buffer);
                        break;
                    }
                    case CHAT_EVENT_IO_EVENT_FD_CLOSED:
                    {
                        // TODO respond to a close
                    }
                    case CHAT_EVENT_IO_EVENT_FAILED:
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
                    case CHAT_EVENT_IO_EVENT_WRITE_FINISHED:
                    {
                        event_buffer.origin = message->params.read_ready.connection_index;
                        process_complete_event(&event_buffer);
                        break;
                    }
                    case CHAT_EVENT_IO_EVENT_FD_CLOSED:
                    {
                        // TODO respond to a close
                    }
                    case CHAT_EVENT_IO_EVENT_FAILED:
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
    uint32_t                 connection_index;
    assert(NULL != master_cblk_ptr);

    master_cblk_ptr->connections.count = 0;
    master_cblk_ptr->connections.size  = CHAT_SERVER_DEFAULT_MAX_CONNECTIONS;

    master_cblk_ptr->connections.list = master_cblk_ptr->allocator(master_cblk_ptr->connections.size * sizeof(CHAT_CONNECTION));
    assert(NULL != master_cblk_ptr->connections); // FIXME make this a proper, recoverable operation

    for (connection_index = 0;
         connection_index < master_cblk_ptr->max_connections;
         connection_index++)
    {
        master_cblk_ptr->connections[connection_index] = NULL;
    }

    status = network_watcher_create(new_master_cblk_ptr->read_network_watcher,
                                    chat_server_network_watcher_read_cback,
                                    master_cblk_ptr,
                                    master_cblk_ptr->max_connections);
    assert(STATUS_SUCCESS == status); // FIXME make this a proper, recoverable operation

    status = network_watcher_create(new_master_cblk_ptr->write_network_watcher,
                                    chat_server_network_watcher_write_cback,
                                    master_cblk_ptr,
                                    master_cblk_ptr->max_connections);
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
