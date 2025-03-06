#include "chat_connection.h"
#include "chat_connection_internal.h"
#include "chat_connection_fsm.h"

#include <assert.h>

#include "message_queue.h"
#include "network_watcher.h"


void chat_connection_watcher_cback(
    void*                              arg,
    bNETWORK_WATCHER_EVENT_TYPE        event_mask,
    const sNETWORK_WATCHER_CBACK_DATA* data)
{
    sCHAT_CONNECTION_CBLK*   master_cblk_ptr;
    sCHAT_CONNECTION_MESSAGE message;
    eSTATUS                  status;
    
    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_CONNECTION_CBLK*)arg;
    
    if (event_mask & NETWORK_WATCHER_EVENT_WATCH_COMPLETE)
    {
        message.type = CHAT_CONNECTION_MESSAGE_TYPE_WATCH_COMPLETE;
        
        assert(NULL != data);
        message.params.watch_complete.revents = data->watch_complete.revents;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & NETWORK_WATCHER_EVENT_WATCH_CANCELLED)
    {
        message.type = CHAT_CONNECTION_MESSAGE_TYPE_WATCH_CANCELLED;
        
        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}
