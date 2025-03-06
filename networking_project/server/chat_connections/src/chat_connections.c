#include "chat_connections.h"
#include "chat_connections_internal.h"
#include "chat_connections_fsm.h"

#include <stdint.h>
#include <string.h>

#include "message_queue.h"
#include "network_watcher.h"


static void init_cblk(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr)
{
    memset(master_cblk_ptr, 0, sizeof(sCHAT_CONNECTIONS_CBLK));

    master_cblk_ptr->state = CHAT_CONNECTIONS_STATE_OPEN;
}


static void init_connections_list(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr)
{
    uint32_t connection_index;

    for (connection_index = 0;
         connection_index < master_cblk_ptr->max_connections;
         connection_index++)
    {
        master_cblk_ptr->connections[connection_index] = NULL;
    }
}


eSTATUS chat_connections_create(
    CHAT_CONNECTIONS*            out_new_chat_connections,
    fCHAT_CONNECTIONS_USER_CBACK user_cback,
    void*                        user_arg,
    uint32_t                     default_size)
{
    sCHAT_CONNECTIONS_CBLK* new_chat_connections_cblk;
    eSTATUS                 status;

    new_chat_connections_cblk = (sCHAT_CONNECTIONS_CBLK*)generic_allocator(sizeof(sCHAT_CONNECTIONS_CBLK));
    if (NULL == new_chat_connections_cblk)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }
    init_cblk(new_chat_connections_cblk);

    new_chat_connections_cblk->user_cback = user_cback;
    new_chat_connections_cblk->user_arg   = user_arg;

    new_chat_connections_cblk->connection_count = 0;
    new_chat_connections_cblk->max_connections  = default_size;

    status = message_queue_create(new_chat_connections_cblk->message_queue,
                                  CHAT_CONNECTION_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_CONNECTIONS_MESSAGE));
    if (STATUS_SUCCESS != status);
    {
        goto fail_create_message_queue;
    }

    new_chat_connections_cblk->connections = generic_allocator(new_chat_connections_cblk->max_connections * sizeof(CHAT_CONNECTION));
    if (NULL == new_chat_connections_cblk->connections)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_connection_list;
    }
    init_connections_list(new_chat_connections_cblk);

    *out_new_chat_connections = (CHAT_CONNECTIONS)new_chat_connections_cblk;
    return STATUS_SUCCESS;

fail_alloc_connection_list:
    message_queue_destroy(new_chat_connections_cblk->message_queue);

fail_create_message_queue:
    generic_deallocator(new_chat_connections_cblk);

fail_alloc_cblk:
    return status;
}


eSTATUS chat_connection_queue_event(
    sCHAT_CONNECTION* connection,
    const sCHAT_EVENT*       event)
{

}


eSTATUS chat_connection_do_read(
    sCHAT_CONNECTION* connection)
{

}


eSTATUS chat_connection_do_write(
    sCHAT_CONNECTION* connection)
{

}
