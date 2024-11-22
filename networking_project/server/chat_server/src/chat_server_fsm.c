#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <stdlib.h>


static void init_processing(
    sCHAT_SERVER_MESSAGE *message,
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    eSTATUS status;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_OPEN:
        {
            status = chat_server_do_open(master_cblk_ptr);
            if(STATUS_SUCCESS != status)
            {
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_SERVER_EVENT_OPEN_FAILED,
                                            NULL);
                break;
            }

            master_cblk_ptr->state = CHAT_SERVER_STATE_OPEN;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_SERVER_EVENT_OPENED,
                                        NULL);
            // TODO enqueue poll message to start poll loop
            break;
        }
        case CHAT_SERVER_MESSAGE_RESET:
        {
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_SERVER_EVENT_RESET,
                                        NULL);
            break;
        }
        case CHAT_SERVER_MESSAGE_CLOSE:
        {
            chat_server_do_close(master_cblk_ptr);

            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSED;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_SERVER_EVENT_CLOSED,
                                        NULL);
            break;
        }
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}


static void open_processing(
    sCHAT_SERVER_MESSAGE *message,
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    eSTATUS status;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_POLL:
        {
            status = chat_server_do_poll(&master_cblk_ptr->connections);
            assert(STATUS_SUCCESS == status);

            status = chat_server_process_connection_events(&master_cblk_ptr->connections);
            // TODO enqueue new poll
            break;
        }
        case CHAT_SERVER_MESSAGE_RESET:
        {
            chat_server_start_reset(master_cblk_ptr);

            master_cblk_ptr->state = CHAT_SERVER_STATE_INIT;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_SERVER_EVENT_RESET,
                                        NULL);
            break;
        }
        case CHAT_SERVER_MESSAGE_CLOSE:
        {
            chat_server_do_close(master_cblk_ptr);

            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSED;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_SERVER_EVENT_CLOSED,
                                        NULL);
            break;
        }
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}


static dispatch_message(
    const sCHAT_SERVER_MESSAGE *message,
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (master_cblk_ptr->state)
    {
        case CHAT_SERVER_STATE_INIT:
        {
            init_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_SERVER_STATE_OPEN:
        {
            open_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_SERVER_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
            break;
        }
    }
}

void* chat_server_process_thread_entry(
    void *arg)
{
    sCHAT_SERVER_CBLK *master_cblk_ptr;
    sCHAT_SERVER_MESSAGE message;

    sCHAT_SERVER_CONNECTIONS *connections = calloc(8, sizeof(sCHAT_SERVER_CONNECTION));
    if (NULL == connections)
    {
        return NULL;
    }

    master_cblk_ptr = (sCHAT_SERVER_CBLK *)arg;
    assert(NULL != master_cblk_ptr);

    master_cblk_ptr->connections

    while (CHAT_SERVER_STATE_CLOSED != master_cblk_ptr->state)
    {
        // TODO
        // Block on getting a message from the message queue
        // Get the message
        
        dispatch_message(&message, master_cblk_ptr);
    }

    return NULL;
}
