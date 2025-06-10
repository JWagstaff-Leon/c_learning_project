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


static char k_server_name[] = "Server";


static void handler_no_op(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event)
{
    // Intentionally empty
    return;
}


static void handler_chat_message(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event)
{
    eSTATUS status;

    sCHAT_EVENT   outgoing_event;
    sCHAT_CLIENT* relevant_client;
    uint32_t      client_index;

    if (CHAT_CLIENT_STATE_ACTIVE == source_client->state)
    {
        status = chat_event_copy(&outgoing_event, event);
        assert(STATUS_SUCCESS == status);

        outgoing_event.origin = source_client->user_info.id;

        for (client_index = 1;
             client_index < master_cblk_ptr->max_clients;
             client_index++)
        {
            relevant_client = master_cblk_ptr->client_list[client_index];

            if (relevant_client == source_client)
            {
                continue;
            }

            if (NULL != relevant_client && CHAT_CLIENT_STATE_ACTIVE == relevant_client->state)
            {
                status = chat_connection_queue_event(relevant_client->connection,
                                                     &outgoing_event);
            }
            assert(STATUS_SUCCESS == status);
        }
    }
}


static void handler_username_request(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event)
{
    eSTATUS     status;
    sCHAT_EVENT outgoing_event;

    if (CHAT_CLIENT_STATE_ACTIVE == source_client->state)
    {
        status = chat_event_populate(&outgoing_event,
                                     CHAT_EVENT_USERNAME_SUBMIT,
                                     CHAT_USER_ID_SERVER,
                                     k_server_name);
        assert(STATUS_SUCCESS == status);

        status = chat_connection_queue_event(source_client->connection,
                                             &outgoing_event);
        assert(STATUS_SUCCESS == status);
    }
}


static void handler_username_submit(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event)
{
    eSTATUS status;

    sCHAT_CLIENT* relevant_client;
    sCHAT_EVENT   outgoing_event;

    sCHAT_CLIENTS_CBACK_DATA cback_data;
    char*                    new_username;
    size_t                   new_username_size;

    if (CHAT_CLIENT_STATE_AUTHENTICATING_PASSWORD < source_client->state)
    {
        status = chat_event_populate(&outgoing_event,
                                     CHAT_EVENT_USERNAME_REJECTED,
                                     CHAT_USER_ID_SERVER,
                                     "Username is already set");
        assert(STATUS_SUCCESS == status);

        status = chat_connection_queue_event(source_client->connection,
                                             &outgoing_event);
        assert(STATUS_SUCCESS == status);
        return;
    }

    if (event->length > CHAT_USER_MAX_NAME_SIZE)
    {
        status = chat_event_populate(&outgoing_event,
                                     CHAT_EVENT_USERNAME_REJECTED,
                                     CHAT_USER_ID_SERVER,
                                     "User is already logged in");
        assert(STATUS_SUCCESS == status);

        status = chat_connection_queue_event(source_client->connection,
                                             &outgoing_event);
        assert(STATUS_SUCCESS == status);
        return;
    }

    new_username = generic_allocator(event->length);
    if (NULL == new_username)
    {
        status = chat_event_populate(&outgoing_event,
                                     CHAT_EVENT_SERVER_ERROR,
                                     CHAT_USER_ID_SERVER,
                                     "Server authentication process error");
        assert(STATUS_SUCCESS == status);

        status = chat_connection_queue_event(source_client->connection,
                                             &outgoing_event);
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
        status = chat_event_populate(&outgoing_event,
                                     CHAT_EVENT_SERVER_ERROR,
                                     CHAT_USER_ID_SERVER,
                                     "Server authentication process error");
        assert(STATUS_SUCCESS == status);

        status = chat_connection_queue_event(source_client->connection,
                                             &outgoing_event);
        assert(STATUS_SUCCESS == status);

        generic_deallocator(new_username);
        return;
    }

    switch (source_client->state)
    {
        case CHAT_CLIENT_STATE_AUTHENTICATING_PASSWORD:
        {
            // Free any previously entered password if going backwards in the flow
            generic_deallocator(source_client->auth_credentials->password);

            source_client->auth_credentials->password      = NULL;
            source_client->auth_credentials->password_size = 0;

            // Fallthrough
        }
        case CHAT_CLIENT_STATE_AUTHENTICATING_USERNAME:
        {
            generic_deallocator(source_client->auth_credentials->username);

            source_client->auth_credentials->username      = new_username;
            source_client->auth_credentials->username_size = new_username_size;

            cback_data.start_auth_transaction.auth_transaction_container = &source_client->auth_transaction;
            cback_data.start_auth_transaction.client_ptr                 = source_client;
            cback_data.start_auth_transaction.credentials                = *source_client->auth_credentials;

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENTS_EVENT_START_AUTH_TRANSACTION,
                                        &cback_data);
            break;
        }
        default:
        {
            status = chat_event_populate(&outgoing_event,
                                         CHAT_EVENT_SERVER_ERROR,
                                         CHAT_USER_ID_SERVER,
                                         "Cannot process username submission now");
            assert(STATUS_SUCCESS == status);

            status = chat_connection_queue_event(source_client->connection,
                                                 &outgoing_event);
            assert(STATUS_SUCCESS == status);
        }
    }
}


static void handler_password_submit(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event)
{
    eSTATUS status;

    sCHAT_CLIENT* relevant_client;
    sCHAT_EVENT   outgoing_event;

    sCHAT_CLIENTS_CBACK_DATA cback_data;
    char*                    new_password;
    size_t                   new_password_size;

    if (CHAT_CLIENT_STATE_AUTHENTICATING_PASSWORD < source_client->state)
    {
        status = chat_event_populate(&outgoing_event,
                                     CHAT_EVENT_SERVER_ERROR,
                                     CHAT_USER_ID_SERVER,
                                     "User is already logged in");
        assert(STATUS_SUCCESS == status);

        status = chat_connection_queue_event(source_client->connection,
                                             &outgoing_event);
        assert(STATUS_SUCCESS == status);
        return;
    }

    new_password = generic_allocator(event->length);
    if (NULL == new_password)
    {
        status = chat_event_populate(&outgoing_event,
                                     CHAT_EVENT_SERVER_ERROR,
                                     CHAT_USER_ID_SERVER,
                                     "Server authentication process error");
        assert(STATUS_SUCCESS == status);

        status = chat_connection_queue_event(source_client->connection,
                                             &outgoing_event);
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
        status = chat_event_populate(&outgoing_event,
                                     CHAT_EVENT_SERVER_ERROR,
                                     CHAT_USER_ID_SERVER,
                                     "Server authentication process error");
        assert(STATUS_SUCCESS == status);

        status = chat_connection_queue_event(source_client->connection,
                                             &outgoing_event);
        assert(STATUS_SUCCESS == status);

        generic_deallocator(new_password);
        return;
    }

    switch (source_client->state)
    {
        case CHAT_CLIENT_STATE_AUTHENTICATING_PASSWORD:
        {
            generic_deallocator(source_client->auth_credentials->password);

            source_client->auth_credentials->password      = new_password;
            source_client->auth_credentials->password_size = new_password_size;

            cback_data.start_auth_transaction.auth_transaction_container = &source_client->auth_transaction;
            cback_data.start_auth_transaction.client_ptr                 = source_client;
            cback_data.start_auth_transaction.credentials                = *source_client->auth_credentials;

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENTS_EVENT_START_AUTH_TRANSACTION,
                                        &cback_data);
            break;
        }
        case CHAT_CLIENT_STATE_AUTHENTICATING_USERNAME:
        default:
        {
            status = chat_event_populate(&outgoing_event,
                                         CHAT_EVENT_SERVER_ERROR,
                                         CHAT_USER_ID_SERVER,
                                         "Cannot process password submission now");
            assert(STATUS_SUCCESS == status);

            status = chat_connection_queue_event(source_client->connection,
                                                 &outgoing_event);
            assert(STATUS_SUCCESS == status);
        }
    }
}


static void handler_user_list(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event)
{
    // REVIEW implement this?
}


static void handler_user_leave(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event)
{
    eSTATUS status;

    source_client->state = CHAT_CLIENT_STATE_DISCONNECTING;

    status = chat_connection_close(source_client->connection);
    assert(STATUS_SUCCESS == status);
}


typedef void (*fEVENT_HANDLER) (
    sCHAT_CLIENTS_CBLK*,
    sCHAT_CLIENT*,
    const sCHAT_EVENT*);


// This list should match up to the enum values in eCHAT_EVENT_TYPE
static const fEVENT_HANDLER event_handler_table[] = {
    handler_no_op,            // CHAT_EVENT_UNDEFINED
    handler_chat_message,     // CHAT_EVENT_CHAT_MESSAGE
    handler_no_op,            // CHAT_EVENT_CONNECTION_FAILED
    handler_no_op,            // CHAT_EVENT_SERVER_ERROR
    handler_no_op,            // CHAT_EVENT_OVERSIZED_CONTENT
    handler_username_request, // CHAT_EVENT_USERNAME_REQUEST
    handler_username_submit,  // CHAT_EVENT_USERNAME_SUBMIT
    handler_no_op,            // CHAT_EVENT_USERNAME_REJECTED
    handler_no_op,            // CHAT_EVENT_PASSWORD_REQUEST
    handler_password_submit,  // CHAT_EVENT_PASSWORD_SUBMIT
    handler_no_op,            // CHAT_EVENT_PASSWORD_REJECTED
    handler_no_op,            // CHAT_EVENT_AUTHENTICATED
    handler_user_list,        // CHAT_EVENT_USER_LIST
    handler_no_op,            // CHAT_EVENT_USER_JOIN
    handler_user_leave,       // CHAT_EVENT_USER_LEAVE
    handler_no_op,            // CHAT_EVENT_SERVER_SHUTDOWN
    handler_no_op,            // CHAT_EVENT_MAX
};


void chat_clients_process_event(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event)
{
    event_handler_table[event->type](master_cblk_ptr,
                                     source_client,
                                     event);
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

    new_client->auth_credentials = generic_allocator(sizeof(sCHAT_USER_CREDENTIALS));
    if (NULL == new_client->auth_credentials)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_credentials;
    }
    memset(new_client->auth_credentials, 0, sizeof(sCHAT_USER_CREDENTIALS));

    status = chat_connection_create(&new_client->connection,
                                    chat_clients_connection_cback,
                                    new_client,
                                    fd);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_connection;
    }

    new_client->state            = CHAT_CLIENT_STATE_INIT;
    new_client->master_cblk_ptr  = master_cblk_ptr;
    new_client->client_container = client_container_ptr;

    *client_container_ptr = new_client;
    return STATUS_SUCCESS;

fail_create_connection:
    generic_deallocator(new_client->auth_credentials);

fail_alloc_credentials:
    generic_deallocator(new_client);

fail_alloc_client:
    return status;
}


eSTATUS realloc_clients(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    uint32_t            new_max_clients)
{
    eSTATUS status;

    uint32_t       client_index;
    sCHAT_CLIENT** new_client_list;

    new_client_list = generic_allocator(sizeof(sCHAT_CLIENT*) * new_max_clients);
    if (NULL == new_client_list)
    {
        return STATUS_ALLOC_FAILED;
    }

    // Close any clients in excess of new_max_clients
    for (client_index = new_max_clients;
         client_index < master_cblk_ptr->client_count;
         client_index++)
    {
        chat_connection_destroy(master_cblk_ptr->client_list[client_index]->connection);
        chat_clients_client_close(master_cblk_ptr->client_list[client_index]);
    }

    // Copy any clients overlapping
    for (client_index = 0;
         client_index < master_cblk_ptr->max_clients && client_index < new_max_clients;
         client_index++)
    {
        new_client_list[client_index] = master_cblk_ptr->client_list[client_index];
        new_client_list[client_index]->client_container = &new_client_list[client_index];
    }

    // Initialize any new clients
    for (client_index = master_cblk_ptr->max_clients;
         client_index < new_max_clients;
         client_index++)
    {
        new_client_list[client_index] = NULL;
    }

    generic_deallocator(master_cblk_ptr->client_list);

    master_cblk_ptr->client_list = new_client_list;
    master_cblk_ptr->max_clients = new_max_clients;

    return STATUS_SUCCESS;
}
