#include "chat_clients.h"
#include "chat_clients_internal.h"
#include "chat_clients_fsm.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "message_queue.h"


static const char* k_auth_strings[] = {
    "Enter your username",  // CHAT_CLIENTS_AUTH_RESULT_USERNAME_REQUIRED
    "Invalid username",     // CHAT_CLIENTS_AUTH_RESULT_USERNAME_REJECTED
    "Create your password", // CHAT_CLIENTS_AUTH_RESULT_PASSWORD_CREATION
    "Enter your password",  // CHAT_CLIENTS_AUTH_RESULT_PASSWORD_REQUIRED
    "Invalid password",     // CHAT_CLIENTS_AUTH_RESULT_PASSWORD_REJECTED
    "Logged in"             // CHAT_CLIENTS_AUTH_RESULT_AUTHENTICATED
};


static eSTATUS fsm_cblk_init(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    // TODO this
    return STATUS_SUCCESS;
}


static void fsm_cblk_close(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    fCHAT_CLIENTS_USER_CBACK user_cback;
    void*                        user_arg;

    assert(NULL != master_cblk_ptr);

    // TODO close the cblk

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    // TODO free the cblk

    user_cback(user_arg,
               CHAT_CLIENTS_EVENT_CLOSED,
               NULL);
}


// REVIEW does this function need to interact with multithreading?
static eSTATUS realloc_clients(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    uint32_t            new_max_clients)
{
    eSTATUS status;

    uint32_t       client_index;
    sCHAT_CLIENT** new_client_ptr_list;

    new_client_ptr_list = generic_allocator(sizeof(sCHAT_CLIENT*) * new_max_clients);
    if (NULL == new_client_ptr_list)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_client_list;
    }

    // Close any clients in excess of new_max_clients
    for (client_index = new_max_clients;
         client_index < master_cblk_ptr->connection_count;
         client_index++)
    {
        chat_clients_client_close(master_cblk_ptr->client_ptr_list[client_index]); // REVIEW need to close connections first?
    }

    // Copy any clients overlapping
    for (client_index = 0;
         client_index < master_cblk_ptr->max_connections && client_index < new_max_clients;
         client_index++)
    {
        new_client_ptr_list[client_index] = master_cblk_ptr->client_ptr_list[client_index];
    }

    // Initialize any new clients
    for (client_index = master_cblk_ptr->max_connections;
         client_index < new_max_clients;
         client_index++)
    {
        new_client_ptr_list[client_index] = NULL;
    }

    generic_deallocator(master_cblk_ptr->client_ptr_list);

    master_cblk_ptr->client_ptr_list = new_client_ptr_list;
    master_cblk_ptr->max_clients     = new_max_clients;

    return STATUS_SUCCESS;

fail_alloc_client_list:
    return status;
}


static void open_processing(
    sCHAT_CLIENTS_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENTS_MESSAGE* message)
{
    eSTATUS status;
    int     new_connection_fd;

    uint32_t       client_index;
    sCHAT_CLIENT** relevant_client_ptr;

    sCHAT_CLIENTS_CBACK_DATA cback_data;

    sCHAT_CLIENT_AUTH*       auth_object;
    sCHAT_CLIENT*            auth_client;
    sCHAT_CLIENTS_AUTH_EVENT auth_event;

    sCHAT_EVENT outgoing_event;

    switch (message->type)
    {
        case CHAT_CLIENTS_MESSAGE_OPEN_CLIENT:
        {
            if (master_cblk_ptr->client_count >= master_cblk_ptr->max_clients)
            {
                status = realloc_clients(master_cblk_ptr,
                                         master_cblk_ptr->max_clients + 10); // REVIEW make this '10' configurable?
                if (STATUS_SUCCESS != status)
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_CLIENTS_EVENT_CLIENT_OPEN_FAILED,
                                                NULL);
                    break;
                }
            }

            for (client_index = 1;
                 client_index < master_cblk_ptr->max_clients;
                 client_index++)
            {
                if (NULL == master_cblk_ptr->client_ptr_list[client_index])
                {
                    relevant_client_ptr = &master_cblk_ptr->client_ptr_list[client_index];
                    break;
                }
            }

            status = chat_clients_client_init(relevant_client_ptr,
                                              master_cblk_ptr,
                                              message->params.open_client.fd);
            if (STATUS_SUCCESS != status)
            {
                close(new_connection_fd);
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_CLIENTS_EVENT_CLIENT_OPEN_FAILED,
                                            NULL);
                break;
            }

            (*relevant_client_ptr)->state       = CHAT_CLIENT_STATE_AUTHENTICATING;
            (*relevant_client_ptr)->auth->state = CHAT_CLIENT_AUTH_STATE_PROCESSING;

            cback_data.request_authentication.auth_object = (*relevant_client_ptr)->auth;
            cback_data.request_authentication.credentials = &(*relevant_client_ptr)->auth->credentials;

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENTS_EVENT_REQUEST_AUTHENTICATION,
                                        &cback_data);
            break;
        }
        case CHAT_CLIENTS_MESSAGE_AUTH_EVENT:
        {
            auth_object = message->params.auth_result.auth_object;

            pthread_mutex_lock(&auth_ptr->mutex);

            auth_client = (sCHAT_CLIENT_AUTH*)message->params.auth_result.auth_object->client_ptr;

            if (CHAT_CLIENT_AUTH_STATE_CANCELLED == auth_ptr->state)
            {
                pthread_mutex_unlock(&auth_ptr->mutex);
                chat_clients_client_close(auth_client);
                break;
            }

            auth_event = message->params.auth_result.auth_event;

            status = chat_event_populate(&outgoing_event,
                                         CHAT_EVENT_USERNAME_REQUEST,
                                         CHAT_USER_ID_SERVER,
                                         k_auth_strings[auth_event->result]);
            assert(STATUS_SUCCESS == status);

            status = chat_connection_queue_event(auth_client_ptr->connection,
                                                 &outgoing_event);
            assert(STATUS_SUCCESS == status);

            switch (auth_event->result)
            {
                case CHAT_CLIENTS_AUTH_RESULT_USERNAME_REQUIRED:
                {
                    auth_ptr->state = CHAT_CLIENT_AUTH_STATE_USERNAME_ENTRY;
                    pthread_mutex_unlock(&auth_object->mutex);
                    break;
                }
                case CHAT_CLIENTS_AUTH_RESULT_USERNAME_REJECTED:
                {
                    auth_ptr->state = CHAT_CLIENT_AUTH_STATE_USERNAME_ENTRY;
                    pthread_mutex_unlock(&auth_object->mutex);
                    break;
                }
                case CHAT_CLIENTS_AUTH_RESULT_PASSWORD_CREATION:
                {
                    auth_ptr->state = CHAT_CLIENT_AUTH_STATE_PASSWORD_ENTRY;
                    pthread_mutex_unlock(&auth_object->mutex);
                    break;
                }
                case CHAT_CLIENTS_AUTH_RESULT_PASSWORD_REQUIRED:
                {
                    auth_ptr->state = CHAT_CLIENT_AUTH_STATE_PASSWORD_ENTRY;
                    pthread_mutex_unlock(&auth_object->mutex);
                    break;
                }
                case CHAT_CLIENTS_AUTH_RESULT_PASSWORD_REJECTED:
                {
                    auth_ptr->state = CHAT_CLIENT_AUTH_STATE_PASSWORD_ENTRY;
                    pthread_mutex_unlock(&auth_object->mutex);
                    break;
                }
                case CHAT_CLIENTS_AUTH_RESULT_AUTHENTICATED:
                {
                    auth_client->user_info.id = auth_event.user_info->id;

                    status = print_string_to_buffer(auth_client->user_info.name,
                                                    auth_event.user_info->name,
                                                    sizeof(auth_client->user_info.name),
                                                    NULL);
                    assert(STATUS_SUCCESS == status);

                    auth_client->auth  = NULL;
                    auth_client->state = CHAT_CLIENT_STATE_ACTIVE;

                    pthread_mutex_unlock(&auth_object->mutex);
                    pthread_mutex_destroy(&auth_object->mutex);

                    generic_deallocator(auth_object->credentials.username);
                    generic_deallocator(auth_object->credentials.password);
                    generic_deallocator(auth_object);

                    break;
                }
            }

            break;
        }
        case CHAT_CLIENTS_MESSAGE_INCOMING_EVENT:
        {
            chat_clients_process_event(master_cblk_ptr,
                                       message->params.incoming_event.client_ptr,
                                       &message->params.incoming_event.event);
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED:
        {
            status = chat_clients_client_close(message->params.client_closed.client_ptr);
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLOSE:
        {
            // TODO start closing
            break;
        }
        default:
        {
            // Should not get here
            assert(0);
            break;
        }
    }
}


static void closing_processing(
    sCHAT_CLIENTS_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENTS_MESSAGE* message)
{
    eSTATUS status;

    switch (message->type)
    {
        case CHAT_CLIENTS_MESSAGE_INCOMING_EVENT:
        {
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED:
        {
            break;
        }
    }
}


void dispatch_message(
    sCHAT_CLIENTS_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENTS_MESSAGE* message)
{
    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (master_cblk_ptr->state)
    {
        case CHAT_CLIENTS_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CLIENTS_STATE_CLOSING:
        {
            closing_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CLIENTS_STATE_CLOSED:
        {
            // Should never get here
            assert(0);
        }
    }
}


void* chat_clients_thread_entry(
    void* arg)
{
    sCHAT_CLIENTS_CBLK* master_cblk_ptr;
    eSTATUS                 status;

    sCHAT_CLIENTS_MESSAGE message;

    assert(NULL != arg);
    master_cblk_ptr = arg;

    status = fsm_cblk_init(master_cblk_ptr);
    assert(STATUS_SUCCESS == status);

    while (CHAT_CLIENTS_STATE_CLOSED != master_cblk_ptr->state)
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
