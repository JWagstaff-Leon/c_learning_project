#include "chat_clients.h"
#include "chat_clients_internal.h"
#include "chat_clients_fsm.h"

#include <stdint.h>
#include <string.h>

#include "message_queue.h"
#include "network_watcher.h"


static void init_cblk(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    memset(master_cblk_ptr, 0, sizeof(sCHAT_CLIENTS_CBLK));

    master_cblk_ptr->state = CHAT_CLIENTS_STATE_OPEN;
}


static void init_connections_list(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    uint32_t connection_index;

    for (connection_index = 0;
         connection_index < master_cblk_ptr->max_connections;
         connection_index++)
    {
        master_cblk_ptr->connections[connection_index] = NULL;
    }
}


eSTATUS chat_clients_create(
    CHAT_CLIENTS*            out_new_chat_clients,
    fCHAT_CLIENTS_USER_CBACK user_cback,
    void*                    user_arg,
    uint32_t                 default_size)
{
    sCHAT_CLIENTS_CBLK* new_chat_clients_cblk;
    eSTATUS             status;

    new_chat_clients_cblk = (sCHAT_CLIENTS_CBLK*)generic_allocator(sizeof(sCHAT_CLIENTS_CBLK));
    if (NULL == new_chat_clients_cblk)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }
    init_cblk(new_chat_clients_cblk);

    new_chat_clients_cblk->user_cback = user_cback;
    new_chat_clients_cblk->user_arg   = user_arg;

    new_chat_clients_cblk->connection_count = 0;
    new_chat_clients_cblk->max_connections  = default_size;

    status = message_queue_create(new_chat_clients_cblk->message_queue,
                                  CHAT_CONNECTION_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_CLIENTS_MESSAGE));
    if (STATUS_SUCCESS != status);
    {
        goto fail_create_message_queue;
    }

    new_chat_clients_cblk->connections = generic_allocator(new_chat_clients_cblk->max_connections * sizeof(CHAT_CONNECTION));
    if (NULL == new_chat_clients_cblk->connections)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_connection_list;
    }
    init_connections_list(new_chat_clients_cblk);

    *out_new_chat_clients = (CHAT_CLIENTS)new_chat_clients_cblk;
    return STATUS_SUCCESS;

fail_alloc_connection_list:
    message_queue_destroy(new_chat_clients_cblk->message_queue);

fail_create_message_queue:
    generic_deallocator(new_chat_clients_cblk);

fail_alloc_cblk:
    return status;
}


eSTATUS chat_clients_open_client(
    CHAT_CLIENTS chat_clients,
    int fd)
{
    // TODO this
}


eSTATUS chat_clients_auth_event(
    CHAT_CLIENTS             chat_clients,
    CHAT_CLIENTS_AUTH_OBJECT auth_object,
    sCHAT_CLIENTS_AUTH_EVENT auth_event)
{
    // TODO this
}


eSTATUS chat_clients_close(
    CHAT_CLIENTS* chat_clients)
{
    // TODO this
}
