#include "chat_clients.h"
#include "chat_clients_internal.h"
#include "chat_clients_fsm.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"
#include "message_queue.h"


static const char* k_auth_strings[] = {
    "Enter your username",  // CHAT_CLIENTS_AUTH_STEP_USERNAME_REQUIRED
    "Invalid username",     // CHAT_CLIENTS_AUTH_STEP_USERNAME_REJECTED
    "Create your password", // CHAT_CLIENTS_AUTH_STEP_PASSWORD_CREATION
    "Enter your password",  // CHAT_CLIENTS_AUTH_STEP_PASSWORD_REQUIRED
    "Invalid password",     // CHAT_CLIENTS_AUTH_STEP_PASSWORD_REJECTED
    "Logged in",            // CHAT_CLIENTS_AUTH_STEP_AUTHENTICATED
    ""                      // CHAT_CLIENTS_AUTH_STEP_CLOSED
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

    uint32_t            client_index;
    sCHAT_CLIENT_ENTRY* new_client_list;

    new_client_list = generic_allocator(sizeof(sCHAT_CLIENT_ENTRY) * new_max_clients);
    if (NULL == new_client_list)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_client_list;
    }

    // Close any clients in excess of new_max_clients
    for (client_index = new_max_clients;
         client_index < master_cblk_ptr->client_count;
         client_index++)
    {
        chat_clients_client_close(master_cblk_ptr->client_list[client_index].client_ptr); // REVIEW need to close connections first?
    }

    // Copy any clients overlapping
    for (client_index = 0;
         client_index < master_cblk_ptr->max_clients && client_index < new_max_clients;
         client_index++)
    {
        new_client_list[client_index] = master_cblk_ptr->client_list[client_index];
    }

    // Initialize any new clients
    for (client_index = master_cblk_ptr->max_clients;
         client_index < new_max_clients;
         client_index++)
    {
        new_client_list[client_index].client_ptr      = NULL;
        new_client_list[client_index].master_cblk_ptr = master_cblk_ptr;
    }

    generic_deallocator(master_cblk_ptr->client_list);

    master_cblk_ptr->client_list = new_client_list;
    master_cblk_ptr->max_clients = new_max_clients;

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

    eCHAT_CLIENTS_AUTH_STEP  auth_step;
    sCHAT_CLIENT*            auth_client;

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
                if (NULL == master_cblk_ptr->client_list[client_index].client_ptr)
                {
                    relevant_client_ptr = &master_cblk_ptr->client_list[client_index].client_ptr;
                    master_cblk_ptr->client_list[client_index].master_cblk_ptr;
                    break;
                }
            }

            // TODO update this to use client entry
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

            (*relevant_client_ptr)->state = CHAT_CLIENT_STATE_AUTHENTICATING_INIT;

            cback_data.start_auth_transaction.auth_transaction_container = &(*relevant_client_ptr)->auth_transaction;

            cback_data.start_auth_transaction.credentials.username      = NULL;
            cback_data.start_auth_transaction.credentials.username_size = 0;

            cback_data.start_auth_transaction.credentials.password      = NULL;
            cback_data.start_auth_transaction.credentials.password_size = 0;

            cback_data.start_auth_transaction.client_ptr = relevant_client_ptr;

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENTS_EVENT_START_AUTH_TRANSACTION,
                                        &cback_data);
            break;
        }
        case CHAT_CLIENTS_MESSAGE_AUTH_EVENT:
        {
            auth_step   = message->params.auth_event.auth_step;
            auth_client = message->params.auth_event.client;

            switch (auth_step)
            {
                case CHAT_CLIENTS_AUTH_STEP_USERNAME_REQUIRED:
                case CHAT_CLIENTS_AUTH_STEP_USERNAME_REJECTED:
                {
                    auth_client->state = CHAT_CLIENT_STATE_AUTHENTICATING_USERNAME;

                    status = chat_event_populate(&outgoing_event,
                                                 CHAT_EVENT_USERNAME_REQUEST,
                                                 CHAT_USER_ID_SERVER,
                                                 k_auth_strings[auth_step]);
                    assert(STATUS_SUCCESS == status);

                    status = chat_connection_queue_event(auth_client->connection,
                                                         &outgoing_event);
                    break;
                }
                case CHAT_CLIENTS_AUTH_STEP_PASSWORD_CREATION:
                case CHAT_CLIENTS_AUTH_STEP_PASSWORD_REQUIRED:
                case CHAT_CLIENTS_AUTH_STEP_PASSWORD_REJECTED:
                {
                    auth_client->state = CHAT_CLIENT_STATE_AUTHENTICATING_PASSWORD;

                    status = chat_event_populate(&outgoing_event,
                                                 CHAT_EVENT_PASSWORD_REQUEST,
                                                 CHAT_USER_ID_SERVER,
                                                 k_auth_strings[auth_step]);
                    assert(STATUS_SUCCESS == status);

                    status = chat_connection_queue_event(auth_client->connection,
                                                         &outgoing_event);
                    break;
                }
                case CHAT_CLIENTS_AUTH_STEP_AUTHENTICATED:
                {
                    auth_client->state = CHAT_CLIENT_STATE_ACTIVE;

                    status = chat_event_populate(&outgoing_event,
                                                 CHAT_EVENT_AUTHENTICATED,
                                                 CHAT_USER_ID_SERVER,
                                                 k_auth_strings[auth_step]);
                    assert(STATUS_SUCCESS == status);

                    status = chat_connection_queue_event(auth_client->connection,
                                                         &outgoing_event);
                    break;
                }
                case CHAT_CLIENTS_AUTH_STEP_CLOSED:
                {
                    generic_deallocator(auth_client->auth_credentials->username);
                    generic_deallocator(auth_client->auth_credentials->password);
                    generic_deallocator(auth_client->auth_credentials);

                    auth_client->auth_credentials = NULL;
                    break;
                }

                cback_data.finish_auth_transaction.client_ptr       = auth_client;
                cback_data.finish_auth_transaction.auth_transaction = auth_client->auth_transaction;
                
                auth_client->auth_transaction = NULL;

                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_CLIENTS_EVENT_FINISH_AUTH_TRANSACTION,
                                            &cback_data);
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

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
