#include "chat_clients.h"
#include "chat_clients_internal.h"

#include <assert.h>


void chat_clients_new_connections_cback(
    void*                              user_arg,
    bNETWORK_WATCHER_EVENT_TYPE        event_mask,
    const sNETWORK_WATCHER_CBACK_DATA* data)
{
    sCHAT_CLIENTS_CBLK*   master_cblk_ptr;
    sCHAT_CLIENTS_MESSAGE message;
    eSTATUS               status;

    assert(NULL != user_arg);
    master_cblk_ptr = (sCHAT_CLIENTS_USER_ARG*)user_arg;


    if (event_mask & NETWORK_WATCHER_EVENT_WATCH_COMPLETE)
    {
        message.type = CHAT_CLIENTS_MESSAGE_NEW_CONNECTION;
        status       = message_queue_put(master_cblk_ptr->message_queue,
                                         &message,
                                         sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & NETWORK_WATCHER_EVENT_WATCH_ERROR)
    {
        message.type = CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_ERROR;
        status       = message_queue_put(master_cblk_ptr->message_queue,
                                         &message,
                                         sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & NETWORK_WATCHER_EVENT_WATCH_CANCELLED)
    {
        message.type = CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_CANCELLED;
        status       = message_queue_put(master_cblk_ptr->message_queue,
                                         &message,
                                         sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & NETWORK_WATCHER_EVENT_CLOSED)
    {
        message.type = CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_CLOSED;
        status       = message_queue_put(master_cblk_ptr->message_queue,
                                         &message,
                                         sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}


void chat_clients_connection_cback(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const uCHAT_CONNECTION_CBACK_DATA* data)
{
    eSTATUS status;

    sCHAT_CLIENTS_CBLK* master_cblk_ptr;
    sCHAT_CLIENT*       client_ptr;

    sCHAT_CLIENTS_MESSAGE message;

    assert(NULL != user_arg);

    master_cblk_ptr = ((sCHAT_CLIENTS_CLIENT_CBACK_ARG*)user_arg)->master_cblk_ptr;
    assert(NULL != master_cblk_ptr);

    client_ptr = ((sCHAT_CLIENTS_CLIENT_CBACK_ARG*)user_arg)->client_ptr;
    assert(NULL != client_ptr);

    if (event_mask & CHAT_CONNECTION_EVENT_INCOMING_EVENT)
    {
        message.type                             = CHAT_CLIENTS_MESSAGE_INCOMING_EVENT;
        message.params.incoming_event.client_ptr = client_ptr;

        status = chat_event_copy(&message.params.incoming_event.event,
                                 &data->incoming_event.event);
        assert(STATUS_SUCCESS == status);

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & CHAT_CONNECTION_EVENT_CONNECTION_CLOSED)
    {
        message.type                            = CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED;
        message.params.client_closed.client_ptr = client_ptr;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}
