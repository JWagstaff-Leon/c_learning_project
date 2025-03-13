#include "chat_clients.h"
#include "chat_clients_internal.h"

#include <sys/socket.h>
#include <sys/types.h>


void chat_clients_process_event(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       client_ptr,
    const sCHAT_EVENT*  event)
{
    eSTATUS     status;
    uint32_t    connection_index;
    sCHAT_EVENT outgoing_event;
    int         printed_length;

    switch(event->type)
    {
        case CHAT_EVENT_CHAT_MESSAGE:
        {
            if (CHAT_CONNECTION_STATE_ACTIVE == master_cblk_ptr->connections[event->origin].state)
            {
                memcpy(&outgoing_event, event, sizeof(sCHAT_EVENT));

                for (connection_index = 1;
                     connection_index < master_cblk_ptr->max_connections;
                     connection_index++)
                {
                    if (connection_index == event->origin)
                    {
                        continue;
                    }

                    relevant_connection = &master_cblk_ptr->connections[connection_index];

                    status = message_queue_put(relevant_connection->event_queue,
                                               event,
                                               sizeof(sCHAT_EVENT));
                    assert(STATUS_SUCCESS == status);
                    master_cblk_ptr->write_watches[connection_index].active = true;
                }
            }
            break;
        }
        case CHAT_EVENT_USERNAME_REQUEST:
        {
            if (CHAT_CONNECTION_STATE_ACTIVE == relevant_connection->state)
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

                relevant_connection = &master_cblk_ptr->connections[event->origin];

                status = message_queue_put(relevant_connection->event_queue,
                                           event,
                                           sizeof(sCHAT_EVENT));
                assert(STATUS_SUCCESS == status);
                master_cblk_ptr->write_watches[connection_index].active = true;
            }
            break;
        }
        case CHAT_EVENT_USERNAME_SUBMIT:
        {
            if (CHAT_CONNECTION_STATE_SETUP == master_cblk_ptr->connections[event->origin].state)
            {
                if (event->length <= CHAT_CONNECTION_MAX_NAME_SIZE)
                {
                    // Compose name accepted message to joining user
                    outgoing_event.type   = CHAT_EVENT_USERNAME_ACCEPTED;
                    outgoing_event.origin = CHAT_EVENT_ORIGIN_SERVER;
                    outgoing_event.length = event->length; // FIXME set this to a real message
                    memcpy(outgoing_event->data,
                           event->data,
                           event->length);

                    relevant_connection = &master_cblk_ptr->connections[event->origin];

                    status = message_queue_put(relevant_connection->event_queue,
                                               event,
                                               sizeof(sCHAT_EVENT));
                    assert(STATUS_SUCCESS == status);
                    master_cblk_ptr->write_watches[connection_index].active = true;

                    relevant_connection->state = CHAT_CONNECTION_STATE_ACTIVE;

                    // Compose message to active users about joining user
                    outgoing_event.type   = CHAT_EVENT_USER_JOIN;
                    outgoing_event.length = event->length; // FIXME set this to a real message

                    for (connection_index = 1;
                         connection_index < master_cblk_ptr->max_connections;
                         connection_index++)
                    {
                        if (connection_index == event->origin)
                        {
                            continue;
                        }

                        relevant_connection = &master_cblk_ptr->connections[connection_index];
                        if (CHAT_CONNECTION_STATE_ACTIVE == relevant_connection->state)
                        {
                            status = message_queue_put(relevant_connection->event_queue,
                                                    event,
                                                    sizeof(sCHAT_EVENT));
                            assert(STATUS_SUCCESS == status);
                            master_cblk_ptr->write_watches[connection_index].active = true;
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

                    relevant_connection = &master_cblk_ptr->connections[event->origin];

                    status = message_queue_put(relevant_connection->event_queue,
                                            event,
                                            sizeof(sCHAT_EVENT));
                    assert(STATUS_SUCCESS == status);
                    master_cblk_ptr->write_watches[connection_index].active = true;
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


eSTATUS chat_clients_accept_new_connection(
    int  listen_fd,
    int* out_new_fd)
{
    struct sockaddr_storage incoming_address;
    socklen_t               incoming_address_size;
    int                     accept_status;

    accept_status = accept(listen_fd,
                          &incoming_address,
                          &incoming_address_size);
    if (accept_status < 0)
    {
        return STATUS_FAILURE;
    }

    *out_new_fd = accept_status;
    return STATUS_SUCCESS;
}


eSTATUS chat_clients_client_open(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       client,
    int                 fd)
{
    eSTATUS status;

    sCHAT_CLIENTS_CLIENT_CBACK_ARG* user_arg;
    user_arg = generic_allocator(sizeof(sCHAT_CLIENTS_CLIENT_CBACK_ARG));

    if (NULL == user_arg)
    {
        return STATUS_ALLOC_FAILED;
    }

    user_arg->master_cblk_ptr = master_cblk_ptr;
    user_arg->client_ptr      = client;

    status = chat_connection_create(&client->connection,
                                    chat_clients_connection_cback,
                                    user_arg,
                                    fd);
    assert(STATUS_SUCCESS == status);
}
