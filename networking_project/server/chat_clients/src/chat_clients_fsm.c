#include "chat_clients.h"
#include "chat_clients_internal.h"
#include "chat_clients_fsm.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "chat_connection.h"
#include "chat_event.h"
#include "common_types.h"
#include "message_queue.h"
#include "shared_ptr.h"


static const char* k_auth_strings[] = {
    "Enter your username",  // CHAT_CLIENTS_AUTH_STEP_USERNAME_REQUIRED
    "Invalid username",     // CHAT_CLIENTS_AUTH_STEP_USERNAME_REJECTED
    "Create your password", // CHAT_CLIENTS_AUTH_STEP_PASSWORD_CREATION
    "Enter your password",  // CHAT_CLIENTS_AUTH_STEP_PASSWORD_REQUIRED
    "Invalid password",     // CHAT_CLIENTS_AUTH_STEP_PASSWORD_REJECTED
    "Logged in",            // CHAT_CLIENTS_AUTH_STEP_AUTHENTICATED
    "Internal error"        // CHAT_CLIENTS_AUTH_STEP_CLOSED
};


static void open_processing(
    sCHAT_CLIENTS_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENTS_MESSAGE* message)
{
    eSTATUS status;
    int     new_connection_fd;

    uint32_t      client_index;
    SHARED_PTR    relevant_client_ptr;
    sCHAT_CLIENT* relevant_client;
    
    sCHAT_CLIENTS_CBACK_DATA cback_data;
    
    eCHAT_CLIENTS_AUTH_STEP auth_step;
    SHARED_PTR              outgoing_client_ptr;
    sCHAT_EVENT             outgoing_event;

    switch (message->type)
    {
        case CHAT_CLIENTS_MESSAGE_OPEN_CLIENT:
        {
            new_connection_fd = message->params.open_client.fd;

            status = chat_clients_client_create(&relevant_client_ptr,
                                                master_cblk_ptr,
                                                new_connection_fd);
            if (STATUS_SUCCESS != status)
            {
                close(new_connection_fd);
                break;
            }
            relevant_client = SP_POINTEE_AS(relevant_client_ptr, sCHAT_CLIENT);

            if (NULL == master_cblk_ptr->client_list_head)
            {
                assert(NULL == master_cblk_ptr->client_list_tail);

                master_cblk_ptr->client_list_head = shared_ptr_share(relevant_client_ptr);
                master_cblk_ptr->client_list_tail = shared_ptr_share(relevant_client_ptr);
            }
            else
            {
                // REVIEW master_cblk_ptr->client_list_tail->next should be NULL; should this be left in for sanity?
                shared_ptr_release(SP_PROPERTY(master_cblk_ptr->client_list_tail, sCHAT_CLIENT, next));

                SP_PROPERTY(master_cblk_ptr->client_list_tail, sCHAT_CLIENT, next) = shared_ptr_share(relevant_client_ptr);
                relevant_client->prev                                              = master_cblk_ptr->client_list_tail;
                master_cblk_ptr->client_list_tail                                  = shared_ptr_share(relevant_client_ptr);
            }

            SP_PROPERTY(relevant_client_ptr, sCHAT_CLIENT, state) = CHAT_CLIENT_STATE_AUTHENTICATING_INIT;

            cback_data.start_auth_transaction.client_ptr                 = shared_ptr_share(relevant_client_ptr);
            cback_data.start_auth_transaction.auth_transaction_container = &relevant_client->auth_transaction;
            cback_data.start_auth_transaction.credentials_ptr            = shared_ptr_share(relevant_client->auth_credentials_ptr);

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENTS_EVENT_START_AUTH_TRANSACTION,
                                        &cback_data);

            shared_ptr_release(relevant_client_ptr); // Release the ownership this function has on the pointer
            break;
        }
        case CHAT_CLIENTS_MESSAGE_AUTH_EVENT:
        {
            auth_step           = message->params.auth_event.auth_step;
            relevant_client_ptr = message->params.auth_event.client_ptr;

            if (NULL == SP_POINTEE(relevant_client_ptr))
            {
                shared_ptr_release(relevant_client_ptr);
                break;
            }
            relevant_client = (SP_POINTEE_AS(relevant_client_ptr, sCHAT_CLIENT));

            switch (auth_step)
            {
                case CHAT_CLIENTS_AUTH_STEP_USERNAME_REQUIRED:
                case CHAT_CLIENTS_AUTH_STEP_USERNAME_REJECTED:
                {
                    relevant_client->state = CHAT_CLIENT_STATE_AUTHENTICATING_USERNAME;

                    status = chat_connection_queue_new_event(relevant_client->connection,
                                                             CHAT_EVENT_USERNAME_REQUEST,
                                                             k_server_info,
                                                             k_auth_strings[auth_step]);
                    assert(STATUS_SUCCESS == status);
                    break;
                }
                case CHAT_CLIENTS_AUTH_STEP_PASSWORD_CREATION:
                case CHAT_CLIENTS_AUTH_STEP_PASSWORD_REQUIRED:
                case CHAT_CLIENTS_AUTH_STEP_PASSWORD_REJECTED:
                {
                    relevant_client->state = CHAT_CLIENT_STATE_AUTHENTICATING_PASSWORD;

                    status = chat_connection_queue_new_event(relevant_client->connection,
                                                             CHAT_EVENT_PASSWORD_REQUEST,
                                                             k_server_info,
                                                             k_auth_strings[auth_step]);
                    assert(STATUS_SUCCESS == status);
                    break;
                }
                case CHAT_CLIENTS_AUTH_STEP_AUTHENTICATED:
                {
                    relevant_client->state = CHAT_CLIENT_STATE_ACTIVE;

                    relevant_client->user_info.id = message->params.auth_event.user_info.id;
                    
                    status = print_string_to_buffer(relevant_client->user_info.name,
                                                    message->params.auth_event.user_info.name,
                                                    sizeof(relevant_client->user_info.name),
                                                    NULL);
                    assert(STATUS_SUCCESS == status);

                    status = chat_connection_queue_new_event(relevant_client->connection,
                                                             CHAT_EVENT_AUTHENTICATED,
                                                             k_server_info,
                                                             k_auth_strings[auth_step]);
                    assert(STATUS_SUCCESS == status);

                    shared_ptr_release(relevant_client->auth_credentials_ptr);
                    relevant_client->auth_credentials_ptr = NULL;

                    chat_clients_introduce_user(master_cblk_ptr,
                                                relevant_client_ptr);
                    
                    break;
                }
                case CHAT_CLIENTS_AUTH_STEP_CLOSED:
                {
                    // REVIEW should this close the client?
                    break;
                }

                cback_data.finish_auth_transaction.auth_transaction = relevant_client->auth_transaction;
                relevant_client->auth_transaction                   = NULL;

                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_CLIENTS_EVENT_FINISH_AUTH_TRANSACTION,
                                            &cback_data);
            }

            shared_ptr_release(relevant_client_ptr);
            break;
        }
        case CHAT_CLIENTS_MESSAGE_INCOMING_EVENT:
        {
            chat_clients_process_event(master_cblk_ptr,
                                       message->params.incoming_event.client_ptr,
                                       &message->params.incoming_event.event);
            shared_ptr_release(message->params.incoming_event.client_ptr);
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED:
        {
            status =chat_clients_client_close(master_cblk_ptr,
                                              message->params.client_closed.client_ptr);
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLOSE:
        {
            for (relevant_client_ptr = master_cblk_ptr->client_list_head;
                NULL != relevant_client_ptr && NULL != SP_POINTEE(relevant_client_ptr);
                relevant_client_ptr = SP_PROPERTY(relevant_client_ptr, sCHAT_CLIENT, next))
            {
                status = chat_connection_close(SP_PROPERTY(relevant_client_ptr, sCHAT_CLIENT, connection));
                assert(STATUS_SUCCESS == status);
            }
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

    SHARED_PTR relevant_client_ptr;

    switch (message->type)
    {
        case CHAT_CLIENTS_MESSAGE_INCOMING_EVENT:
        {
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED:
        {
            status = chat_clients_client_close(master_cblk_ptr,
                                               message->params.client_closed.client_ptr);
            assert(STATUS_SUCCESS == status);

            if (NULL == master_cblk_ptr->client_list_head)
            {
                assert(NULL == master_cblk_ptr->client_list_tail);
                master_cblk_ptr->state = CHAT_CLIENTS_STATE_CLOSED;
            }
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
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}


static void fsm_cblk_close(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    eSTATUS status;

    fCHAT_CLIENTS_USER_CBACK user_cback;
    void*                    user_arg;

    assert(NULL != master_cblk_ptr);

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    generic_deallocator(master_cblk_ptr);

    user_cback(user_arg,
               CHAT_CLIENTS_EVENT_CLOSED,
               NULL);
}


void* chat_clients_thread_entry(
    void* arg)
{
    sCHAT_CLIENTS_CBLK* master_cblk_ptr;
    eSTATUS                 status;

    sCHAT_CLIENTS_MESSAGE message;

    assert(NULL != arg);
    master_cblk_ptr = arg;

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
