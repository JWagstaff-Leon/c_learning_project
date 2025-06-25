#include "chat_auth.h"
#include "chat_auth_internal.h"
#include "chat_auth_fsm.h"

#include <assert.h>

#include "sqlite3.h"


static void open_processing(
    sCHAT_AUTH_CBLK*          master_cblk_ptr,
    const sCHAT_AUTH_MESSAGE* message)
{
    eCHAT_AUTH_RESULT auth_result;
    int               sqlite_status;

    sCHAT_AUTH_CBACK_DATA cback_data;

    sCHAT_AUTH_TRANSACTION* auth_transaction;
    sCHAT_USER_CREDENTIALS* credentials_ptr;

    switch (message->type)
    {
        case CHAT_AUTH_MESSAGE_PROCESS_CREDENTIALS:
        {
            auth_transaction = message->params.process_credentials.auth_transaction;
            credentials_ptr  = SP_POINTEE_AS(message->params.process_credentials.credentials_ptr, sCHAT_USER_CREDENTIALS);
            pthread_mutex_lock(&auth_transaction->mutex);

            if (CHAT_AUTH_TRANSACTION_STATE_CANCELLED == auth_transaction->state)
            {
                pthread_mutex_unlock(&auth_transaction->mutex);
                pthread_mutex_destroy(&auth_transaction->mutex);

                shared_ptr_release(auth_transaction->consumer_arg_ptr);
                generic_deallocator(auth_transaction);

                shared_ptr_release(message->params.process_credentials.credentials_ptr);
                break;
            }

            if (NULL == credentials_ptr->username)
            {
                cback_data.auth_result.result           = CHAT_AUTH_RESULT_USERNAME_REQUIRED;
                cback_data.auth_result.consumer_arg_ptr = auth_transaction->consumer_arg_ptr;
                auth_transaction->consumer_arg_ptr      = NULL;

                auth_transaction->state = CHAT_AUTH_TRANSACTION_STATE_DONE;

                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_AUTH_EVENT_AUTH_RESULT,
                                            &cback_data);

                pthread_mutex_unlock(&auth_transaction->mutex);

                shared_ptr_release(message->params.process_credentials.credentials_ptr);
                break;
            }

            auth_result = chat_auth_sql_auth_user(master_cblk_ptr->database,
                                                  credentials_ptr,
                                                  &cback_data.auth_result.user_info);
            assert(CHAT_AUTH_RESULT_FAILURE != auth_result); // REVIEW make this do a retry?
            shared_ptr_release(message->params.process_credentials.credentials_ptr);

            auth_transaction->state = CHAT_AUTH_TRANSACTION_STATE_DONE;

            cback_data.auth_result.result           = auth_result;
            cback_data.auth_result.consumer_arg_ptr = auth_transaction->consumer_arg_ptr;
            auth_transaction->consumer_arg_ptr      = NULL;

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_AUTH_EVENT_AUTH_RESULT,
                                        &cback_data);

            pthread_mutex_unlock(&auth_transaction->mutex);
            break;
        }
        case CHAT_AUTH_MESSAGE_CLOSE:
        {
            master_cblk_ptr->state = CHAT_AUTH_STATE_CLOSED;
            break;
        }
    }
}


static void dispatch_message(
    sCHAT_AUTH_CBLK*          master_cblk_ptr,
    const sCHAT_AUTH_MESSAGE* message)
{
    switch(master_cblk_ptr->state)
    {
        case CHAT_AUTH_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_AUTH_STATE_INIT:
        case CHAT_AUTH_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
            break;
        }
    }
}


static void fsm_cblk_close(
    sCHAT_AUTH_CBLK* master_cblk_ptr)
{
    int sqlite_status;

    sqlite_status = sqlite3_close(master_cblk_ptr->database);
    assert(SQLITE_OK == sqlite_status);
}


void* chat_auth_thread_entry(
    void* arg)
{
    sCHAT_AUTH_CBLK* master_cblk_ptr;

    eSTATUS            status;
    sCHAT_AUTH_MESSAGE message;

    master_cblk_ptr = (sCHAT_AUTH_CBLK*)arg;

    status = chat_auth_sql_init_database(master_cblk_ptr->database);
    assert(STATUS_SUCCESS == status);
    while (CHAT_AUTH_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        dispatch_message(master_cblk_ptr, &message);
    }

    fsm_cblk_close(master_cblk_ptr);
    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                CHAT_AUTH_EVENT_CLOSED,
                                NULL);

    return NULL;
}
