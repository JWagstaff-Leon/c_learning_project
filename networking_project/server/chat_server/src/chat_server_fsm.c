#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <stdlib.h>

#include "common_types.h"
#include "message_queue.h"


static void process_complete_event(
    const sCHAT_EVENT*        event,
    sCHAT_SERVER_CONNECTIONS* connections)
{
    uint32_t                        send_connection_index;
    eSTATUS                         status;
    sCHAT_SERVER_CONNECTION_MESSAGE message;

    assert(NULL != event);

    switch(event->type)
    {
        case CHAT_EVENT_CHAT_MESSAGE:
        {
            if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == current_connection->state)
            {
                for (send_connection_index = 1;
                     send_connection_index < connections->size;
                     send_connection_index++)
                {
                    if (send_connection_index == event->origin)
                    {
                        continue;
                    }

                    message.type                    = CHAT_SERVER_CONNECTION_MESSAGE_SEND_EVENT;
                    message.params.send_event.event = event;

                    status = message_queue_put(connections->list[send_connection_index].message_queue,
                                               message,
                                               sizeof(message));
                    assert(STATUS_SUCCESS == status);
                }
            }
            break;
        }
        case CHAT_EVENT_USERNAME_REQUEST:
        {
            if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == current_connection->state)
            {
                event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
                event_buffer.type   = CHAT_EVENT_USERNAME_SUBMIT;
                snprintf((char*)&event_buffer.data[0], CHAT_EVENT_MAX_DATA_SIZE, "%s", connections->list[0].name);
                event_buffer.length = strnlen(&event_buffer.data[0], CHAT_EVENT_MAX_DATA_SIZE - 1);

                send(current_connection->pollfd.fd,
                     &event_buffer,
                     CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                     0);
            }
            break;
        }
        case CHAT_EVENT_USERNAME_SUBMIT:
        {
            if (CHAT_SERVER_CONNECTION_STATE_IN_SETUP == current_connection->state)
            {
                if (current_connection->event_reader.event.length <= CHAT_SERVER_CONNECTION_MAX_NAME_SIZE)
                {
                    // Compose name accepted message to joining user
                    event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
                    event_buffer.type   = CHAT_EVENT_USERNAME_ACCEPTED;
                    snprintf((char*)&event_buffer.data[0], CHAT_EVENT_MAX_DATA_SIZE, "%s", k_server_event_canned_messages[CHAT_EVENT_USERNAME_ACCEPTED]);
                    event_buffer.length = strnlen(k_server_event_canned_messages[CHAT_EVENT_USERNAME_ACCEPTED], CHAT_EVENT_MAX_DATA_SIZE - 1);

                    send(current_connection->pollfd.fd,
                         &event_buffer,
                         CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                         0);

                    snprintf(current_connection->name,
                             sizeof(current_connection->name),
                             "%s",
                             &current_connection->event_reader.event.data[0]);

                    current_connection->state = CHAT_SERVER_CONNECTION_STATE_ACTIVE;

                    // Compose message to active users about joining user
                    event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
                    event_buffer.type   = CHAT_EVENT_USER_JOIN;
                    // TODO do this to all snprintf's while applicable:
                    // Use snprintf return to determine event length
                    snprintf((char*)&event_buffer.data[0],
                             CHAT_EVENT_MAX_DATA_SIZE,
                             "%s",
                             current_connection->name);
                    event_buffer.length = strnlen(&current_connection->name[0], CHAT_EVENT_MAX_DATA_SIZE - 1);

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
                            send(connections->list[send_connection_index].pollfd.fd,
                                 &event_buffer,
                                 CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                                 0);
                        }
                    }
                }
                else
                {
                    event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
                    event_buffer.type   = CHAT_EVENT_USERNAME_REJECTED;
                    snprintf((char*)&event_buffer.data, CHAT_EVENT_MAX_DATA_SIZE, "%s", &k_name_too_long_message[0]);
                    event_buffer.length = sizeof(k_name_too_long_message);

                    send(current_connection->pollfd.fd,
                         &event_buffer,
                         CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                         0);
                }
            }
            break;
        }
        case CHAT_EVENT_USER_LIST:
        {
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
            status = chat_server_network_open(&master_cblk_ptr->connections.list[0]);
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

    sCHAT_EVENT              event_buffer;
    sCHAT_SERVER_CONNECTION* relevant_connection;
    sCHAT_EVENT_IO_RESULT    chat_event_io_result;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_READ_READY:
        {
            relevant_connection = master_cblk_ptr->connections.list[message->params.read_ready.connection_index];
            chat_event_io_result = chat_event_io_do_operation_on_fd(&relevant_connection->event_reader,
                                                                    relevant_connection->fd,
                                                                    &event_buffer);
            switch (chat_event_io_result.event)
            {
                case CHAT_EVENT_IO_EVENT_READ_FINISHED:
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
    sCHAT_SERVER_CONNECTION* connections_list;
    sCHAT_SERVER_CONNECTION* current_connection;
    assert(NULL != master_cblk_ptr);

    connections_list = master_cblk_ptr->allocator(CHAT_SERVER_MAX_CONNECTIONS * sizeof(sCHAT_SERVER_CONNECTION));
    assert(NULL != connections_list);

    master_cblk_ptr->connections.list  = connections_list;
    master_cblk_ptr->connections.count = 0;
    master_cblk_ptr->connections.size  = 8;

    for (connection_index = 0;
         connection_index < master_cblk_ptr->connections.size;
         connection_index++)
    {
        current_connection  = &connections_list[connection_index];
        *current_connection = k_blank_user;

        status = chat_event_io_create(&current_connection->event_reader,
                                      master_cblk_ptr->allocator,
                                      CHAT_EVENT_IO_MODE_READER);
        assert(STATUS_SUCCESS == status);

        status = chat_event_io_create(&current_connection->event_writer,
                                      master_cblk_ptr->allocator,
                                      CHAT_EVENT_IO_MODE_WRITER);
        assert(STATUS_SUCCESS == status);

        status = message_queue_create(&current_connection->message_queue,
                                      CHAT_SERVER_CONNECTION_MESSAGE_QUEUE_SIZE,
                                      sizeof(sCHAT_SERVER_CONNECTION_MESSAGE),
                                      master_cblk_ptr->allocator,
                                      master_cblk_ptr->deallocator);
        assert(STATUS_SUCCESS == status);
    }
}


static void fsm_cblk_close(
    sCHAT_SERVER_CBLK* master_cblk_ptr)
{
    eSTATUS status;
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    assert(NULL != master_cblk_ptr);

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
