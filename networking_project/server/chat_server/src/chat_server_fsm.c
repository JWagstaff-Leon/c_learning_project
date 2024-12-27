#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <stdlib.h>

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
            status = chat_server_network_open(&master_cblk_ptr->connections.list[0]);
            if(STATUS_SUCCESS != status)
            {
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CHAT_SERVER_EVENT_OPEN_FAILED,
                                            NULL);
                break;
            }

            master_cblk_ptr->connections.count = 1;

            master_cblk_ptr->state = CHAT_SERVER_STATE_OPEN;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_SERVER_EVENT_OPENED,
                                        NULL);

            new_message.type = CHAT_SERVER_MESSAGE_POLL;
            status           = message_queue_put(master_cblk_ptr->message_queue,
                                                 &new_message,
                                                 sizeof(new_message));
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_SERVER_MESSAGE_CLOSE:
        {
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


static void open_processing(
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE new_message;

    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_POLL:
        {
            status = chat_server_network_poll(&master_cblk_ptr->connections);
            assert(STATUS_SUCCESS == status);

            status = chat_server_process_connections_events(&master_cblk_ptr->connections,
                                                            master_cblk_ptr->allocator);
            assert(STATUS_SUCCESS == status);

            new_message.type = CHAT_SERVER_MESSAGE_POLL;
            status           = message_queue_put(master_cblk_ptr->message_queue,
                                                 &new_message,
                                                 sizeof(new_message));
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_SERVER_MESSAGE_CLOSE:
        {
            chat_server_network_close(&master_cblk_ptr->connections);

            status = message_queue_destroy(master_cblk_ptr->message_queue);
            assert(STATUS_SUCCESS == status);

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
    uint32_t                 connection_index;
    sCHAT_SERVER_CONNECTION* connections_list;
    assert(NULL != master_cblk_ptr);

    connections_list = master_cblk_ptr->allocator(CHAT_SERVER_MAX_CONNECTIONS * sizeof(sCHAT_SERVER_CONNECTION));
    assert(NULL != connections_list);

    master_cblk_ptr->connections.list  = connections_list;
    master_cblk_ptr->connections.count = 0;
    master_cblk_ptr->connections.size  = 8;

    for (connection_index = 0;
         connection_index < master_cblk_ptr->connections.size;
         connection_index++)
    {
        master_cblk_ptr->connections.list[connection_index] = k_blank_user;
    }
}


static void fsm_cblk_close(
    sCHAT_SERVER_CBLK* master_cblk_ptr)
{
    eSTATUS status;
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    assert(NULL != master_cblk_ptr);

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
