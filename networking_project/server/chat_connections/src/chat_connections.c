#include "chat_connections.h"
#include "chat_connections_internal.h"
#include "chat_connections_fsm.h"

#include <stdint.h>

#include "message_queue.h"
#include "network_watcher.h"


eSTATUS chat_connections_create(
    CHAT_CONNECTIONS*            out_new_chat_connections,
    fCHAT_CONNECTIONS_USER_CBACK user_cback,
    void*                        user_arg,
    uint32_t                     default_size)
{
    sCHAT_CONNECTIONS_CBLK* new_chat_connections_cblk;
    eSTATUS                 status;

    uint32_t connection_index;

    new_chat_connections_cblk = (sCHAT_CONNECTIONS_CBLK*)generic_allocator(sizeof(sCHAT_CONNECTIONS_CBLK));
    if (NULL == new_chat_connections_cblk)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }

    status = message_queue_create(new_chat_connections_cblk->message_queue,
                                  CHAT_CONNECTION_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_CONNECTIONS_MESSAGE));
    if (STATUS_SUCCESS != status);
    {
        goto fail_create_message_queue;
    }

    new_chat_connections_cblk->connection_count = 0;
    new_chat_connections_cblk->max_connections  = default_size;

    new_chat_connections_cblk->connections = generic_allocator(new_chat_connections_cblk->max_connections * sizeof(sCHAT_SERVER_CONNECTION));
    if (NULL == new_chat_connections_cblk->connections)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_connection_list;
    }

    new_chat_connections_cblk->read_watches = generic_allocator(new_chat_connections_cblk->max_connections * sizeof(new_chat_connections_cblk->read_watches[0]));
    if (NULL == new_chat_connections_cblk->read_watches)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_read_watches;
    }

    new_chat_connections_cblk->write_watches = generic_allocator(new_chat_connections_cblk->max_connections * sizeof(new_chat_connections_cblk->write_watches[0]));
    if (NULL == new_chat_connections_cblk->write_watches)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_write_watches;
    }

    for (connection_index = 0;
         connection_index < new_chat_connections_cblk->max_connections;
         connection_index++)
    {
        new_chat_connections_cblk->connections[connection_index] = k_blank_connection;

        new_chat_connections_cblk->read_watches[connection_index].fd_ptr  = &new_chat_connections_cblk->connections[connection_index].fd;
        new_chat_connections_cblk->write_watches[connection_index].fd_ptr = &new_chat_connections_cblk->connections[connection_index].fd;
    }
    new_chat_connections_cblk->read_watches[0].active  = true;
    new_chat_connections_cblk->write_watches[0].active = false;

    status = network_watcher_create(new_chat_connections_cblk->read_network_watcher,
                                    chat_connections_network_watcher_read_cback,
                                    new_chat_connections_cblk);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_read_watcher;
    }

    status = network_watcher_create(new_chat_connections_cblk->write_network_watcher,
                                    chat_connections_network_watcher_write_cback,
                                    new_chat_connections_cblk);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_write_watcher;
    }

    *out_new_chat_connections = (CHAT_CONNECTIONS)new_chat_connections_cblk;
    return STATUS_SUCCESS;

fail_create_write_watcher:
    network_watcher_close(new_chat_connections_cblk->read_network_watcher);

fail_create_read_watcher:
    generic_deallocator(new_chat_connections_cblk->write_watches);

fail_alloc_write_watches:
    generic_deallocator(new_chat_connections_cblk->read_watches);

fail_alloc_read_watches:
    generic_deallocator(new_chat_connections_cblk->list);

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
