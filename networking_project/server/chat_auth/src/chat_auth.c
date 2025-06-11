#include "chat_auth.h"
#include "chat_auth_internal.h"
#include "chat_auth_fsm.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"

#include "chat_user.h"
#include "common_types.h"


eSTATUS chat_auth_create(
    CHAT_AUTH*            out_chat_auth,
    const char*           database_path,
    fCHAT_AUTH_USER_CBACK user_cback,
    void*                 user_arg)
{
    eSTATUS status;
    int     sqlite_status;

    sCHAT_AUTH_CBLK* new_auth_chat_cblk;

    new_auth_chat_cblk = generic_allocator(sizeof(sCHAT_AUTH_CBLK));
    if (NULL == new_auth_chat_cblk)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }

    memset(new_auth_chat_cblk, 0, sizeof(sCHAT_AUTH_CBLK));
    new_auth_chat_cblk->state = CHAT_AUTH_STATE_OPEN;

    status = message_queue_create(&new_auth_chat_cblk->message_queue,
                                  CHAT_AUTH_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_AUTH_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_message_queue;
    }

    sqlite_status = sqlite3_open_v2(database_path,
                                    &new_auth_chat_cblk->database,
                                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                    NULL);
    if(SQLITE_OK != sqlite_status)
    {
        status = STATUS_FAILURE;
        goto fail_open_database;
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
    sqlite3_close_v2(new_auth_chat_cblk->database);

fail_open_database:
    message_queue_destroy(new_auth_chat_cblk->message_queue);

fail_create_message_queue:
    generic_deallocator(new_auth_chat_cblk);

fail_alloc_cblk:
success:
    return status;
}


eSTATUS chat_auth_submit_credentials(
    CHAT_AUTH              chat_auth,
    sCHAT_USER_CREDENTIALS credentials,
    void*                  consumer_arg,
    CHAT_AUTH_TRANSACTION* out_auth_transaction)
{
    sCHAT_AUTH_CBLK*   master_cblk_ptr;
    sCHAT_AUTH_MESSAGE message;
    eSTATUS            status;

    sCHAT_AUTH_TRANSACTION* new_auth_transaction;

    if (NULL == chat_auth)
    {
        return STATUS_INVALID_ARG;
    }

    if (NULL == out_auth_transaction)
    {
        return STATUS_INVALID_ARG;
    }

    new_auth_transaction = generic_allocator(sizeof(sCHAT_AUTH_TRANSACTION));
    if (NULL == new_auth_transaction)
    {
        return STATUS_ALLOC_FAILED;
    }

    new_auth_transaction->consumer_arg = consumer_arg;
    new_auth_transaction->state        = CHAT_AUTH_TRANSACTION_STATE_PROCESSING;
    pthread_mutex_init(&new_auth_transaction->mutex, NULL);

    message.type = CHAT_AUTH_MESSAGE_PROCESS_CREDENTIALS;

    message.params.process_credentials.credentials      = credentials;
    message.params.process_credentials.auth_transaction = new_auth_transaction;

    master_cblk_ptr = (sCHAT_AUTH_CBLK*)chat_auth;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    if (status != STATUS_SUCCESS)
    {
        generic_deallocator(new_auth_transaction);
        return status;
    }

    *out_auth_transaction = (CHAT_AUTH_TRANSACTION)new_auth_transaction;
    return STATUS_SUCCESS;
}


eSTATUS chat_auth_finish_transaction(
    CHAT_AUTH_TRANSACTION auth_transaction)
{
    sCHAT_AUTH_TRANSACTION* auth_transaction_cblk;

    if (NULL == auth_transaction)
    {
        return STATUS_INVALID_ARG;
    }

    auth_transaction_cblk = (sCHAT_AUTH_TRANSACTION*)auth_transaction;

    pthread_mutex_lock(&auth_transaction_cblk->mutex);

    auth_transaction_cblk->consumer_arg = NULL;

    switch (auth_transaction_cblk->state)
    {
        case CHAT_AUTH_TRANSACTION_STATE_PROCESSING:
        {
            auth_transaction_cblk->state = CHAT_AUTH_TRANSACTION_STATE_CANCELLED;
            pthread_mutex_unlock(&auth_transaction_cblk->mutex);
            break;
        }
        case CHAT_AUTH_TRANSACTION_STATE_CANCELLED:
        {
            // Nothing to be done in this case
            pthread_mutex_unlock(&auth_transaction_cblk->mutex);
            break;
        }
        case CHAT_AUTH_TRANSACTION_STATE_DONE:
        {
            pthread_mutex_unlock(&auth_transaction_cblk->mutex);
            pthread_mutex_destroy(&auth_transaction_cblk->mutex);
            generic_deallocator(auth_transaction_cblk);
            break;
        }
        default:
        {
            assert(0); // Should never get here
            pthread_mutex_unlock(&auth_transaction_cblk->mutex);
            break;
        }
    }

    return STATUS_SUCCESS;
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

    message.type = CHAT_AUTH_MESSAGE_CLOSE;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}
