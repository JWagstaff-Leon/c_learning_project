// Includes --------------------------------------------------------------------

#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "chat_auth.h"
#include "chat_clients.h"
#include "common_types.h"
#include "message_queue.h"


// Function Implementations ----------------------------------------------------

static void init_cblk(
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    memset(master_cblk_ptr, 0, sizeof(master_cblk_ptr));

    master_cblk_ptr->state     = CHAT_SERVER_STATE_INIT;
    master_cblk_ptr->listen_fd = -1;
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

    init_cblk(new_chat_server);

    new_chat_server->user_cback = user_cback;
    new_chat_server->user_arg   = user_arg;

    status = message_queue_create(&new_chat_server->message_queue,
                                  CHAT_SERVER_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_SERVER_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_msg_queue;
    }

    status = chat_clients_create(&new_chat_server->clients,
                                 chat_server_clients_cback,
                                 new_chat_server);
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

    *out_new_chat_server = new_chat_server;
    return STATUS_SUCCESS;

fail_create_connection_listener:
    chat_auth_destroy(new_chat_server->auth);

fail_create_auth:
    chat_clients_destroy(new_chat_server->clients);

fail_create_clients:
    message_queue_destroy(new_chat_server->message_queue);

fail_create_msg_queue:
    generic_deallocator(new_chat_server);

fail_alloc_cblk:
    return status;
}


eSTATUS chat_server_open(
    CHAT_SERVER chat_server)
{
    eSTATUS            status;
    sCHAT_SERVER_CBLK* master_cblk_ptr;
    
    if (NULL == chat_server)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_SERVER_CBLK*)chat_server;
    if (CHAT_SERVER_STATE_INIT != master_cblk_ptr->state)
    {
        return STATUS_INVALID_STATE;
    }

    status = open_listen_socket(&master_cblk_ptr->listen_fd);
    if (STATUS_SUCCESS != status)
    {
        goto fail_open_listen_socket;
    }

    master_cblk_ptr->state = CHAT_SERVER_STATE_OPEN;
    status = generic_create_thread(chat_server_thread_entry,
                                   master_cblk_ptr,
                                   NULL);
    if (STATUS_SUCCESS != status)
    {
        master_cblk_ptr->state = CHAT_SERVER_STATE_INIT;
        goto fail_create_thread;
    }

    return STATUS_SUCCESS;

fail_create_thread:
    close(master_cblk_ptr->listen_fd);

fail_open_listen_socket:
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


eSTATUS chat_server_destroy(
    CHAT_SERVER chat_server)
{
    eSTATUS            status;
    sCHAT_SERVER_CBLK* master_cblk_ptr;
    
    if (NULL == chat_server)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_SERVER_CBLK*)chat_server;
    if (CHAT_SERVER_STATE_INIT   != master_cblk_ptr->state &&
        CHAT_SERVER_STATE_CLOSED != master_cblk_ptr->state)
    {
        return STATUS_INVALID_STATE;
    }

    status = network_watcher_destroy(master_cblk_ptr->connection_listener);
    assert(STATUS_SUCCESS == status);

    status = chat_auth_destroy(master_cblk_ptr->auth);
    assert(STATUS_SUCCESS == status);
    
    status = chat_clients_destroy(master_cblk_ptr->clients);
    assert(STATUS_SUCCESS == status);

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);

    generic_deallocator(master_cblk_ptr);
    return STATUS_SUCCESS;
}
