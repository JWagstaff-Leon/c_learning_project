#include "chat_client.h"
#include "chat_client_internal.h"
#include "chat_client_fsm.h"

#include <assert.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "chat_event_io.h"
#include "common_types.h"


static void init_processing(
    sCHAT_CLIENT_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENT_MESSAGE* message)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    eCHAT_EVENT_TYPE event_type;

    switch (message->type)
    {
        case CHAT_CLIENT_MESSAGE_INCOMING_EVENT:
        {
            chat_client_handle_incoming_event(master_cblk_ptr,
                                              &message->params.incoming_event.event);
            break;
        }
        case CHAT_CLIENT_MESSAGE_CONNECTION_CLOSED:
        {
            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
            break;
        }
        case CHAT_CLIENT_MESSAGE_CLOSE:
        {
            status = chat_connection_queue_new_event(master_cblk_ptr->server_connection,
                                                     CHAT_EVENT_USER_LEAVE,
                                                     master_cblk_ptr->user_info,
                                                     "");
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_DISCONNECTING;
            break;
        }
    }
}


static void auth_entry_processing(
    sCHAT_CLIENT_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENT_MESSAGE* message)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    eCHAT_EVENT_TYPE event_type;

    switch (message->type)
    {
        case CHAT_CLIENT_MESSAGE_USER_INPUT:
        {
            switch (master_cblk_ptr->state)
            {
                case CHAT_CLIENT_STATE_USERNAME_ENTRY:
                {
                    event_type = CHAT_EVENT_USERNAME_SUBMIT;
                    break;
                }
                case CHAT_CLIENT_STATE_PASSWORD_ENTRY:
                {
                    event_type = CHAT_EVENT_PASSWORD_SUBMIT;
                    break;
                }
                default:
                {
                    assert(0);
                    break;
                }
            }
            status = chat_connection_queue_new_event(master_cblk_ptr->server_connection,
                                                     event_type,
                                                     master_cblk_ptr->user_info,
                                                     message->params.user_input.text);
            assert(STATUS_SUCCESS == status);

            switch (master_cblk_ptr->state)
            {
                case CHAT_CLIENT_STATE_USERNAME_ENTRY:
                {
                    master_cblk_ptr->state = CHAT_CLIENT_STATE_USERNAME_VERIFYING;
                    break;
                }
                case CHAT_CLIENT_STATE_PASSWORD_ENTRY:
                {
                    master_cblk_ptr->state = CHAT_CLIENT_STATE_PASSWORD_VERIFYING;
                    break;
                }
                default:
                {
                    assert(0);
                    break;
                }
            }

            break;
        }
        case CHAT_CLIENT_MESSAGE_INCOMING_EVENT:
        {
            chat_client_handle_incoming_event(master_cblk_ptr,
                                              &message->params.incoming_event.event);
            break;
        }
        case CHAT_CLIENT_MESSAGE_CONNECTION_CLOSED:
        {
            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
            break;
        }
        case CHAT_CLIENT_MESSAGE_CLOSE:
        {
            status = chat_connection_queue_new_event(master_cblk_ptr->server_connection,
                                                     CHAT_EVENT_USER_LEAVE,
                                                     master_cblk_ptr->user_info,
                                                     "");
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_DISCONNECTING;
            break;
        }
    }
}


static void auth_verifying_processing(
    sCHAT_CLIENT_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENT_MESSAGE* message)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    switch (message->type)
    {
        case CHAT_CLIENT_MESSAGE_INCOMING_EVENT:
        {
            chat_client_handle_incoming_event(master_cblk_ptr,
                                              &message->params.incoming_event.event);
            break;
        }
        case CHAT_CLIENT_MESSAGE_CONNECTION_CLOSED:
        {
            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
            break;
        }
        case CHAT_CLIENT_MESSAGE_CLOSE:
        {
            status = chat_connection_queue_new_event(master_cblk_ptr->server_connection,
                                                     CHAT_EVENT_USER_LEAVE,
                                                     master_cblk_ptr->user_info,
                                                     "");
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_DISCONNECTING;
            break;
        }
    }
}


static void active_processing(
    sCHAT_CLIENT_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENT_MESSAGE* message)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    switch (message->type)
    {
        case CHAT_CLIENT_MESSAGE_USER_INPUT:
        {
            status = chat_connection_queue_new_event(master_cblk_ptr->server_connection,
                                                     CHAT_EVENT_CHAT_MESSAGE,
                                                     master_cblk_ptr->user_info,
                                                     message->params.user_input.text);
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CHAT_CLIENT_MESSAGE_INCOMING_EVENT:
        {
            chat_client_handle_incoming_event(master_cblk_ptr,
                                              &message->params.incoming_event.event);
            break;
        }
        case CHAT_CLIENT_MESSAGE_CONNECTION_CLOSED:
        {
            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
            break;
        }
        case CHAT_CLIENT_MESSAGE_CLOSE:
        {
            status = chat_connection_queue_new_event(master_cblk_ptr->server_connection,
                                                     CHAT_EVENT_USER_LEAVE,
                                                     master_cblk_ptr->user_info,
                                                     "");
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_DISCONNECTING;
            break;
        }
    }
}


static void disconnecting_processing(
    sCHAT_CLIENT_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENT_MESSAGE* message)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_CLIENT_MESSAGE_CONNECTION_CLOSED:
        {
            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
            break;
        }
    }
}


static void dispatch_message(
    sCHAT_CLIENT_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENT_MESSAGE* message)
{
    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (master_cblk_ptr->state)
    {
        case CHAT_CLIENT_STATE_INIT:
        {
            init_processing(master_cblk_ptr, message);
        }
        case CHAT_CLIENT_STATE_USERNAME_ENTRY:
        case CHAT_CLIENT_STATE_PASSWORD_ENTRY:
        {
            auth_entry_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CLIENT_STATE_USERNAME_VERIFYING:
        case CHAT_CLIENT_STATE_PASSWORD_VERIFYING:
        {
            auth_verifying_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CLIENT_STATE_ACTIVE:
        {
            active_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CLIENT_STATE_DISCONNECTING:
        {
            disconnecting_processing(master_cblk_ptr, message);
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


static void fsm_cblk_close(
    sCHAT_CLIENT_CBLK* master_cblk_ptr)
{
    // FIXME this should only deallocate the children of this cblk; the destory of this module will deallocate it
    fCHAT_CLIENT_USER_CBACK user_cback;
    void*                   user_arg;

    assert(NULL != master_cblk_ptr);

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    generic_deallocator(master_cblk_ptr);

    user_cback(user_arg,
               CHAT_CLIENT_EVENT_CLOSED,
               NULL);
}


void* chat_client_thread_entry(
    void* arg)
{
    sCHAT_CLIENT_CBLK *master_cblk_ptr;
    sCHAT_CLIENT_MESSAGE message;
    eSTATUS status;


    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_CLIENT_CBLK*)arg;

    while (CHAT_CLIENT_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        dispatch_message(master_cblk_ptr, &message);
    }

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
