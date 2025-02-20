#include "chat_connections.h"
#include "chat_connections_internal.h"

#include <sys/socket.h>
#include <sys/types.h>


void chat_connections_process_event_from(
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
            if (CHAT_CONNECTION_STATE_ACTIVE == current_connection->state)
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
            if (CHAT_CONNECTION_STATE_SETUP == current_connection->state)
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

                    status = chat_server_connection_queue_event(current_connection,
                                                                &outgoing_event);
                    assert(STATUS_SUCCESS == status);

                    current_connection->state = CHAT_CONNECTION_STATE_ACTIVE;

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

                        if (CHAT_CONNECTION_STATE_ACTIVE == connections->list[send_connection_index].state)
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


eSTATUS chat_connections_accept_new_connection(
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
