#include "chat_client_fsm.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>

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
            status = chat_client_network_open(master_cblk_ptr,
                                              message->params.connect.address);
            if (STATUS_SUCCESS != status)
            {
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_CLIENT_EVENT_CONNECT_FAILED,
                                            NULL);
                break;
            }

            master_cblk_ptr->state = CHAT_CLIENT_STATE_INACTIVE;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_CONNECTED,
                                        NULL);
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
            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_CLOSED,
                                        NULL);
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
        case CHAT_CLIENT_MESSAGE_SEND_TEXT:
        {
            chat_client_network_send_username(master_cblk_ptr->server_connection.fd,
                                              &message->params.send_text.text[0]);
            // TODO transiton states or substates to a "waiting for username acceptance"
            // TODO whatever new (sub)state will call back to the user with the result when determined
            break;
        }
        case CHAT_CLIENT_MESSAGE_DISCONNECT:
        {
            // TODO do disconnect
            master_cblk_ptr->state = CHAT_CLIENT_STATE_NOT_CONNECTED;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_DISCONNECTED,
                                        NULL);
            break;
        }
        case CHAT_CLIENT_MESSAGE_CLOSE:
        {
            // TODO do disconnect
            // TODO do close
            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_CLOSED,
                                        NULL);
            break;
        }
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
        case CHAT_CLIENT_MESSAGE_SEND_TEXT:
        {
            status = chat_client_network_send_message(master_cblk_ptr->server_connection.fd,
                                                      &message->params.send_text.text[0]);
            break;
        }
        case CHAT_CLIENT_MESSAGE_POLL:
        {
            break;
        }
        case CHAT_CLIENT_MESSAGE_DISCONNECT:
        {
            // TODO do disconnect
            master_cblk_ptr->state = CHAT_CLIENT_STATE_NOT_CONNECTED;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_DISCONNECTED,
                                        NULL);
            break;
        }
        case CHAT_CLIENT_MESSAGE_CLOSE:
        {
            // TODO do disconnect
            // TODO do close
            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_CLOSED,
                                        NULL);
            break;
        }
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
