#include "chat_auth.h"
#include "chat_auth_internal.h"
#include "chat_auth_fsm.h"

#include <assert.h>


static void no_database_processing(
    sCHAT_AUTH_CBLK*          master_cblk_ptr,
    const sCHAT_AUTH_MESSAGE* message)
{
    eSTATUS status;

    switch (message->type)
    {
        case CHAT_AUTH_MESSAGE_OPEN_DATABASE:
        {
            status = chat_auth_read_database_file(master_cblk_ptr,
                                                  message->params.open_database.path);
            // TODO callback and change states based on status
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

    switch (message->type)
    {
        case CHAT_AUTH_MESSAGE_PROCESS_CREDENTIALS:
        {
            // TODO do the auth step
            break;
        }
        case CHAT_AUTH_MESSAGE_CLOSE_DATABASE:
        {
            status = chat_auth_save_database_file(master_cblk_ptr,
                                                  message->params.close_database.path);
            // TODO callback and change states based on status
        }
        case CHAT_AUTH_MESSAGE_SHUTDOWN:
        {
            // TODO switch to closed state
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
        case CHAT_AUTH_MESSAGE_CLOSE_DATABASE:
        {
            generic_deallocator(message->params.close_database.path);
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
