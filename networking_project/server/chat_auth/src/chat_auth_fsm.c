#include "chat_auth.h"
#include "chat_auth_internal.h"
#include "chat_auth_fsm.h"

#include <assert.h>

#include "sqlite3.h"


static void no_database_processing(
    sCHAT_AUTH_CBLK*          master_cblk_ptr,
    const sCHAT_AUTH_MESSAGE* message)
{
    eSTATUS status;
    int     sqlite_status;

    switch (message->type)
    {
        case CHAT_AUTH_MESSAGE_OPEN_DATABASE:
        {
            sqlite_status = sqlite3_open_v2(message->params.open_database.path,
                                            &master_cblk_ptr->database,
                                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                            NULL);
            switch (sqlite_status)
            {
                case SQLITE_OK:
                {
                    master_cblk_ptr->state = CHAT_AUTH_STATE_OPEN;

                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_AUTH_EVENT_DATABASE_OPENED,
                                                NULL);
                    break;
                }
                default:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_AUTH_EVENT_DATABASE_OPEN_FAILED,
                                                NULL);
                    break;
                }
            }

            status = chat_auth_init_database(master_cblk_ptr->database);
            assert(STATUS_SUCCESS == status);
            
            break;
        }
        case CHAT_AUTH_MESSAGE_SHUTDOWN:
        {
            master_cblk_ptr->state = CHAT_AUTH_STATE_CLOSED;
            break;
        }
    }
}


static void open_processing(
    sCHAT_AUTH_CBLK*          master_cblk_ptr,
    const sCHAT_AUTH_MESSAGE* message)
{
    eSTATUS status;
    int     sqlite_status;

    sCHAT_AUTH_CBACK_DATA cback_data;

    switch (message->type)
    {
        case CHAT_AUTH_MESSAGE_PROCESS_CREDENTIALS:
        {
            // TODO check for finalization of auth_object; don't process if it's dead
            if (NULL == message->params.process_credentials.credentials.username)
            {
                cback_data.auth_result.result      = CHAT_AUTH_RESULT_USERNAME_REQUIRED;
                cback_data.auth_result.auth_object = message->params.process_credentials.auth_object;

                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_AUTH_EVENT_AUTH_RESULT,
                                            &cback_data);
                break;
            }

            status = chat_auth_sql_auth_user(master_cblk_ptr->database,
                                             message->params.process_credentials.credentials,
                                             &cback_data.auth_result.user_info);
            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    cback_data.auth_result.result = CHAT_AUTH_RESULT_AUTHENTICATED;
                    break;
                }
                case STATUS_INCOMPLETE:
                {
                    cback_data.auth_result.result = CHAT_AUTH_RESULT_PASSWORD_REJECTED;
                    break;
                }
                case STATUS_NOT_FOUND:
                {
                    cback_data.auth_result.result = CHAT_AUTH_RESULT_PASSWORD_CREATION;
                    break;
                }
                case STATUS_EMPTY:
                {
                    cback_data.auth_result.result = CHAT_AUTH_RESULT_PASSWORD_REQUIRED;
                    break;
                }
                default:
                {
                    // Should not get here
                    assert(0);
                }
            }

            cback_data.auth_result.auth_object = message->params.process_credentials.auth_object;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_AUTH_EVENT_AUTH_RESULT,
                                        &cback_data);
            
            break;
        }
        case CHAT_AUTH_MESSAGE_CLOSE_DATABASE:
        {
            sqlite_status = sqlite3_close_v2(master_cblk_ptr->database);
            switch (sqlite_status)
            {
                case SQLITE_OK:
                {
                    master_cblk_ptr->database = NULL;
                    master_cblk_ptr->state = CHAT_AUTH_STATE_NO_DATABASE;

                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_AUTH_EVENT_DATABASE_CLOSED,
                                                NULL);
                    break;
                }
                default:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_AUTH_EVENT_DATABASE_CLOSE_FAILED,
                                                NULL);
                    break;
                }
            }
        }
        case CHAT_AUTH_MESSAGE_SHUTDOWN:
        {
            sqlite_status = sqlite3_close_v2(master_cblk_ptr->database);
            switch (sqlite_status)
            {
                case SQLITE_OK:
                {
                    master_cblk_ptr->database = NULL;
                    master_cblk_ptr->state = CHAT_AUTH_STATE_CLOSED;

                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_AUTH_EVENT_DATABASE_CLOSED,
                                                NULL);
                    break;
                }
                default:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_AUTH_EVENT_DATABASE_CLOSE_FAILED,
                                                NULL);
                    break;
                }
            }
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
        case CHAT_AUTH_STATE_NO_DATABASE:
        {
            no_database_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_AUTH_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_AUTH_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
            break;
        }
    }
}


static void message_cleanup(
    const sCHAT_AUTH_MESSAGE* message)
{
    switch (message->type)
    {
        case CHAT_AUTH_MESSAGE_OPEN_DATABASE:
        {
            generic_deallocator(message->params.open_database.path);
            break;
        }
    }
}


void* chat_auth_thread_entry(
    void* arg)
{
    sCHAT_AUTH_CBLK* master_cblk_ptr;

    eSTATUS            status;
    sCHAT_AUTH_MESSAGE message;

    master_cblk_ptr = (sCHAT_AUTH_CBLK*)arg;

    while (CHAT_AUTH_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        dispatch_message(master_cblk_ptr, &message);
        message_cleanup(&message);
    }
}
