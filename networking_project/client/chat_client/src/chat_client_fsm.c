#include "chat_client_fsm.h"

#include <assert.h>
#include <stddef.h>

#include "common_types.h"
#include "chat_client_internal.h"


static void not_connected_processing(
    const sCHAT_CLIENT_MESSAGE* message,
    sCHAT_CLIENT_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_CLIENT_MESSAGE_CONNECT:
        {
            // TODO do the connection
            break;
        }
        case CHAT_CLIENT_MESSAGE_DISCONNECT:
        {
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_DISCONNECTED,
                                        NULL);
            break;
        }
        case CHAT_CLIENT_MESSAGE_CLOSE:
        {
            // TODO do close
            break;
        }
    }
}


static void inactive_processing(
    const sCHAT_CLIENT_MESSAGE* message,
    sCHAT_CLIENT_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    // REVIEW maybe do a subfsm for this?
    switch (message->type)
    {
        
    }
}


static void active_processing(
    const sCHAT_CLIENT_MESSAGE* message,
    sCHAT_CLIENT_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        
    }
}


static void disconnecting_processing(
    const sCHAT_CLIENT_MESSAGE* message,
    sCHAT_CLIENT_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {

    }
}


static void dispatch_message(
    const sCHAT_CLIENT_MESSAGE* message,
    sCHAT_CLIENT_CBLK*          master_cblk_ptr)
{
    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (master_cblk_ptr->state)
    {
        case CHAT_CLIENT_STATE_NOT_CONNECTED:
        {
            not_connected_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_CLIENT_STATE_INACTIVE:
        {
            inactive_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_CLIENT_STATE_ACTIVE:
        {
            active_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_CLIENT_STATE_DISCONNECTING:
        {
            disconnecting_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_CLIENT_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
            break;
        }
    }
}


void* chat_client_thread_entry(
    void* arg)
{
    sCHAT_CLIENT_CBLK *master_cblk_ptr;
    sCHAT_CLIENT_MESSAGE message;
    eSTATUS status;

    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_CLIENT_CBLK*)arg;

    fsm_cblk_init(master_cblk_ptr);

    while (CHAT_CLIENT_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
        dispatch_message(&message, master_cblk_ptr);
    }

    master_cblk_ptr->deallocator(master_cblk_ptr);
    return NULL;
}
