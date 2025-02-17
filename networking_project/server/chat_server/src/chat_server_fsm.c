#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chat_connection.h"
#include "chat_event.h"
#include "common_types.h"
#include "message_queue.h"


static void init_processing(
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE new_message;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_OPEN:
        {
            status = chat_server_network_open(master_cblk_ptr);
            if(STATUS_SUCCESS != status)
            {
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_SERVER_EVENT_OPEN_FAILED,
                                            NULL);
                break;
            }

            master_cblk_ptr->connections.count = 1;

            status = chat_server_network_start_network_watch(master_cblk_ptr);
            if (STATUS_SUCCESS != status)
            {
                chat_server_network_stop_network_watch(master_cblk_ptr);
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_SERVER_EVENT_OPEN_FAILED,
                                            NULL);
                break;
            }

            master_cblk_ptr->state = CHAT_SERVER_STATE_OPEN;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_SERVER_EVENT_OPENED,
                                        NULL);
            break;
        }
        case CHAT_SERVER_MESSAGE_CLOSE:
        {
            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSED;
            break;
        }
        case CHAT_SERVER_MESSAGE_READ_READY:
        case CHAT_SERVER_MESSAGE_WRITE_READY:
        {
            // Expect, but ignore these
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
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE new_message;

    sCHAT_SERVER_CONNECTION* relevant_connection;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_CLOSE:
        {
            chat_server_network_close(&master_cblk_ptr->connections);
            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSED;
            break;
        }
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}


static void dispatch_message(
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
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


static void fsm_cblk_init(
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    eSTATUS                  status;

    assert(NULL != master_cblk_ptr);

    status = chat_connections_create(&master_cblk_ptr->connections,
                                     chat_server_connections_cback,
                                     master_cblk_ptr,
                                     CHAT_SERVER_DEFAULT_MAX_CONNECTIONS);
    assert(STATUS_SUCCESS == status); // FIXME make this a proper, recoverable operation
}


static void fsm_cblk_close(
    sCHAT_SERVER_CBLK* master_cblk_ptr)
{
    eSTATUS                 status;
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    assert(NULL != master_cblk_ptr);

    status = network_watcher_close(master_cblk_ptr->write_network_watcher);
    assert(STATUS_SUCCESS == status);

    status = network_watcher_close(master_cblk_ptr->read_network_watcher);
    assert(STATUS_SUCCESS == status);

    chat_server_network_close(&master_cblk_ptr->connections);

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);

    free(master_cblk_ptr->connections.list);

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    master_cblk_ptr->deallocator(master_cblk_ptr);

    user_cback(user_arg,
               CHAT_SERVER_EVENT_CLOSED,
               NULL);
}


void* chat_server_thread_entry(
    void* arg)
{
    sCHAT_SERVER_CBLK *master_cblk_ptr;
    sCHAT_SERVER_MESSAGE message;
    eSTATUS status;

    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_SERVER_CBLK*)arg;

    fsm_cblk_init(master_cblk_ptr);

    while (CHAT_SERVER_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
        dispatch_message(&message, master_cblk_ptr);
    }

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
