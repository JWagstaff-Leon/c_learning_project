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
            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
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
            outgoing_event->type   = message->params.send_new.event_type;
            outgoing_event->origin = 0;
            outgoing_event->length = strlen(message->params.send_new.text) + 1;
            snprintf(&outgoing_event->data[0],
                     sizeof(outgoing_event->data),
                     "%s",
                     message->params.send_new.text);

            master_cblk_ptr->send_state = CHAT_CLIENT_SUBFSM_SEND_STATE_IN_PROGRESS;
            // Fallthrough
        }
        case CHAT_CLIENT_SUBFSM_SEND_STATE_IN_PROGRESS:
        {
            if (master_cblk_ptr->server_connection.revents & POLLOUT)
            {
                status = chat_event_io_write_to_fd(&master_cblk_ptr->outgoing_event_writer,
                                                   master_cblk_ptr->server_connection.fd);
                if (STATUS_INCOMPLETE != status)
                {
                    master_cblk_ptr->send_state = CHAT_CLIENT_SUBFSM_SEND_STATE_READY;
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_CLIENT_EVENT_SEND_FINISHED,
                                                NULL);
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
        case CHAT_CLIENT_MESSAGE_SEND_NEW:
        {
            if (CHAT_CLIENT_SUBFSM_SEND_STATE_IN_PROGRESS == master_cblk_ptr->send_state)
            {
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_CLIENT_EVENT_SEND_REJECTED,
                                            NULL);
                break;
            }
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_SEND_STARTED,
                                        NULL);

            memcpy(&new_message, message, sizeof(new_message));
            new_message.params.send_new.event_type = CHAT_EVENT_USERNAME_SUBMIT;
            // Fallthrough
        }
        case CHAT_CLIENT_MESSAGE_SEND_CONTINUE:
        {
            subfsm_send_processing(&new_message, master_cblk_ptr);
            break;
        }
        case CHAT_CLIENT_MESSAGE_POLL:
        {
            status = chat_client_network_poll(master_cblk_ptr);
            assert(STATUS_SUCCESS == status);

            new_message.type = CHAT_CLIENT_MESSAGE_POLL;
            status           = message_queue_put(master_cblk_ptr->message_queue,
                                                 &new_message,
                                                 sizeof(new_message));
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CHAT_CLIENT_MESSAGE_DISCONNECT:
        {
            status = chat_client_network_disconnect(master_cblk_ptr);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_NOT_CONNECTED;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_DISCONNECTED,
                                        NULL);
            break;
        }
        case CHAT_CLIENT_MESSAGE_CLOSE:
        {
            status = chat_client_network_disconnect(master_cblk_ptr);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_CLOSED;
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
        case CHAT_CLIENT_MESSAGE_SEND_NEW:
        {
            memcpy(&new_message, message, sizeof(new_message));
            new_message.params.send_new.event_type = CHAT_EVENT_CHAT_MESSAGE;
            subfsm_send_processing(&new_message, master_cblk_ptr);
            break;
        }
        case CHAT_CLIENT_MESSAGE_POLL:
        {
            status = chat_client_network_poll(master_cblk_ptr);
            assert(STATUS_SUCCESS == status);

            new_message.type = CHAT_CLIENT_MESSAGE_POLL;
            status           = message_queue_put(master_cblk_ptr->message_queue,
                                                 &new_message,
                                                 sizeof(new_message));
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CHAT_CLIENT_MESSAGE_DISCONNECT:
        {
            status = chat_client_network_disconnect(master_cblk_ptr);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_NOT_CONNECTED;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_DISCONNECTED,
                                        NULL);
            break;
        }
        case CHAT_CLIENT_MESSAGE_CLOSE:
        {
            status = chat_client_network_disconnect(master_cblk_ptr);
            assert(STATUS_SUCCESS == status);

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
    fCHAT_CLIENT_USER_CBACK user_cback;
    void*                   user_arg;

    assert(NULL != master_cblk_ptr);

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    master_cblk_ptr->deallocator(master_cblk_ptr);
    
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
    
    // FIXME use the new chat_event_io_create
    // TODO finish this; add asserts
    status = chat_client_io_create_reader(&master_cblk_ptr->event_reader,
                                          master_cblk_ptr->io_params,
                                          NULL,
                                          master_cblk_ptr);
    status = chat_client_io_create_writer(&master_cblk_ptr->event_writer,
                                          master_cblk_ptr->io_params,
                                          NULL,
                                          master_cblk_ptr);

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
