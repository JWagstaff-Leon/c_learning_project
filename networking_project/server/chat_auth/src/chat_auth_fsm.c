#include "chat_auth.h"
#include "chat_auth_internal.h"
#include "chat_auth_fsm.h"

#include <assert.h>


static void open_processing(
    sCHAT_AUTH_CBLK*          master_cblk_ptr,
    const sCHAT_AUTH_MESSAGE* message)
{
    switch (message->type)
    {
        case CHAT_AUTH_MESSAGE_PROCESS_CREDENTIALS:
        {
            // TODO do the auth step
            break;
        }
        case CHAT_AUTH_MESSAGE_SHUTDOWN:
        {
            // TODO switch to closed state
            break;
        }
    }
}


static void closed_processing(
    sCHAT_AUTH_CBLK*          master_cblk_ptr,
    const sCHAT_AUTH_MESSAGE* message)
{
    switch (message->type)
    {
        case CHAT_AUTH_MESSAGE_OPEN:
        {
            // TODO open up the auth
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
        case CHAT_AUTH_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
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
    }
}
