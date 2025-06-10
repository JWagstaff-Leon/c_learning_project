#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>

#include "chat_auth.h"
#include "chat_clients.h"
#include "common_types.h"
#include "message_queue.h"

void chat_server_clients_cback(
    void*                           user_arg,
    bCHAT_CLIENTS_EVENT_TYPE        event_mask,
    const sCHAT_CLIENTS_CBACK_DATA* data)
{
    eSTATUS status;

    sCHAT_SERVER_CBLK*   master_cblk_ptr;
    sCHAT_SERVER_MESSAGE message;

    assert(NULL != user_arg);
    master_cblk_ptr = user_arg;

    if (event_mask & CHAT_CLIENTS_EVENT_START_AUTH_TRANSACTION)
    {
        message.type = CHAT_SERVER_MESSAGE_START_AUTH_TRANSACTION;

        message.params.clients.start_auth_transaction.client_ptr                 = data->start_auth_transaction.client_ptr;
        message.params.clients.start_auth_transaction.auth_transaction_container = data->start_auth_transaction.auth_transaction_container;
        message.params.clients.start_auth_transaction.credentials                = data->start_auth_transaction.credentials;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & CHAT_CLIENTS_EVENT_FINISH_AUTH_TRANSACTION)
    {
        message.type = CHAT_SERVER_MESSAGE_FINISH_AUTH_TRANSACTION;

        message.params.clients.finish_auth_transaction.auth_transaction = data->finish_auth_transaction.auth_transaction;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & CHAT_CLIENTS_EVENT_CLOSED)
    {
        message.type = CHAT_SERVER_MESSAGE_CLIENTS_CLOSED;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}


void chat_server_auth_cback(
    void*                        user_arg,
    bCHAT_AUTH_EVENT_TYPE        event_mask,
    const sCHAT_AUTH_CBACK_DATA* data)
{
    eSTATUS status;

    sCHAT_SERVER_CBLK*   master_cblk_ptr;
    sCHAT_SERVER_MESSAGE message;

    assert(NULL != user_arg);
    master_cblk_ptr = user_arg;

    if (event_mask & CHAT_AUTH_EVENT_AUTH_RESULT)
    {
        message.type = CHAT_SERVER_MESSAGE_AUTH_RESULT;

        message.params.auth.auth_result.result     = data->auth_result.result;
        message.params.auth.auth_result.user_info  = data->auth_result.user_info;
        message.params.auth.auth_result.client_ptr = data->auth_result.consumer_arg;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & CHAT_AUTH_EVENT_CLOSED)
    {
        message.type = CHAT_SERVER_MESSAGE_AUTH_CLOSED;
        
        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}


void chat_server_connection_listener_cback(
    void*                              user_arg,
    bNETWORK_WATCHER_EVENT_TYPE        event_mask,
    const sNETWORK_WATCHER_CBACK_DATA* data)
{
    eSTATUS status;

    sCHAT_SERVER_CBLK*   master_cblk_ptr;
    sCHAT_SERVER_MESSAGE message;

    assert(NULL != user_arg);
    master_cblk_ptr = user_arg;

    if (event_mask & NETWORK_WATCHER_EVENT_WATCH_COMPLETE)
    {
        message.type = CHAT_SERVER_MESSAGE_INCOMING_CONNECTION;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & NETWORK_WATCHER_EVENT_WATCH_ERROR)
    {
        message.type = CHAT_SERVER_MESSAGE_LISTENER_ERROR;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & NETWORK_WATCHER_EVENT_WATCH_CANCELLED)
    {
        message.type = CHAT_SERVER_MESSAGE_LISTENER_CANCELLED;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & NETWORK_WATCHER_EVENT_CLOSED)
    {
        message.type = CHAT_SERVER_MESSAGE_LISTENER_CLOSED;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}
