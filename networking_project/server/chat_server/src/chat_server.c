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
    master_cblk_ptr->state     = CHAT_SERVER_STATE_INIT;
    master_cblk_ptr->listen_fd = -1;

    return STATUS_SUCCESS;
}


eSTATUS chat_server_create(
    CHAT_SERVER*            out_new_chat_server,
    fCHAT_SERVER_USER_CBACK user_cback,
    void*                   user_arg)
{
    sCHAT_SERVER_CBLK* new_master_cblk_ptr;
    eSTATUS            status;
    
    assert(NULL != user_cback);
    assert(NULL != out_new_chat_server);

    new_master_cblk_ptr = (sCHAT_SERVER_CBLK*)generic_allocator(sizeof(sCHAT_SERVER_CBLK));
    if (NULL == new_master_cblk_ptr)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }

    status = init_cblk(new_master_cblk_ptr);
    if (STATUS_SUCCESS != status)
    {
        goto fail_init_cblk;
    }

    new_master_cblk_ptr->user_cback = user_cback;
    new_master_cblk_ptr->user_arg   = user_arg;

    status = message_queue_create(&new_master_cblk_ptr->message_queue,
                                  CHAT_SERVER_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_SERVER_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_init_msg_queue;
    }

    status = chat_clients_create(&new_master_cblk_ptr->clients,
                                 chat_server_clients_cback,
                                 new_master_cblk_ptr,
                                 8); // REVIEW make this 8 dynamic/configurable?
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_clients;
    }

    status = chat_auth_create(&new_master_cblk_ptr->auth,
                              chat_server_auth_cback,
                              new_master_cblk_ptr);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_auth;
    }

    status = network_watcher_create(&new_master_cblk_ptr->listening_connection,
                                    chat_server_listening_connection_cback,
                                    new_master_cblk_ptr);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_listening_connection;
    }

    status = generic_create_thread(chat_server_thread_entry, new_master_cblk_ptr);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_thread;
    }
    
    *out_new_chat_server = new_master_cblk_ptr;
    status               = STATUS_SUCCESS;
    goto func_exit;

fail_create_thread:
    network_watcher_close(new_master_cblk_ptr->listening_connection);

fail_create_listening_connection:
    chat_auth_close(new_master_cblk_ptr->auth);

fail_create_auth:
    // FIXME change all immediate "close"s to "destroy"s; "destroy"s will not callback when closed
    //       calling back would lead to a use-after-free (trying to get the message queue off of a freed cblk)
    chat_clients_close(new_master_cblk_ptr->clients);

fail_create_clients:
    message_queue_destroy(new_master_cblk_ptr->message_queue);

fail_init_msg_queue:
    // Fallthrough

fail_init_cblk:
    generic_deallocator(new_master_cblk_ptr);

fail_alloc_cblk:
    // Fallthrough

func_exit:
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
