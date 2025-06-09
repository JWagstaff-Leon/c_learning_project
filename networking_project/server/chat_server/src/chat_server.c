// Includes --------------------------------------------------------------------

#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "chat_auth.h"
#include "chat_clients.h"
#include "common_types.h"
#include "message_queue.h"


// Function Implementations ----------------------------------------------------

static eSTATUS init_cblk(
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);

    memset(master_cblk_ptr, 0, sizeof(master_cblk_ptr));

    master_cblk_ptr->state     = CHAT_SERVER_STATE_OPEN;
    master_cblk_ptr->listen_fd = -1;

    return STATUS_SUCCESS;
}


eSTATUS chat_server_create(
    CHAT_SERVER*            out_new_chat_server,
    fCHAT_SERVER_USER_CBACK user_cback,
    void*                   user_arg)
{
    sCHAT_SERVER_CBLK* new_chat_server;
    eSTATUS            status;

    assert(NULL != out_new_chat_server);
    assert(NULL != user_cback);

    new_chat_server = (sCHAT_SERVER_CBLK*)generic_allocator(sizeof(sCHAT_SERVER_CBLK));
    if (NULL == new_chat_server)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }

    status = init_cblk(new_chat_server);
    if (STATUS_SUCCESS != status)
    {
        goto fail_init_cblk;
    }

    new_chat_server->user_cback = user_cback;
    new_chat_server->user_arg   = user_arg;

    status = message_queue_create(&new_chat_server->message_queue,
                                  CHAT_SERVER_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_SERVER_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_init_msg_queue;
    }

    status = chat_clients_create(&new_chat_server->clients,
                                 chat_server_clients_cback,
                                 new_chat_server,
                                 CHAT_SERVER_DEFAULT_MAX_CONNECTIONS);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_clients;
    }

    status = chat_auth_create(&new_chat_server->auth,
                              chat_server_auth_cback,
                              new_chat_server);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_auth;
    }

    status = network_watcher_create(&new_chat_server->connection_listener,
                                    chat_server_connection_listener_cback,
                                    new_chat_server);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_connection_listener;
    }

    status = generic_create_thread(chat_server_thread_entry, new_chat_server);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_thread;
    }

    *out_new_chat_server = new_chat_server;
    status               = STATUS_SUCCESS;
    goto success;

fail_create_thread:
    network_watcher_close(new_chat_server->connection_listener);

fail_create_connection_listener:
    chat_auth_close(new_chat_server->auth);

fail_create_auth:
    // FIXME change all immediate "close"s to "destroy"s; "destroy"s will not callback when closed
    //       calling back would lead to a use-after-free (trying to get the message queue off of a freed cblk)
    chat_clients_close(new_chat_server->clients);

fail_create_clients:
    message_queue_destroy(new_chat_server->message_queue);

fail_init_msg_queue:
    // Fallthrough

fail_init_cblk:
    generic_deallocator(new_chat_server);

fail_alloc_cblk:
    // Fallthrough

success:
    return status;
}


eSTATUS chat_server_close(
    CHAT_SERVER chat_server)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE message;
    sCHAT_SERVER_CBLK*   master_cblk_ptr;

    assert(NULL != chat_server);
    master_cblk_ptr = (sCHAT_SERVER_CBLK*)chat_server;

    message.type = CHAT_SERVER_MESSAGE_CLOSE;
    status       = message_queue_put(master_cblk_ptr->message_queue,
                                     &message,
                                     sizeof(message));

    return status;
}
