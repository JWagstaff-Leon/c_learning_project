#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "chat_clients.h"
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


static eSTATUS open_listen_socket(
    int* out_fd)
{
    eSTATUS status;

    int listen_fd;
    int std_lib_status;
    int opt_value = 1;

    struct sockaddr_in address;
    
    assert(NULL != master_cblk_ptr);

    listen_fd = socket(AF_INET,
                       SOCK_STREAM,
                       0);
    if (listen_fd < 0)
    {
        return STATUS_FAILURE;
    }

    std_lib_status = setsockopt(listen_fd,
                                SOL_SOCKET,
                                SO_REUSEADDR | SO_REUSEPORT,
                                &opt_value,
                                sizeof(opt_value));
    if (0 != std_lib_status)
    {
        status = STATUS_FAILURE;
        goto fail_post_open_socket;
    }

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(8080);
    memset(address.sin_zero, 0, sizeof(address.sin_zero));

    std_lib_status = bind(listen_fd,
                          (struct sockaddr *)&address,
                          sizeof(address));
    if (std_lib_status < 0)
    {
        status = STATUS_FAILURE;
        goto fail_post_open_socket;
    }

    std_lib_status = listen(listen_fd,
                            16); // REVIEW change this to something dynamic?
    if(std_lib_status < 0)
    {
        status = STATUS_FAILURE;
        goto fail_post_open_socket;
    }

    status = STATUS_SUCCESS;
    goto success;

fail_post_open_socket:
    close(listen_socket_fd);

success:
    return status;
}


static eSTATUS fsm_cblk_init(
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    eSTATUS status;

    status = open_listen_socket(&master_cblk_ptr->listen_fd);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }
    
    status = network_watcher_start_watch(master_cblk_ptr->connection_listener,
                                         NETWORK_WATCHER_MODE_READ,
                                         master_cblk_ptr->listen_fd);
    if (STATUS_SUCCESS != status)
    {
        close(master_cblk_ptr->listen_fd);
        master_cblk_ptr->listen_fd = -1;
        return status;
    }

    return status;
}


static void fsm_cblk_close(
    sCHAT_SERVER_CBLK* master_cblk_ptr)
{
    eSTATUS                 status;
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    assert(NULL != master_cblk_ptr);

    status = network_watcher_close(master_cblk_ptr->connection_listener);
    assert(STATUS_SUCCESS == status);
    
    close(master_cblk_ptr->listen_fd);

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);

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

    status = fsm_cblk_init(master_cblk_ptr);
    assert(STATUS_SUCCESS == status);

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
