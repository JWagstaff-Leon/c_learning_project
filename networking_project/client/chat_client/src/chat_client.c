#include "chat_client.h"
#include "chat_client_internal.h"
#include "chat_client_fsm.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common_types.h"
#include "message_queue.h"


static void init_cblk(
    sCHAT_CLIENT_CBLK* master_cblk_ptr)
{
    memset(master_cblk_ptr, 0, sizeof(sCHAT_CLIENT_CBLK));

    master_cblk_ptr->state     = CHAT_CLIENT_STATE_CLOSED;
    master_cblk_ptr->server_fd = -1;
}


eSTATUS chat_client_create(
    CHAT_CLIENT*            out_new_client,
    fCHAT_CLIENT_USER_CBACK user_cback,
    void*                   user_arg)
{
    sCHAT_CLIENT_CBLK* new_master_cblk_ptr;
    eSTATUS            status;


    assert(NULL != user_cback);
    assert(NULL != out_new_client);

    new_master_cblk_ptr = (sCHAT_CLIENT_CBLK*)generic_allocator(sizeof(sCHAT_CLIENT_CBLK));
    if (NULL == new_master_cblk_ptr)
    {
        return STATUS_ALLOC_FAILED;
    }

    init_cblk(new_master_cblk_ptr);

    new_master_cblk_ptr->user_cback = user_cback;
    new_master_cblk_ptr->user_arg   = user_arg;

    status = message_queue_create(&new_master_cblk_ptr->message_queue,
                                  CHAT_CLIENT_MSG_QUEUE_SIZE,
                                  sizeof(sCHAT_CLIENT_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        generic_deallocator(new_master_cblk_ptr);
        return status;
    }

    *out_new_client = new_master_cblk_ptr;
    return STATUS_SUCCESS;
}


eSTATUS chat_client_open(
    CHAT_CLIENT client,
    const char* address,
    const char* port)
{
    eSTATUS status;

    sCHAT_CLIENT_CBLK*   master_cblk_ptr;
    sCHAT_CLIENT_MESSAGE message;

    int connection_fd;
    int stdlib_status;

    struct addrinfo hints, *addrinfo;

    if (NULL == client)
    {
        return STATUS_INVALID_ARG;
    }
    if (NULL == address)
    {
        return STATUS_INVALID_ARG;
    }
    if (NULL == port)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_CLIENT_CBLK*)client;
    if (master_cblk_ptr->state > CHAT_CLIENT_STATE_CLOSED)
    {
        return STATUS_INVALID_STATE;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    stdlib_status = getaddrinfo(address, port, &hints, &addrinfo);
    if (0 != stdlib_status)
    {
        return STATUS_FAILURE;
    }

    connection_fd = socket(addrinfo->ai_family, addrinfo->ai_socktype, addrinfo->ai_protocol);
    if (connection_fd < 0)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    stdlib_status = connect(connection_fd, addrinfo->ai_addr, addrinfo->ai_addrlen);
    if (0 != stdlib_status)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    status = chat_connection_create(&master_cblk_ptr->server_connection,
                                    chat_client_connection_cback,
                                    master_cblk_ptr,
                                    connection_fd);
    if(STATUS_SUCCESS != status)
    {
        goto func_exit;
    }

    master_cblk_ptr->server_fd = connection_fd;
    master_cblk_ptr->state     = CHAT_CLIENT_STATE_INIT;

    status = generic_create_thread(chat_client_thread_entry,
                                   master_cblk_ptr,
                                   NULL);
    if (STATUS_SUCCESS != status)
    {
        chat_connection_destroy(master_cblk_ptr->server_connection);
        goto func_exit;
    }
    
    status = STATUS_SUCCESS;

func_exit:
    freeaddrinfo(addrinfo);
    return status;
}


eSTATUS chat_client_user_input(
    CHAT_CLIENT client,
    const char* text)
{
    sCHAT_CLIENT_CBLK*   master_cblk_ptr;
    sCHAT_CLIENT_MESSAGE message;

    eSTATUS status;

    assert(NULL != client);
    assert(NULL != text);

    master_cblk_ptr = (sCHAT_CLIENT_CBLK*)client;

    message.type = CHAT_CLIENT_MESSAGE_USER_INPUT;

    status = print_string_to_buffer(&message.params.user_input.text[0],
                                    text,
                                    sizeof(message.params.user_input.text),
                                    NULL);
    if(STATUS_SUCCESS != status)
    {
        return STATUS_INVALID_ARG;
    }

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}


eSTATUS chat_client_close(
    CHAT_CLIENT client)
{
    sCHAT_CLIENT_CBLK*   master_cblk_ptr;
    sCHAT_CLIENT_MESSAGE message;

    eSTATUS status;

    assert(NULL != client);
    master_cblk_ptr = (sCHAT_CLIENT_CBLK*)client;

    message.type = CHAT_CLIENT_MESSAGE_CLOSE;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}
