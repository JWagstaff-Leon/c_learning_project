#include "chat_clients.h"
#include "chat_clients_internal.h"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common_types.h"
#include "chat_connection.h"
#include "chat_event.h"
#include "shared_ptr.h"


static void chat_clients_client_destroy(
    void* client_ptr);


static void handler_no_op(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          source_client_ptr,
    const sCHAT_EVENT*  event)
{
    // Intentionally empty
    return;
}


static void handler_chat_message(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          source_client_ptr,
    const sCHAT_EVENT*  event)
{
    eSTATUS status;

    sCHAT_EVENT   outgoing_event;
    SHARED_PTR    relevant_client_ptr;
    sCHAT_CLIENT* source_client;

    source_client = SP_POINTEE_AS(source_client_ptr, sCHAT_CLIENT);
    if (CHAT_CLIENT_STATE_ACTIVE == source_client->state)
    {
        status = chat_event_copy(&outgoing_event, event);
        assert(STATUS_SUCCESS == status);

        memcpy(&outgoing_event.origin,
               &source_client->user_info,
               sizeof(outgoing_event.origin));

        for (relevant_client_ptr = master_cblk_ptr->client_list_head;
             NULL != relevant_client_ptr && NULL != SP_POINTEE(relevant_client_ptr);
             relevant_client_ptr = SP_PROPERTY(relevant_client_ptr, sCHAT_CLIENT, next))
        {
            if (relevant_client_ptr == source_client_ptr)
            {
                continue;
            }

            if (CHAT_CLIENT_STATE_ACTIVE == SP_PROPERTY(relevant_client_ptr, sCHAT_CLIENT, state))
            {
                status = chat_connection_queue_event(SP_PROPERTY(relevant_client_ptr, sCHAT_CLIENT, connection),
                                                     &outgoing_event);
            }
            assert(STATUS_SUCCESS == status);
        }
    }
}


static void handler_username_submit(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          source_client_ptr,
    const sCHAT_EVENT*  event)
{
    eSTATUS status;

    sCHAT_CLIENT*           source_client;
    sCHAT_USER_CREDENTIALS* credentials_ptr;

    sCHAT_CLIENTS_CBACK_DATA cback_data;
    char*                    new_username;
    size_t                   new_username_size;

    source_client   = SP_POINTEE_AS(source_client_ptr, sCHAT_CLIENT);
    credentials_ptr = SP_POINTEE_AS(source_client->auth_credentials_ptr, sCHAT_USER_CREDENTIALS);

    if (CHAT_CLIENT_STATE_AUTHENTICATING_PROCESSING < source_client->state)
    {
        status = chat_connection_queue_new_event(source_client->connection,
                                                 CHAT_EVENT_USERNAME_REJECTED,
                                                 k_server_info,
                                                 "User is already logged in");
        assert(STATUS_SUCCESS == status);
        return;
    }

    new_username = generic_allocator(event->length);
    if (NULL == new_username)
    {
        status = chat_connection_queue_new_event(source_client->connection,
                                                 CHAT_EVENT_SERVER_ERROR,
                                                 k_server_info,
                                                 "Server authentication process error");
        assert(STATUS_SUCCESS == status);
        return;
    }
    new_username_size = event->length;

    status = print_string_to_buffer(new_username,
                                    event->data,
                                    new_username_size,
                                    NULL);
    if (STATUS_SUCCESS != status)
    {
        status = chat_connection_queue_new_event(source_client->connection,
                                                 CHAT_EVENT_SERVER_ERROR,
                                                 k_server_info,
                                                 "Server authentication process error");
        assert(STATUS_SUCCESS == status);

        generic_deallocator(new_username);
        return;
    }

    switch (source_client->state)
    {
        case CHAT_CLIENT_STATE_AUTHENTICATING_PASSWORD:
        {
            // Free any previously entered password if going backwards in the flow
            generic_deallocator(credentials_ptr->password);

            credentials_ptr->password      = NULL;
            credentials_ptr->password_size = 0;

            // Fallthrough
        }
        case CHAT_CLIENT_STATE_AUTHENTICATING_USERNAME:
        {
            generic_deallocator(credentials_ptr->username);

            credentials_ptr->username      = new_username;
            credentials_ptr->username_size = new_username_size;

            cback_data.start_auth_transaction.auth_transaction_container = &source_client->auth_transaction;
            cback_data.start_auth_transaction.client_ptr                 = shared_ptr_share(source_client_ptr);
            cback_data.start_auth_transaction.credentials_ptr            = shared_ptr_share(source_client->auth_credentials_ptr);

            source_client->state = CHAT_CLIENT_STATE_AUTHENTICATING_PROCESSING;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENTS_EVENT_START_AUTH_TRANSACTION,
                                        &cback_data);
            break;
        }
        case CHAT_CLIENT_STATE_AUTHENTICATING_PROCESSING:
        default:
        {
            status = chat_connection_queue_new_event(source_client->connection,
                                                     CHAT_EVENT_SERVER_ERROR,
                                                     k_server_info,
                                                     "Cannot process username submission now");
        }
    }
}


static void handler_password_submit(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          source_client_ptr,
    const sCHAT_EVENT*  event)
{
    eSTATUS status;

    sCHAT_CLIENT*           source_client;
    sCHAT_USER_CREDENTIALS* credentials_ptr;

    sCHAT_CLIENTS_CBACK_DATA cback_data;
    char*                    new_password;
    size_t                   new_password_size;

    source_client   = SP_POINTEE_AS(source_client_ptr, sCHAT_CLIENT);
    credentials_ptr = SP_POINTEE_AS(source_client->auth_credentials_ptr, sCHAT_USER_CREDENTIALS);

    if (CHAT_CLIENT_STATE_AUTHENTICATING_PROCESSING < source_client->state)
    {
        status = chat_connection_queue_new_event(source_client->connection,
                                                CHAT_EVENT_SERVER_ERROR,
                                                k_server_info,
                                                "User is already logged in");
        assert(STATUS_SUCCESS == status);
        return;
    }

    new_password = generic_allocator(event->length);
    if (NULL == new_password)
    {
        status = chat_connection_queue_new_event(source_client->connection,
                                                 CHAT_EVENT_SERVER_ERROR,
                                                 k_server_info,
                                                 "Server authentication process error");
        assert(STATUS_SUCCESS == status);
        return;
    }
    new_password_size = event->length;

    status = print_string_to_buffer(new_password,
                                    event->data,
                                    new_password_size,
                                    NULL);
    if (STATUS_SUCCESS != status)
    {
        status = chat_connection_queue_new_event(source_client->connection,
                                                 CHAT_EVENT_SERVER_ERROR,
                                                 k_server_info,
                                                 "Server authentication process error");
        assert(STATUS_SUCCESS == status);

        generic_deallocator(new_password);
        return;
    }

    switch (source_client->state)
    {
        case CHAT_CLIENT_STATE_AUTHENTICATING_PASSWORD:
        {
            generic_deallocator(credentials_ptr->password);

            credentials_ptr->password      = new_password;
            credentials_ptr->password_size = new_password_size;

            cback_data.start_auth_transaction.auth_transaction_container = &source_client->auth_transaction;
            cback_data.start_auth_transaction.client_ptr                 = shared_ptr_share(source_client_ptr);
            cback_data.start_auth_transaction.credentials_ptr            = shared_ptr_share(source_client->auth_credentials_ptr);

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENTS_EVENT_START_AUTH_TRANSACTION,
                                        &cback_data);
            break;
        }
        case CHAT_CLIENT_STATE_AUTHENTICATING_PROCESSING:
        case CHAT_CLIENT_STATE_AUTHENTICATING_USERNAME:
        default:
        {
            status = chat_connection_queue_new_event(source_client->connection,
                                                     CHAT_EVENT_SERVER_ERROR,
                                                     k_server_info,
                                                     "Cannot process password submission now");
            assert(STATUS_SUCCESS == status);
        }
    }
}


static void handler_user_leave(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          source_client_ptr,
    const sCHAT_EVENT*  event)
{
    eSTATUS status;

    SP_PROPERTY(source_client_ptr, sCHAT_CLIENT, state) = CHAT_CLIENT_STATE_DISCONNECTING;

    // TODO broadcast user leave
    status = chat_connection_close(SP_PROPERTY(source_client_ptr, sCHAT_CLIENT, connection));
    assert(STATUS_SUCCESS == status);
}


typedef void (*fEVENT_HANDLER) (
    sCHAT_CLIENTS_CBLK*,
    SHARED_PTR,
    const sCHAT_EVENT*);


// This list should match up to the enum values in eCHAT_EVENT_TYPE
static const fEVENT_HANDLER event_handler_table[] = {
    handler_no_op,            // CHAT_EVENT_UNDEFINED
    handler_chat_message,     // CHAT_EVENT_CHAT_MESSAGE
    handler_no_op,            // CHAT_EVENT_CONNECTION_FAILED
    handler_no_op,            // CHAT_EVENT_SERVER_ERROR
    handler_no_op,            // CHAT_EVENT_OVERSIZED_CONTENT
    handler_no_op,            // CHAT_EVENT_USERNAME_REQUEST
    handler_username_submit,  // CHAT_EVENT_USERNAME_SUBMIT
    handler_no_op,            // CHAT_EVENT_USERNAME_REJECTED
    handler_no_op,            // CHAT_EVENT_PASSWORD_REQUEST
    handler_password_submit,  // CHAT_EVENT_PASSWORD_SUBMIT
    handler_no_op,            // CHAT_EVENT_PASSWORD_REJECTED
    handler_no_op,            // CHAT_EVENT_AUTHENTICATED
    handler_no_op,            // CHAT_EVENT_USER_JOIN
    handler_user_leave,       // CHAT_EVENT_USER_LEAVE
    handler_no_op,            // CHAT_EVENT_SERVER_SHUTDOWN
    handler_no_op,            // CHAT_EVENT_MAX
};


void chat_clients_process_event(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          source_client_ptr,
    const sCHAT_EVENT*  event)
{
    event_handler_table[event->type](master_cblk_ptr,
                                     source_client_ptr,
                                     event);
}


static void chat_clients_cleanup_credentials(
    void* credentials_ptr)
{
    sCHAT_USER_CREDENTIALS* credentials = (sCHAT_USER_CREDENTIALS*)credentials_ptr;
    memset(credentials->username,
           0,
           credentials->username_size);
    memset(credentials->password,
           0,
           credentials->password_size);

    generic_deallocator(credentials->username);
    generic_deallocator(credentials->password);

    credentials->username = NULL;
    credentials->password = NULL;
}


eSTATUS chat_clients_introduce_user(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          client_ptr)
{
    eSTATUS status;

    SHARED_PTR  relevant_client_ptr;
    sCHAT_EVENT outgoing_event;

    status = chat_event_populate(&outgoing_event,
                                 CHAT_EVENT_USER_JOIN,
                                 k_server_info,
                                 SP_PROPERTY(client_ptr, sCHAT_CLIENT, user_info.name));
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    for (relevant_client_ptr = master_cblk_ptr->client_list_head;
         NULL != relevant_client_ptr && NULL != SP_POINTEE(relevant_client_ptr);
         relevant_client_ptr = SP_PROPERTY(relevant_client_ptr, sCHAT_CLIENT, next))
    {
        if (relevant_client_ptr == relevant_client_ptr)
        {
            continue;
        }

        if (CHAT_CLIENT_STATE_ACTIVE == SP_PROPERTY(relevant_client_ptr, sCHAT_CLIENT, state))
        {
            status = chat_connection_queue_event(SP_PROPERTY(relevant_client_ptr, sCHAT_CLIENT, connection),
                                                 &outgoing_event);
        }
        assert(STATUS_SUCCESS == status);
    }
}


eSTATUS chat_clients_client_create(
    SHARED_PTR*         out_new_client_ptr,
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    int                 fd)
{
    eSTATUS       status;
    SHARED_PTR    new_client_ptr;
    sCHAT_CLIENT* new_client;

    new_client_ptr = shared_ptr_create(sizeof(sCHAT_CLIENT),
                                       chat_clients_client_destroy);
    if (NULL == new_client_ptr)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_client;
    }
    new_client = SP_POINTEE_AS(new_client_ptr, sCHAT_CLIENT);

    memset(new_client, 0, sizeof(sCHAT_CLIENT));
    new_client->master_cblk_ptr = master_cblk_ptr;

    new_client->auth_credentials_ptr = shared_ptr_create(sizeof(sCHAT_USER_CREDENTIALS),
                                                         chat_clients_cleanup_credentials);
    if (NULL == new_client->auth_credentials_ptr)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_credentials;
    }
    memset(SP_POINTEE(new_client->auth_credentials_ptr), 0, sizeof(sCHAT_USER_CREDENTIALS));

    status = chat_connection_create(&new_client->connection,
                                    chat_clients_connection_cback,
                                    shared_ptr_share(new_client_ptr),
                                    fd);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_connection;
    }

    new_client->state = CHAT_CLIENT_STATE_INIT;

    *out_new_client_ptr = new_client_ptr;
    return STATUS_SUCCESS;

fail_create_connection:
    shared_ptr_release(new_client_ptr); // Release the reference shared with the connection
    shared_ptr_release(new_client->auth_credentials_ptr);

fail_alloc_credentials:
    shared_ptr_release(new_client_ptr);

fail_alloc_client:
    return status;
}


eSTATUS chat_clients_client_close(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          client_ptr)
{
    sCHAT_CLIENT* client = SP_POINTEE_AS(client_ptr, sCHAT_CLIENT);

    if (NULL != client->prev && NULL != SP_POINTEE(client->prev))
    {
        SP_PROPERTY(client->prev, sCHAT_CLIENT, next) = shared_ptr_share(client->next);
    }
    if (NULL != client->next && NULL != SP_POINTEE(client->next))
    {
        SP_PROPERTY(client->next, sCHAT_CLIENT, prev) = shared_ptr_share(client->prev);
    }
    if (SP_POINTEE(master_cblk_ptr->client_list_head) == client)
    {
        shared_ptr_release(master_cblk_ptr->client_list_head);
        master_cblk_ptr->client_list_head = client->next;
        client->next = NULL;
    }
    if (SP_POINTEE(master_cblk_ptr->client_list_tail) == client)
    {
        shared_ptr_release(master_cblk_ptr->client_list_tail);
        master_cblk_ptr->client_list_tail = client->prev;
        client->prev = NULL;
    }

    shared_ptr_release(client->prev);
    shared_ptr_release(client->next);
    
    shared_ptr_release(client_ptr);
    return STATUS_SUCCESS;
}


static void chat_clients_client_destroy(
    void* client_ptr)
{
    sCHAT_CLIENT*            client;
    sCHAT_CLIENTS_CBLK*      master_cblk_ptr;
    sCHAT_CLIENTS_CBACK_DATA cback_data;

    client = (sCHAT_CLIENT*)client_ptr;

    shared_ptr_release(client->prev);
    shared_ptr_release(client->next);

    if (CHAT_CLIENT_STATE_AUTHENTICATING_PROCESSING == client->state)
    {
        cback_data.finish_auth_transaction.auth_transaction = client->auth_transaction;

        master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                    CHAT_CLIENTS_EVENT_FINISH_AUTH_TRANSACTION,
                                    &cback_data);
    }

    shared_ptr_release(client->auth_credentials_ptr);
}
