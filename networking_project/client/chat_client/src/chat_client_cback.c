#include "chat_client.h"
#include "chat_client_internal.h"
#include "chat_client_fsm.h"

#include <assert.h>

#include "chat_connection.h"
#include "chat_event.h"
#include "common_types.h"


void chat_client_connection_cback(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const sCHAT_CONNECTION_CBACK_DATA* data)
{
    sCHAT_CLIENT_CBLK*   master_cblk_ptr;
    sCHAT_CLIENT_MESSAGE message;

    eSTATUS status;

    master_cblk_ptr = (sCHAT_CLIENT_CBLK*)user_arg;

    if (event_mask & CHAT_CONNECTION_EVENT_INCOMING_EVENT)
    {
        message.type = CHAT_CLIENT_MESSAGE_INCOMING_EVENT;

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
        message.type = CHAT_CLIENT_MESSAGE_CONNECTION_CLOSED;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}
