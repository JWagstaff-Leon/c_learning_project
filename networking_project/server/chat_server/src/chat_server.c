// Includes --------------------------------------------------------------------

#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include "common_types.h"
#include "message_queue.h"

// Function Implementations ----------------------------------------------------

static eSTATUS init_cblk(
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);

    memset(master_cblk_ptr, 0, sizeof(master_cblk_ptr));
    master_cblk_ptr->state = CHAT_SERVER_STATE_INIT;

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
                                  CHAT_SERVER_MSG_QUEUE_SIZE,
                                  sizeof(sCHAT_SERVER_MESSAGE),
                                  allocator,
                                  deallocator);

    return status;
}


eSTATUS chat_server_create(
    CHAT_SERVER*                     out_new_chat_server,
    fGENERIC_ALLOCATOR               allocator,
    fGENERIC_DEALLOCATOR             deallocator,
    fCHAT_SERVER_THREAD_CREATE_CBACK create_thread,
    fCHAT_SERVER_USER_CBACK          user_cback,
    void*                            user_arg)
{
    sCHAT_SERVER_CBLK* new_master_cblk_ptr;
    
    eSTATUS status;
    int     thread_status;

    assert(NULL != allocator);
    assert(NULL != deallocator);
    assert(NULL != user_cback);
    assert(NULL != out_new_chat_server);

    new_master_cblk_ptr = (sCHAT_SERVER_CBLK*)allocator(sizeof(sCHAT_SERVER_CBLK));
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

    status = create_thread(chat_server_thread_entry, new_master_cblk_ptr);
    if (STATUS_SUCCESS != status)
    {
        deallocator(new_master_cblk_ptr);
        return status;
    }
    
    *out_new_chat_server = new_master_cblk_ptr;
    return STATUS_SUCCESS;
}


eSTATUS chat_server_open(
    CHAT_SERVER chat_server)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE message;
    sCHAT_SERVER_CBLK*   master_cblk_ptr;

    assert(NULL != chat_server);
    master_cblk_ptr = (sCHAT_SERVER_CBLK*)chat_server;

    message.type = CHAT_SERVER_MESSAGE_OPEN;
    status       = message_queue_put(master_cblk_ptr->message_queue,
                                     &message,
                                     sizeof(message));

    return status;
}


eSTATUS chat_server_close(
    CHAT_SERVER chat_server)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE message;
    sCHAT_SERVER_CBLK*   master_cblk_ptr;

    assert(NULL != chat_server);
    master_cblk_ptr = (sCHAT_SERVER_CBLK*)chat_server;

    message.type = CHAT_SERVER_MESSAGE_CLOSE;
    status       = message_queue_put(master_cblk_ptr->message_queue,
                                     &message,
                                     sizeof(message));

    return status;
}
