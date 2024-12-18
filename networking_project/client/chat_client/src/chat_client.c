#include "chat_client.h"
#include "chat_client_internal.h"
#include "chat_client_fsm.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


static eSTATUS init_cblk(
    sCHAT_CLIENT_CBLK *master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);

    memset(master_cblk_ptr, 0, sizeof(master_cblk_ptr));
    master_cblk_ptr->state                = CHAT_CLIENT_STATE_NOT_CONNECTED;
    master_cblk_ptr->server_connection.fd = -1;
    master_cblk_ptr->send_state           = CHAT_CLIENT_SUBFSM_SEND_STATE_READY;

    return STATUS_SUCCESS;
}


static eSTATUS init_msg_queue(
    MESSAGE_QUEUE*       message_queue_ptr,
    fGENERIC_ALLOCATOR   allocator,
    fGENERIC_DEALLOCATOR deallocator)
{
    eSTATUS status;

    assert(NULL != message_queue_ptr);
    assert(NULL != allocator);
    assert(NULL != deallocator);

    status = message_queue_create(message_queue_ptr,
                                  CHAT_CLIENT_MSG_QUEUE_SIZE,
                                  sizeof(sCHAT_CLIENT_MESSAGE),
                                  allocator,
                                  deallocator);

    return status;
}


eSTATUS chat_client_create(
    CHAT_CLIENT*            out_new_chat_client,
    fGENERIC_ALLOCATOR      allocator,
    fGENERIC_DEALLOCATOR    deallocator,
    fGENERIC_THREAD_CREATOR create_thread,
    fCHAT_CLIENT_USER_CBACK user_cback,
    void*                   user_arg)
{
    sCHAT_CLIENT_CBLK* new_master_cblk_ptr;
    eSTATUS            status;

    assert(NULL != allocator);
    assert(NULL != deallocator);
    assert(NULL != user_cback);
    assert(NULL != out_new_chat_client);

    new_master_cblk_ptr = (sCHAT_CLIENT_CBLK*)allocator(sizeof(sCHAT_CLIENT_CBLK));
    if (NULL == new_master_cblk_ptr)
    {
        return STATUS_ALLOC_FAILED;
    }

    status = init_cblk(new_master_cblk_ptr);
    if (STATUS_SUCCESS != status)
    {
        deallocator(new_master_cblk_ptr);
        return status;
    }

    new_master_cblk_ptr->allocator   = allocator;
    new_master_cblk_ptr->deallocator = deallocator;
    new_master_cblk_ptr->user_cback  = user_cback;
    new_master_cblk_ptr->user_arg    = user_arg;

    status = init_msg_queue(&new_master_cblk_ptr->message_queue,
                            allocator,
                            deallocator);
    if (STATUS_SUCCESS != status)
    {
        deallocator(new_master_cblk_ptr);
        return status;
    }

    status = create_thread(chat_client_thread_entry, new_master_cblk_ptr);
    if (STATUS_SUCCESS != status)
    {
        message_queue_destroy(new_master_cblk_ptr->message_queue);
        deallocator(new_master_cblk_ptr);
        return status;
    }

    *out_new_chat_client = new_master_cblk_ptr;
    return STATUS_SUCCESS;
}


eSTATUS chat_client_connect(
    CHAT_CLIENT client,
    const char* address,
    uint16_t    port)
{
    sCHAT_CLIENT_CBLK*   master_cblk_ptr;
    sCHAT_CLIENT_MESSAGE message;

    eSTATUS status;
    int     address_conversion_status;

    assert(NULL != client);
    assert(NULL != address);

    master_cblk_ptr = (sCHAT_CLIENT_CBLK*)client;

    message.type                            = CHAT_CLIENT_MESSAGE_CONNECT;
    message.params.connect.address.sin_port = htons(port);

    message.params.connect.address.sin_family = AF_INET;
    address_conversion_status                 = inet_pton(AF_INET,
                                                          address,
                                                          &message.params.connect.address.sin_addr);
    if (0 == address_conversion_status)
    {
        message.params.connect.address.sin_family = AF_INET6;
        address_conversion_status                 = inet_pton(AF_INET6,
                                                              address,
                                                              &message.params.connect.address.sin_addr);
        if (0 == address_conversion_status)
        {
            return STATUS_INVALID_ARG;
        }
    }

    memset(message.params.connect.address.sin_zero,
           0,
           sizeof(message.params.connect.address.sin_zero));

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}


eSTATUS chat_client_send_text(
    CHAT_CLIENT client,
    const char* text)
{
    sCHAT_CLIENT_CBLK*   master_cblk_ptr;
    sCHAT_CLIENT_MESSAGE message;

    eSTATUS status;
    int     string_copy_status;

    assert(NULL != client);
    assert(NULL != text);

    master_cblk_ptr = (sCHAT_CLIENT_CBLK*)client;

    message.type = CHAT_CLIENT_MESSAGE_SEND_TEXT;
    string_copy_status = snprintf(&message.params.send_text.text[0],
                                  sizeof(message.params.send_text.text),
                                  "%s",
                                  text);
    if(string_copy_status < 0)
    {
        return STATUS_INVALID_ARG;
    }

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    return status;
}


eSTATUS chat_client_disconnect(
    CHAT_CLIENT client)
{
    sCHAT_CLIENT_CBLK*   master_cblk_ptr;
    sCHAT_CLIENT_MESSAGE message;

    eSTATUS status;

    assert(NULL != client);
    master_cblk_ptr = (sCHAT_CLIENT_CBLK*)client;

    message.type = CHAT_CLIENT_MESSAGE_DISCONNECT;

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
