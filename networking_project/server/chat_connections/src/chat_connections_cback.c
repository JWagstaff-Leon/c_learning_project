#include "chat_connections.h"
#include "chat_connections_internal.h"


void chat_connections_network_watcher_read_cback(
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const sNETWORK_WATCHER_CBACK_DATA* data)
{
    sCHAT_CONNECTIONS_CBLK*   master_cblk_ptr;
    sCHAT_CONNECTIONS_MESSAGE message;
    eSTATUS                   status;

    (void) data;

    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_CONNECTIONS_CBLK*)arg;

    if (event & NETWORK_WATCHER_EVENT_WATCH_COMPLETE)
    {
        message.type = CHAT_CONNECTIONS_MESSAGE_READ_READY;
        status       = message_queue_put(master_cblk_ptr->message_queue,
                                         message,
                                         sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}


void chat_connections_network_watcher_write_cback(
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const uNETWORK_WATCHER_CBACK_DATA* data)
{
    sCHAT_CONNECTIONS_CBLK*   master_cblk_ptr;
    sCHAT_CONNECTIONS_MESSAGE message;
    eSTATUS                   status;

    (void) data;

    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_CONNECTIONS_CBLK*)arg;

    if (event & NETWORK_WATCHER_EVENT_WATCH_COMPLETE)
    {
        message.type = CHAT_CONNECTIONS_MESSAGE_WRITE_READY;
        status       = message_queue_put(master_cblk_ptr->message_queue,
                                         message,
                                         sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}
