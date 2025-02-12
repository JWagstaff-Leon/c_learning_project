#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>

#include "chat_connections.h"
#include "common_types.h"


void chat_server_connections_cback(
    void*                               user_arg,
    eCHAT_CONNECTIONS_EVENT_TYPE        event_mask,
    const uCHAT_CONNECTIONS_CBACK_DATA* data)
{
    sCHAT_SERVER_CBLK*   master_cblk_ptr;
    sCHAT_SERVER_MESSAGE message;

    assert(NULL != user_arg);
    master_cblk_ptr = user_arg;

    

    if (event_mask & CHAT_CONNECTIONS_EVENT_CLOSED)
    {
        // TODO handle close
    }
}
