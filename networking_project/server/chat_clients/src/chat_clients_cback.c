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
    sCHAT_CLIENT*       client_ptr;

    sCHAT_CLIENTS_MESSAGE message;

    assert(NULL != user_arg);

    master_cblk_ptr = ((sCHAT_CLIENT*)user_arg)->master_cblk_ptr;
    assert(NULL != master_cblk_ptr);

    client_ptr = (sCHAT_CLIENT*)user_arg;
    assert(NULL != client_ptr);

    if (event_mask & CHAT_CONNECTION_EVENT_INCOMING_EVENT)
    {
        message.type                             = CHAT_CLIENTS_MESSAGE_INCOMING_EVENT;
        message.params.incoming_event.client_ptr = client_ptr;

        status = chat_event_copy(&message.params.incoming_event.event,
                                 &data->incoming_event.event);
        assert(STATUS_SUCCESS == status);

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & CHAT_CONNECTION_EVENT_CONNECTION_CLOSED)
    {
        message.type                            = CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED;
        message.params.client_closed.client_ptr = client_ptr;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}
