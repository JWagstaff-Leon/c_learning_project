#include "chat_clients.h"
#include "chat_clients_internal.h"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
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
    char*                    new_credential;
    size_t                   new_credential_size;    

    switch(event->type)
    {
        case CHAT_EVENT_CHAT_MESSAGE:
        {
            if (CHAT_CLIENT_STATE_ACTIVE == source_client->state)
            {
                status = chat_event_copy(&outgoing_event, event);
                assert(STATUS_SUCCESS == status);

                outgoing_event->origin = source_client->user_info.id;

                for (client_index = 1;
                     client_index < master_cblk_ptr->max_clients;
                     client_index++)
                {
                    relevant_client = master_cblk_ptr->client_ptr_list[client_index];

                    if (relevant_client == source_client)
                    {
                        continue;
                    }

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
            if (CHAT_CLIENT_STATE_ACTIVE == source_client->state)
            {
                status = chat_event_populate(&outgoing_event,
                                             CHAT_EVENT_USERNAME_SUBMIT,
                                             CHAT_USER_ID_SERVER,
                                             k_server_name);
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

            if (event->length > CHAT_USER_MAX_NAME_SIZE)
            {
                // TODO send error
                break;
            }

            auth_object = source_client->auth;

            pthread_mutex_lock(&auth_object->mutex);

            if (CHAT_CLIENT_AUTH_STATE_CANCELLED == auth_object->state)
            {
                pthread_mutex_unlock(&auth_object->mutex);

                relevant_client = auth_object->client_ptr;
                assert(relevant_client == source_client);

                relevant_client->auth = NULL;

                generic_deallocator(auth_object->credentials.username);
                generic_deallocator(auth_object->credentials.password);
                generic_deallocator(auth_object);

                status = chat_event_populate(&outgoing_event,
                                             CHAT_EVENT_CONNECTION_FAILED,
                                             CHAT_USER_ID_SERVER,
                                             ""); // REVIEW add real message
                assert(STATUS_SUCCESS == status);

                status = chat_connection_queue_event(relevant_client->connection,
                                                     &outgoing_event);
                assert(STATUS_SUCCESS == status);
                
                status = chat_connection_close(relevant_client->connection);
                assert(STATUS_SUCCESS == status);
                
                break;
            }

            new_credential = generic_allocator(event->length);
            if (NULL == new_credential)
            {
                // TODO send error
                pthread_mutex_unlock(&auth_object->mutex);
                break;
            }
            new_credential_size = event->length;

            status = print_string_to_buffer(new_credential,
                                            event->data,
                                            new_credential_size,
                                            NULL);
            assert(STATUS_SUCCESS == status); // FIXME real error handling

            switch (auth_object->state)
            {
                case CHAT_CLIENT_AUTH_STATE_PASSWORD_ENTRY:
                {
                    // Free any previously entered password if going backwards in the flow
                    generic_deallocator(auth_object->credentials.password);

                    auth_object->credentials.password      = NULL;
                    auth_object->credentials.password_size = 0;

                    // Fallthrough
                }
                case CHAT_CLIENT_AUTH_STATE_USERNAME_ENTRY:
                {
                    generic_deallocator(auth_object->credentials.username);

                    auth_object->credentials.username      = new_credential;
                    auth_object->credentials.username_size = new_credential_size;

                    auth_object->state = CHAT_CLIENT_AUTH_STATE_PROCESSING;

                    cback_data.request_authentication.auth_object = auth_object;
                    cback_data.request_authentication.credentials = auth_object->credentials;

                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_CLIENTS_EVENT_REQUEST_AUTHENTICATION,
                                                cback_data);
                    break;
                }
                default:
                {
                    // TODO send bad state message
                }
            }

            pthread_mutex_unlock(&auth_object->mutex);
            break;
        }
        case CHAT_EVENT_PASSWORD_SUBMIT:
        {
            if (CHAT_CLIENT_STATE_AUTHENTICATING != source_client->state)
            {
                // TODO send password rejected with already auth-ed/not in auth message
                break;
            }

            auth_object = source_client->auth;

            pthread_mutex_lock(&auth_object->mutex);

            if (CHAT_CLIENT_AUTH_STATE_CANCELLED == auth_object->state)
            {
                relevant_client = auth_object->client_ptr;
                assert(relevant_client == source_client);

                relevant_client->auth = NULL;

                status = chat_event_populate(&outgoing_event,
                                             CHAT_EVENT_CONNECTION_FAILED,
                                             CHAT_USER_ID_SERVER,
                                             ""); // REVIEW add real message
                assert(STATUS_SUCCESS == status);

                status = chat_connection_queue_event(relevant_client->connection,
                                                     &outgoing_event);
                assert(STATUS_SUCCESS == status);
                
                status = chat_connection_close(relevant_client->connection);
                assert(STATUS_SUCCESS == status);
                
                break;
            }

            new_credential = generic_allocator(event->length);
            if (NULL == new_credential)
            {
                // TODO send error
                pthread_mutex_unlock(&auth_object->mutex);
                break;
            }
            new_credential_size = event->length;

            status = print_string_to_buffer(new_credential,
                                            event->data,
                                            new_credential_size,
                                            NULL);
            assert(STATUS_SUCCESS == status); // FIXME real error handling

            switch (auth_object->state)
            {
                case CHAT_CLIENT_AUTH_STATE_PASSWORD_ENTRY:
                {
                    generic_deallocator(auth_object->credentials.password);
                    
                    auth_object->credentials.password      = new_credential;
                    auth_object->credentials.password_size = new_credential_size;
                    
                    auth_object->state = CHAT_CLIENT_AUTH_STATE_PROCESSING;
                    
                    cback_data.request_authentication.auth_object = auth_object;
                    cback_data.request_authentication.credentials = auth_object->credentials;
                    
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_CLIENTS_EVENT_REQUEST_AUTHENTICATION,
                                                cback_data);
                    break;
                }
                case CHAT_CLIENT_AUTH_STATE_USERNAME_ENTRY:
                default:
                {
                    // TODO send bad state message
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


eSTATUS chat_clients_client_init(
    sCHAT_CLIENT**      client_container_ptr,
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    int                 fd)
{
    eSTATUS status;
    sCHAT_CLIENT* new_client;

    new_client = generic_allocator(sizeof(sCHAT_CLIENT));
    if (NULL == new_client)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_client;
    }
    memset(new_client, 0, sizeof(sCHAT_CLIENT));

    new_client->auth = generic_allocator(sizeof(sCHAT_CLIENT_AUTH));
    if (NULL == new_client->auth)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_auth;
    }
    memset(new_client->auth, 0, sizeof(sCHAT_CLIENT_AUTH));

    status = chat_connection_create(&new_client->connection,
                                    chat_clients_connection_cback,
                                    new_client,
                                    fd);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_connection;
    }

    new_client->state       = CHAT_CLIENT_STATE_INIT;
    new_client->auth->state = CHAT_CLIENT_AUTH_STATE_INIT;

    pthread_mutex_init(&new_client->auth->mutex);

    new_client->container_ptr   = client_container_ptr;
    new_client->master_cblk_ptr = master_cblk_ptr;

    *client_container_ptr = new_client;

    return STATUS_SUCCESS;

fail_create_connection:
    generic_deallocator(new_client->auth);

fail_alloc_auth:
    generic_deallocator(new_client);

fail_alloc_client:
    return status;
}


void chat_clients_client_close(
    sCHAT_CLIENT* client_ptr)
{
    eSTATUS  status;
    uint32_t client_index;

    if (NULL != client_ptr->auth)
    {
        pthread_mutex_lock(&client_ptr->auth->mutex);
        if (CHAT_CLIENT_AUTH_STATE_PROCESSING != client_ptr->auth->state)
        {
            pthread_mutex_unlock(&client_ptr->auth->mutex);
            pthread_mutex_destroy(&client_ptr->auth->mutex);
            generic_deallocator(client_ptr->auth->credentials.username);
            generic_deallocator(client_ptr->auth->credentials.password);
            generic_deallocator(client_ptr->auth);
        }
        else
        {
            client_ptr->auth->state = CHAT_CLIENT_AUTH_STATE_CANCELLED;
            pthread_mutex_unlock(&client_ptr->auth->mutex);
        }
    }

    client_ptr->container_ptr = NULL;
    generic_deallocator(client_ptr);
}
