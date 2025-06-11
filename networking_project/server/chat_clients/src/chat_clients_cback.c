#include "chat_clients.h"
#include "chat_clients_internal.h"
#include "chat_clients_fsm.h"

#include <assert.h>

#include "chat_event.h"
#include "common_types.h"
#include "message_queue.h"


void chat_clients_connection_cback(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const sCHAT_CONNECTION_CBACK_DATA* data)
{
    eSTATUS status;

    sCHAT_CLIENTS_CBLK* master_cblk_ptr;
    sCHAT_CLIENT_ENTRY* client_entry;

    sCHAT_CLIENTS_MESSAGE message;

    assert(NULL != user_arg);

    master_cblk_ptr = ((sCHAT_CLIENT_ENTRY*)user_arg)->master_cblk_ptr;
    assert(NULL != master_cblk_ptr);

    client_entry = (sCHAT_CLIENT_ENTRY*)user_arg;
    assert(NULL != client_entry);

    if (event_mask & CHAT_CONNECTION_EVENT_INCOMING_EVENT)
    {
        message.type                               = CHAT_CLIENTS_MESSAGE_INCOMING_EVENT;
        message.params.incoming_event.client_entry = client_entry;

        status = chat_event_copy(&message.params.incoming_event.event,
                                 &data->incoming_event.event);
        assert(STATUS_SUCCESS == status);

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & CHAT_CONNECTION_EVENT_CLOSED)
    {
        message.type                              = CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED;
        message.params.client_closed.client_entry = client_entry;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}
