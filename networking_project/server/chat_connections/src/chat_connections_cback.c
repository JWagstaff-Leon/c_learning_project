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

    if (event & NETWORK_WATCHER_EVENT_READY)
    {
        message.type = CHAT_CONNECTIONS_MESSAGE_READ_READY;
        status = message_queue_put(master_cblk_ptr->message_queue,
                                   message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}


void chat_connections_network_watcher_write_cback(
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
    
    if (event & NETWORK_WATCHER_EVENT_READY)
    {
        message.type = CHAT_CONNECTIONS_MESSAGE_WRITE_READY;
        status = message_queue_put(master_cblk_ptr->message_queue,
                                   message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}



void chat_server_network_watcher_write_cback(
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const sNETWORK_WATCHER_CBACK_DATA* data)
{
    sCHAT_SERVER_CBLK*   master_cblk_ptr;
    sCHAT_SERVER_MESSAGE message;
    eSTATUS              status;

    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_SERVER_CBLK*)arg;

    if (event & NETWORK_WATCHER_EVENT_READY)
    {

        message.params.read_ready.index_count         = data->ready.index_count;
        message.params.read_ready.connection_indecies = generic_allocator(sizeof(data->ready.connection_indecies[0]) * data->ready.index_count);

        assert(NULL != message.params.read_ready.connection_indecies);
        
        message.type = CHAT_SERVER_MESSAGE_WRITE_READY;
        memcpy(&message.params.write_ready.connection_indecies[0],
               &data->ready.connection_indecies[0],
               sizeof(data->ready.connection_indecies[0]) * data->ready.index_count);

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}
