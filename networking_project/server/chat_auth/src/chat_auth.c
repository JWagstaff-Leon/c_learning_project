#include "chat_auth.h"
#include "chat_auth_internal.h"
#include "chat_auth_fsm.h"

#include <stdio.h>
#include <string.h>

#include "chat_user.h"
#include "common_types.h"


eSTATUS chat_auth_create(
    CHAT_AUTH*            out_chat_auth,
    fCHAT_AUTH_USER_CBACK user_back,
    void*                 user_arg)
{
    eSTATUS            status;
    sCHAT_AUTH_MESSAGE message;


    sCHAT_AUTH_CBLK* new_auth_chat_cblk;

    new_auth_chat_cblk = generic_allocator(sizeof(sCHAT_AUTH_CBLK));
    if (NULL == new_auth_chat_cblk)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }

    memset(new_auth_chat_cblk, 0, sizeof(sCHAT_AUTH_CBLK));
    new_auth_chat_cblk->state = CHAT_AUTH_STATE_NO_DATABASE;

    status = message_queue_create(new_auth_chat_cblk->message_queue,
                                  CHAT_AUTH_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_AUTH_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_message_queue;
    }

    status = generic_create_thread(chat_auth_thread_entry, new_auth_chat_cblk);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    *out_chat_auth = (CHAT_AUTH)new_auth_chat_cblk;
    status = STATUS_SUCCESS;
    goto success;

fail_create_thread:
    message_queue_destroy(new_auth_chat_cblk->message_queue);

fail_create_message_queue:
    generic_deallocator(new_auth_chat_cblk);

fail_alloc_cblk:
success:
    return status;
}


eSTATUS chat_auth_open_db(
    CHAT_AUTH   chat_auth,
    const char* db_path)
{
    sCHAT_AUTH_CBLK*   master_cblk_ptr;
    sCHAT_AUTH_MESSAGE message;
    eSTATUS            status;

    if (NULL == chat_auth)
    {
        return STATUS_INVALID_ARG;
    }

    if (NULL == db_path)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_AUTH_CBLK*)chat_auth;

    message.type                      = CHAT_AUTH_MESSAGE_OPEN_DATABASE;
    message.params.open_database.path = generic_allocator(strlen(db_path) + 1);
    if (NULL == message.params.open_database.path)
    {
        return STATUS_ALLOC_FAILED;
    }
    strcpy(message.params.open_database.path, db_path);

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    if (STATUS_SUCCESS != status)
    {
        generic_deallocator(message.params.open_database.path);
    }
    return status;
}


eSTATUS chat_auth_close_db(
    CHAT_AUTH   chat_auth,
    const char* db_path)
{
    sCHAT_AUTH_CBLK*   master_cblk_ptr;
    sCHAT_AUTH_MESSAGE message;
    eSTATUS            status;

    if (NULL == chat_auth)
    {
        return STATUS_INVALID_ARG;
    }

    message.type                       = CHAT_AUTH_MESSAGE_CLOSE_DATABASE;
    message.params.close_database.path = NULL;

    if (NULL != db_path)
    {
        master_cblk_ptr = (sCHAT_AUTH_CBLK*)chat_auth;

        message.params.close_database.path = generic_allocator(strlen(db_path) + 1);
        if (NULL == message.params.close_database.path)
        {
            return STATUS_ALLOC_FAILED;
        }
        strcpy(message.params.close_database.path, db_path);
    }

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    if (STATUS_SUCCESS != status)
    {
        generic_deallocator(message.params.close_database.path);
    }
    return status;
}


eSTATUS chat_auth_submit_credentials(
    CHAT_AUTH              chat_auth,
    void*                  auth_object,
    sCHAT_USER_CREDENTIALS credentials)
{
    sCHAT_AUTH_CBLK*   master_cblk_ptr;
    sCHAT_AUTH_MESSAGE message;
    eSTATUS            status;

    if (NULL == chat_auth)
    {
        return STATUS_INVALID_ARG;
    }

    if (NULL == auth_object)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_AUTH_CBLK*)chat_auth;

    message.type                                   = CHAT_AUTH_MESSAGE_PROCESS_CREDENTIALS;
    message.params.process_credentials.credentials = credentials;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}


eSTATUS chat_auth_close(
    CHAT_AUTH chat_auth)
{
    sCHAT_AUTH_CBLK*   master_cblk_ptr;
    sCHAT_AUTH_MESSAGE message;
    eSTATUS            status;

    if (NULL == chat_auth)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_AUTH_CBLK*)chat_auth;

    message.type = CHAT_AUTH_MESSAGE_SHUTDOWN;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}
