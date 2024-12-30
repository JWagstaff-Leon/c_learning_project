#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <string.h>

#include "common_types.h"
#include "network_watcher.h"


void chat_server_network_watcher_read_cback(
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

        message.params.read_ready.index_count = data->ready.index_count;
        message.params.read_ready.connection_indecies = generic_allocator(sizeof(data->ready.connection_indecies[0]) * data->ready.index_count);

        if (NULL != message.params.read_ready.connection_indecies)
        {
            message.type = CHAT_SERVER_MESSAGE_READ_READY;
            memcpy(&message.params.read_ready.connection_indecies[0],
                   &data->ready.connection_indecies[0],
                   sizeof(data->ready.connection_indecies[0]) * data->ready.index_count);
        }
        else
        {
            message.type = CHAT_SERVER_MESSAGE_READ_READY_ALLOC_FAILED;
        }

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

        message.params.read_ready.index_count = data->ready.index_count;
        message.params.read_ready.connection_indecies = generic_allocator(sizeof(data->ready.connection_indecies[0]) * data->ready.index_count);

        if (NULL != message.params.read_ready.connection_indecies)
        {
            message.type = CHAT_SERVER_MESSAGE_WRITE_READY;
            memcpy(&message.params.write_ready.connection_indecies[0],
                   &data->ready.connection_indecies[0],
                   sizeof(data->ready.connection_indecies[0]) * data->ready.index_count);
        }
        else
        {
            message.type = CHAT_SERVER_MESSAGE_WRITE_READY_ALLOC_FAILED;
        }

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}
