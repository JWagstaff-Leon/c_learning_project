#include "chat_clients.h"
#include "chat_clients_internal.h"
#include "chat_clients_fsm.h"

#include <assert.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>

#include "chat_user.h"
#include "common_types.h"
#include "message_queue.h"
#include "network_watcher.h"


static void init_cblk(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    memset(master_cblk_ptr, 0, sizeof(sCHAT_CLIENTS_CBLK));

    master_cblk_ptr->state = CHAT_CLIENTS_STATE_OPEN;
}


eSTATUS chat_clients_create(
    CHAT_CLIENTS*            out_new_chat_clients,
    fCHAT_CLIENTS_USER_CBACK user_cback,
    void*                    user_arg)
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

    status = message_queue_create(&new_chat_clients_cblk->message_queue,
                                  CHAT_CONNECTION_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_CLIENTS_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_message_queue;
    }

    status = generic_create_thread(chat_clients_thread_entry,
                                   new_chat_clients_cblk,
                                   NULL);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_thread;
    }

    *out_new_chat_clients = (CHAT_CLIENTS)new_chat_clients_cblk;
    return STATUS_SUCCESS;

fail_create_thread:
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
    eSTATUS               status;
    sCHAT_CLIENTS_CBLK*   master_cblk_ptr;
    sCHAT_CLIENTS_MESSAGE message;

    assert(NULL != chat_clients);
    master_cblk_ptr = (sCHAT_CLIENTS_CBLK*)chat_clients;

    message.type                  = CHAT_CLIENTS_MESSAGE_OPEN_CLIENT;
    message.params.open_client.fd = fd;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}


eSTATUS chat_clients_auth_event(
    CHAT_CLIENTS            chat_clients,
    eCHAT_CLIENTS_AUTH_STEP auth_step,
    sCHAT_USER              user_info,
    SHARED_PTR              client_ptr)
{
    eSTATUS               status;
    sCHAT_CLIENTS_CBLK*   master_cblk_ptr;
    sCHAT_CLIENTS_MESSAGE message;

    if (NULL == chat_clients)
    {
        return STATUS_INVALID_ARG;
    }
    if (NULL == client_ptr || NULL == SP_POINTEE(client_ptr))
    {
        return STATUS_INVALID_ARG;
    }
    
    master_cblk_ptr = (sCHAT_CLIENTS_CBLK*)chat_clients;

    message.type = CHAT_CLIENTS_MESSAGE_AUTH_EVENT;

    message.params.auth_event.auth_step  = auth_step;
    message.params.auth_event.user_info  = user_info;
    message.params.auth_event.client_ptr = client_ptr;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}


eSTATUS chat_clients_close(
    CHAT_CLIENTS* chat_clients)
{
    // TODO this
}
