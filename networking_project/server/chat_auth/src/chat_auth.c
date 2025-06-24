#include "chat_auth.h"
#include "chat_auth_internal.h"
#include "chat_auth_fsm.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sqlite3.h"

#include "chat_user.h"
#include "common_types.h"
#include "shared_ptr.h"


eSTATUS chat_auth_create(
    CHAT_AUTH*            out_chat_auth,
    fCHAT_AUTH_USER_CBACK user_cback,
    void*                 user_arg)
{
    eSTATUS          status;
    sCHAT_AUTH_CBLK* new_auth_chat_cblk;

    new_auth_chat_cblk = generic_allocator(sizeof(sCHAT_AUTH_CBLK));
    if (NULL == new_auth_chat_cblk)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }

    memset(new_auth_chat_cblk, 0, sizeof(sCHAT_AUTH_CBLK));
    new_auth_chat_cblk->state = CHAT_AUTH_STATE_INIT;

    new_auth_chat_cblk->user_cback = user_cback;
    new_auth_chat_cblk->user_arg   = user_arg;

    status = message_queue_create(&new_auth_chat_cblk->message_queue,
                                  CHAT_AUTH_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_AUTH_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_message_queue;
    }

    *out_chat_auth = (CHAT_AUTH)new_auth_chat_cblk;
    return STATUS_SUCCESS;

fail_create_message_queue:
    generic_deallocator(new_auth_chat_cblk);

fail_alloc_cblk:
    return status;
}


eSTATUS chat_auth_open(
    CHAT_AUTH   chat_auth,
    const char* database_path)
{
    eSTATUS status;
    int     sqlite_status;

    sCHAT_AUTH_CBLK* master_cblk_ptr;

    if (NULL == chat_auth)
    {
        return STATUS_INVALID_ARG;
    }

    if (NULL == database_path)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_AUTH_CBLK*)chat_auth;
    if (CHAT_AUTH_STATE_INIT != master_cblk_ptr->state)
    {
        return STATUS_INVALID_STATE;
    }

    sqlite_status = sqlite3_open_v2(database_path,
                                    &master_cblk_ptr->database,
                                    SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                    NULL);
    if(SQLITE_OK != sqlite_status)
    {
        status = STATUS_FAILURE;
        goto fail_open_database;
    }

    status = generic_create_thread(chat_auth_thread_entry,
                                   master_cblk_ptr,
                                   NULL);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_thread;
    }

    return STATUS_SUCCESS;

fail_create_thread:
    sqlite3_close_v2(master_cblk_ptr->database);

fail_open_database:
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

    message.type = CHAT_AUTH_MESSAGE_CLOSE;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}

eSTATUS chat_auth_destroy(
    CHAT_AUTH chat_auth)
{
    eSTATUS          status;
    sCHAT_AUTH_CBLK* master_cblk_ptr;

    if (NULL == chat_auth)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_AUTH_CBLK*)chat_auth;
    if (CHAT_AUTH_STATE_INIT   != master_cblk_ptr->state &&
        CHAT_AUTH_STATE_CLOSED != master_cblk_ptr->state)
    {
        return STATUS_INVALID_STATE;
    }

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);

    generic_deallocator(master_cblk_ptr);
    return STATUS_SUCCESS;
}


eSTATUS chat_auth_submit_credentials(
    CHAT_AUTH              chat_auth,
    SHARED_PTR             credentials_ptr,
    SHARED_PTR             consumer_arg_ptr,
    CHAT_AUTH_TRANSACTION* out_auth_transaction)
{
    sCHAT_AUTH_CBLK*   master_cblk_ptr;
    sCHAT_AUTH_MESSAGE message;
    eSTATUS            status;

    sCHAT_AUTH_TRANSACTION* new_auth_transaction;

    if (NULL == chat_auth)
    {
        status = STATUS_INVALID_ARG;
        goto func_fail;

    }

    if (NULL == out_auth_transaction)
    {
        status = STATUS_INVALID_ARG;
        goto func_fail;
    }

    new_auth_transaction = generic_allocator(sizeof(sCHAT_AUTH_TRANSACTION));
    if (NULL == new_auth_transaction)
    {
        status = STATUS_ALLOC_FAILED;
        goto func_fail;

    }

    master_cblk_ptr = (sCHAT_AUTH_CBLK*)chat_auth;

    new_auth_transaction->consumer_arg_ptr = consumer_arg_ptr;
    new_auth_transaction->state            = CHAT_AUTH_TRANSACTION_STATE_PROCESSING;
    pthread_mutex_init(&new_auth_transaction->mutex, NULL);

    message.type = CHAT_AUTH_MESSAGE_PROCESS_CREDENTIALS;

    message.params.process_credentials.credentials_ptr  = credentials_ptr;
    message.params.process_credentials.auth_transaction = new_auth_transaction;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    if (status != STATUS_SUCCESS)
    {
        generic_deallocator(new_auth_transaction);
        goto func_fail;
    }

    *out_auth_transaction = (CHAT_AUTH_TRANSACTION)new_auth_transaction;
    return STATUS_SUCCESS;

func_fail:
    shared_ptr_release(credentials_ptr);
    shared_ptr_release(consumer_arg_ptr);

    return status;
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
    switch (auth_transaction_cblk->state)
    {
        case CHAT_AUTH_TRANSACTION_STATE_PROCESSING:
        {
            auth_transaction_cblk->state = CHAT_AUTH_TRANSACTION_STATE_CANCELLED;
            break;
        }
        case CHAT_AUTH_TRANSACTION_STATE_CANCELLED:
        {
            // Nothing to be done in this case
            break;
        }
        case CHAT_AUTH_TRANSACTION_STATE_DONE:
        {
            pthread_mutex_unlock(&auth_transaction_cblk->mutex);
            pthread_mutex_destroy(&auth_transaction_cblk->mutex);
            generic_deallocator(auth_transaction_cblk);
            return STATUS_SUCCESS;
        }
        default:
        {
            assert(0); // Should never get here
            break;
        }
    }

    pthread_mutex_unlock(&auth_transaction_cblk->mutex);
    return STATUS_SUCCESS;
}
