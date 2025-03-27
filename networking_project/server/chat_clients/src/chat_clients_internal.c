#include "chat_clients.h"
#include "chat_clients_internal.h"

#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>


static char k_server_name[] = "Server";


void chat_clients_process_event(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event)
{
    eSTATUS status;

    uint32_t      client_index;
    sCHAT_CLIENT* relevant_client;

    sCHAT_EVENT outgoing_event;

    sCHAT_CLIENTS_CBACK_DATA cback_data;
    sCHAT_CLIENT_AUTH*       auth_object;
    sCHAT_USER_CREDENTIALS*  credentials;

    switch(event->type)
    {
        case CHAT_EVENT_CHAT_MESSAGE:
        {
            if (CHAT_CLIENT_STATE_ACTIVE == source_client->state)
            {
                status = chat_event_copy(&outgoing_event, event);
                assert(STATUS_SUCCESS == status);

                status = chat_event_fill_origin(&outgoing_event,
                                                source_client->user_info.name,
                                                source_client->user_info.id);
                assert(STATUS_SUCCESS == status);

                for (client_index = 1;
                     client_index < master_cblk_ptr->max_clients;
                     client_index++)
                {
                    if (&master_cblk_ptr->client_ptr_list[client_index] == source_client)
                    {
                        continue;
                    }

                    relevant_client = master_cblk_ptr->client_ptr_list[client_index];

                    if (NULL != relevant_client && CHAT_CLIENT_STATE_ACTIVE == relevant_client->state)
                    {
                        status = chat_connection_queue_event(relevant_client->connection,
                                                             outgoing_event);
                    }
                    assert(STATUS_SUCCESS == status);
                }
            }
            break;
        }
        case CHAT_EVENT_USERNAME_REQUEST:
        {
            if (CHAT_CLIENT_STATE_ACTIVE == relevant_client->state)
            {
                outgoing_event.origin.id = CHAT_USER_ID_SERVER;

                status = chat_event_fill_origin(&outgoing_event, k_server_name, CHAT_USER_ID_SERVER);
                assert(STATUS_SUCCESS == status);

                status = chat_event_fill_content(&outgoing_event, CHAT_EVENT_USERNAME_SUBMIT, k_server_name);
                assert(STATUS_SUCCESS == status);

                status = chat_connection_queue_event(source_client->connection,
                                                     outgoing_event);
                assert(STATUS_SUCCESS == status);
            }
            break;
        }
        case CHAT_EVENT_USERNAME_SUBMIT:
        {
            if (CHAT_CLIENT_STATE_AUTHENTICATING != source_client->state)
            {
                // TODO send username rejected with already auth-ed/not in auth message
                break;
            }

            // FIXME move the allocation to when the client is moved into the authenticating state;
            //       deallocate when they're moved out
            cback_data.request_authentication.auth_object = generic_allocator(sizeof(sCHAT_CLIENT_AUTH_OPERATION));
            if (NULL == cback_data.request_authentication.auth_object)
            {
                status = chat_event_fill_origin(outgoing_event, k_server_name, CHAT_USER_ID_SERVER);
                assert(STATUS_SUCCESS == status);

                status = chat_event_fill_content(outgoing_event, CHAT_EVENT_SERVER_ERROR, NULL); // TODO decide what data goes here
                assert(STATUS_SUCCESS == status); // FIXME add real error handling

                status = chat_connection_queue_event(source_client->connection, &outgoing_event);
                assert(STATUS_SUCCESS == status);

                // REVIEW close the client?
                break;
            }
            pthread_mutex_init(&auth_object->mutex); // NOTE this needs to move with auth allocation

            // NOTE this is where what shouldn't be moved starts

            auth_object = source_client->auth;

            cback_data.request_authentication.auth_object = auth_object;
            cback_data.request_authentication.credentials = auth_object->credentials;

            pthread_mutex_lock(&auth_object->mutex);

            switch (auth_object->state)
            {
                case CHAT_CLIENT_AUTH_STATE_PASSWORD_ENTRY:
                {
                    generic_deallocator(auth_object->credentials.username);
                    auth_object->credentials.username      = NULL;
                    auth_object->credentials.username_size = 0;

                    auth_object->state = CHAT_CLIENT_AUTH_STATE_USERNAME_ENTRY;

                    // Fallthrough
                }
                case CHAT_CLIENT_AUTH_STATE_USERNAME_ENTRY:
                {
                    if (event->length > CHAT_USER_MAX_NAME_SIZE)
                    {
                        // TODO send error
                        break;
                    }

                    auth_object->credentials.username_size = event->length;
                    auth_object->credentials.username      = generic_allocator(auth_object->credentials.username_size);
                    if (NULL == auth_object->credentials.username)
                    {
                        // TODO send error
                        break;
                    }

                    status = print_string_to_buffer(auth_object->credentials.username,
                                                    event->data,
                                                    auth_object->credentials.username_size,
                                                    NULL);
                    assert(STATUS_SUCCESS == status);

                    auth_object->state = CHAT_CLIENT_AUTH_STATE_PROCESSING;
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_CLIENTS_EVENT_REQUEST_AUTHENTICATION,
                                                cback_data);
                    break;
                }
            }

            pthread_mutex_unlock(&auth_object->mutex);
            break;

            // REVIEW reuse this for auth complete?
            // if (event->length <= CHAT_CONNECTION_MAX_NAME_SIZE)
            // {
            //     // Compose name accepted message to joining user
            //     outgoing_event.type   = CHAT_EVENT_USERNAME_ACCEPTED;
            //     outgoing_event.origin = CHAT_EVENT_ORIGIN_SERVER;
            //     outgoing_event.length = event->length; // FIXME set this to a real message
            //     memcpy(outgoing_event->data,
            //            event->data,
            //            event->length);

            //     relevant_client = &master_cblk_ptr->connections[event->origin];

            //     status = message_queue_put(relevant_client->event_queue,
            //                                event,
            //                                sizeof(sCHAT_EVENT));
            //     assert(STATUS_SUCCESS == status);
            //     master_cblk_ptr->write_watches[client_index].active = true;

            //     relevant_client->state = CHAT_CONNECTION_STATE_ACTIVE;

            //     // Compose message to active users about joining user
            //     outgoing_event.type   = CHAT_EVENT_USER_JOIN;
            //     outgoing_event.length = event->length; // FIXME set this to a real message

            //     for (client_index = 1;
            //          client_index < master_cblk_ptr->max_connections;
            //          client_index++)
            //     {
            //         if (client_index == event->origin)
            //         {
            //             continue;
            //         }

            //         relevant_client = &master_cblk_ptr->connections[client_index];
            //         if (CHAT_CONNECTION_STATE_ACTIVE == relevant_client->state)
            //         {
            //             status = message_queue_put(relevant_client->event_queue,
            //                                     event,
            //                                     sizeof(sCHAT_EVENT));
            //             assert(STATUS_SUCCESS == status);
            //             master_cblk_ptr->write_watches[client_index].active = true;
            //         }
            //     }
            // }
            // else // event->length > CHAT_SERVER_CONNECTION_MAX_NAME_SIZE
            // {
            //     outgoing_event.origin = CHAT_EVENT_ORIGIN_SERVER;
            //     outgoing_event.type   = CHAT_EVENT_USERNAME_REJECTED;
            //     printed_length = snprintf((char*)&outgoing_event.data,
            //                               CHAT_EVENT_MAX_DATA_SIZE,
            //                               "%s",
            //                               &k_name_too_long_message[0]); // FIXME this canned message is in a different file
            //     assert(printed_length >= 0);
            //     outgoing_event.length = printed_length > CHAT_EVENT_MAX_DATA_SIZE
            //                             ? CHAT_EVENT_MAX_DATA_SIZE
            //                             : printed_length;

            //     relevant_client = &master_cblk_ptr->connections[event->origin];

            //     status = message_queue_put(relevant_client->event_queue,
            //                             event,
            //                             sizeof(sCHAT_EVENT));
            //     assert(STATUS_SUCCESS == status);
            //     master_cblk_ptr->write_watches[client_index].active = true;
            // }
        }
        case CHAT_EVENT_PASSWORD_SUBMIT:
        {
            if (CHAT_CLIENT_STATE_AUTHENTICATING != source_client->state)
            {
                // TODO send password rejected with already auth-ed/not in auth message
                break;
            }

            auth_object = source_client->auth;

            cback_data.request_authentication.auth_object = auth_object;
            cback_data.request_authentication.credentials = auth_object->credentials;

            pthread_mutex_lock(&auth_object->mutex);

            switch (auth_object->state)
            {
                case CHAT_CLIENT_AUTH_STATE_PASSWORD_ENTRY:
                {
                    auth_object->credentials.password_size = event->length;
                    auth_object->credentials.password      = generic_allocator(auth_object->credentials.password_size);
                    if (NULL == auth_object->credentials.password)
                    {
                        // TODO send error
                        break;
                    }

                    status = print_string_to_buffer(auth_object->credentials.password,
                                                    event->data,
                                                    auth_object->credentials.password_size,
                                                    NULL);
                    assert(STATUS_SUCCESS == status);

                    auth_object->state = CHAT_CLIENT_AUTH_STATE_PROCESSING;
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_CLIENTS_EVENT_REQUEST_AUTHENTICATION,
                                                cback_data);
                    break;
                }
            }

            pthread_mutex_unlock(&auth_object->mutex);
            break;
        }
        case CHAT_EVENT_USER_LIST:
        {
            // REVIEW implement this?
            break;
        }
        case CHAT_EVENT_USER_LEAVE:
        {
            source_client->state = CHAT_CLIENT_STATE_DISCONNECTING;
            chat_connection_close(source_client->connection);
            break;
        }

        // All these are no-op if sent to the server
        case CHAT_EVENT_UNDEFINED:
        case CHAT_EVENT_CONNECTION_FAILED:
        case CHAT_EVENT_OVERSIZED_CONTENT:
        case CHAT_EVENT_USERNAME_REJECTED:
        case CHAT_EVENT_PASSWORD_REJECTED:
        case CHAT_EVENT_SERVER_SHUTDOWN:
        case CHAT_EVENT_USER_JOIN:
        default:
        {
            break;
        }
    }
}


eSTATUS chat_clients_accept_new_connection(
    const sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    int*                      out_new_fd)
{
    struct sockaddr_storage incoming_address;
    socklen_t               incoming_address_size;
    int                     accept_status;

    accept_status = accept(master_cblk_ptr->connection_listen_fd,
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
