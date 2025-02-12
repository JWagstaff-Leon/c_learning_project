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

    new_chat_connections_cblk->list = generic_allocator(new_chat_connections_cblk->size * sizeof(sCHAT_SERVER_CONNECTION));
    if (NULL == new_chat_connections_cblk->list)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_connection_list;
    }

    new_chat_connections_cblk->count = 0;
    new_chat_connections_cblk->size  = default_size;

    new_chat_connections_cblk->read_fd_buffer = generic_allocator(new_chat_connections_cblk->size * sizeof(new_chat_connections_cblk->read_fd_buffer[0]));
    if (NULL == new_chat_connections_cblk->read_fd_buffer)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_read_fd_buffer;
    }

    new_chat_connections_cblk->write_fd_buffer = generic_allocator(new_chat_connections_cblk->size * sizeof(new_chat_connections_cblk->write_fd_buffer[0]));
    if (NULL == new_chat_connections_cblk->write_fd_buffer)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_write_fd_buffer;
    }

    for (connection_index = 0;
         connection_index < new_chat_connections_cblk->size;
         connection_index++)
    {
        new_chat_connections_cblk->list[connection_index] = k_blank_connection;

        new_chat_connections_cblk->read_fd_buffer[connection_index].fd  = -1;
        new_chat_connections_cblk->write_fd_buffer[connection_index].fd = -1;
    }

    status = network_watcher_create(new_master_cblk_ptr->read_network_watcher,
                                    chat_connections_network_watcher_read_cback,
                                    new_chat_connections_cblk,
                                    new_chat_connections_cblk->size);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_read_watcher;
    }

    status = network_watcher_create(new_master_cblk_ptr->write_network_watcher,
                                    chat_connections_network_watcher_write_cback,
                                    new_chat_connections_cblk,
                                    new_chat_connections_cblk->size);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_write_watcher;
    }

    *out_new_chat_connections = (CHAT_CONNECTIONS)new_chat_connections_cblk;
    return STATUS_SUCCESS;

fail_create_write_wacther:
    network_watcher_close(new_chat_connections_cblk->read_network_watcher);

fail_create_read_wacther:
    generic_deallocator(new_chat_connections_cblk->write_fd_buffer);

fail_alloc_write_fd_buffer:
    generic_deallocator(new_chat_connections_cblk->read_fd_buffer);

fail_alloc_read_fd_buffer:
    generic_deallocator(new_chat_connections_cblk->list);

fail_alloc_connection_list:
    message_queue_destroy(new_chat_connections_cblk->message_queue);

fail_create_message_queue:
    generic_deallocator(new_chat_connections_cblk);

fail_alloc_cblk:
    return status;
}


eSTATUS chat_connections_new_connection(
    CHAT_CONNECTIONS connections,
    int              connection_fd)
{

}


static bool is_connection_readable(
    const sCHAT_CONNECTION* connection)
{
    // Function exists for ease of enhancement
    return true;
}


eSTATUS chat_connections_get_readable(
    const sCHAT_CONNECTIONS* restrict connections,
    int*                     restrict out_fds)
{
    uint32_t          connection_index;
    sCHAT_CONNECTION* current_connection;

    assert(NULL != connections);
    assert(NULL != out_fds);

     // We only care about reading from the incoming connection socket fd
    out_fds[0] = connections->list[0].fd;

    for (connection_index = 1; connection_index < connections->size; connection_index++)
    {
        current_connection = connections->list[connection_index];
        if (is_connection_readable(current_connection))
        {
            out_fds[connection_index] = current_connection->fd;
        }
        else
        {
            out_fds[connection_index] = -1;
        }
    }

    return STATUS_SUCCESS;
}


static bool is_connection_writeable(
    const sCHAT_CONNECTION* connection)
{
    if (CHAT_CONNECTION_STATE_DISCONNECTED == connection->state)
    {
        return false;
    }

    if (message_queue_get_count(connection->event_buffer) <= 0)
    {
        return false;
    }

    return true;
}


eSTATUS chat_connections_get_writeable(
    const sCHAT_CONNECTIONS* restrict connections,
    int*                     restrict out_fds)
{
    uint32_t          connection_index;
    sCHAT_CONNECTION* current_connection;

    assert(NULL != connections);
    assert(NULL != out_fds);

    out_fds[0] = -1;

    for (connection_index = 1; connection_index < connections->size; connection_index++)
    {
        current_connection = connections->list[connection_index];
        if (is_connection_writeable(current_connection))
        {
            out_fds[connection_index] = connections->list[connection_index].fd;
        }
        else
        {
            out_fds[connection_index] = -1;
        }
    }

    return STATUS_SUCCESS;
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