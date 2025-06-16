#include "chat_client.h"
#include "chat_client_internal.h"

#include "chat_connection.h"


void chat_client_connection_cback(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const sCHAT_CONNECTION_CBACK_DATA* data)
{
    if (event_mask & CHAT_CONNECTION_EVENT_INCOMING_EVENT)
    {
        // TODO
    }

    if (event_mask & CHAT_CONNECTION_EVENT_CLOSED)
    {
        // TODO
    }
}
