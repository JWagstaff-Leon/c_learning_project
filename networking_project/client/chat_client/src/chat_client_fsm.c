#include "chat_client_fsm.h"

#include <assert.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
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


static void subfsm_send_processing(
    const sCHAT_CLIENT_MESSAGE* message,
    sCHAT_CLIENT_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;
    sCHAT_EVENT*         outgoing_event;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    outgoing_event = &master_cblk_ptr->outgoing_event_writer.event;

    switch (master_cblk_ptr->send_state)
    {
        case CHAT_CLIENT_SUBFSM_SEND_STATE_READY:
        {
            outgoing_event->type   = message->params.send.event_type;
            outgoing_event->origin = 0;
            outgoing_event->length = strlen(message->params.send_text.text) + 1;
            snprintf(&outgoing_event->data[0],
                     sizeof(outgoing_event->data),
                     "%s",
                     message->params.send_text.text);

            master_cblk_ptr->send_state = CHAT_CLIENT_SUBFSM_SEND_STATE_IN_PROGRESS;
            // Fallthrough
        }
        case CHAT_CLIENT_SUBFSM_SEND_STATE_IN_PROGRESS:
        {
            if (master_cblk_ptr->server_connection.revents & POLLOUT)
            {
                status = chat_event_io_write_to_fd(master_cblk_ptr->outgoing_event_writer,
                                                master_cblk_ptr->server_connection.fd);
                if (STATUS_INCOMPLETE != status)
                {
                    master_cblk_ptr->send_state = CHAT_CLIENT_SUBFSM_SEND_STATE_READY;
                }
            }
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

    switch (message->type)
    {
        case CHAT_CLIENT_MESSAGE_SEND:
        {
            memcpy(&new_message, message, sizeof(new_message));
            new_message.params.send.event_type = CHAT_EVENT_USERNAME_SUBMIT;
            subfsm_send_processing(&new_message, master_cblk_ptr);
            break;
        }
        case CHAT_CLIENT_MESSAGE_POLL:
        {
            status = chat_client_network_poll(master_cblk_ptr);
            assert(STATUS_SUCCESS == status);
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
        case CHAT_CLIENT_MESSAGE_SEND:
        {
            memcpy(&new_message, message, sizeof(new_message));
            new_message.params.send.event_type = CHAT_EVENT_CHAT_MESSAGE;
            subfsm_send_processing(&new_message, master_cblk_ptr);
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
