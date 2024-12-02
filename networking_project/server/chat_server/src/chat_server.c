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


// Globals ---------------------------------------------------------------------

static sCHAT_SERVER_CBLK k_master_cblk; // REVIEW add an allocator to allow dynamic allocation?


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
    MESSAGE_QUEUE_ID* message_queue_ptr)
{
    eSTATUS status;

    assert(NULL != message_queue_ptr);

    status = message_queue_create(message_queue_ptr,
                                  CHAT_SERVER_MSG_QUEUE_SIZE,
                                  sizeof(sCHAT_SERVER_MESSAGE),
                                  malloc,
                                  free);

    return status;
}


eSTATUS chat_server_init(
    fCHAT_SERVER_THREAD_CREATE_CBACK create_thread,
    fCHAT_SERVER_USER_CBACK          user_cback,
    void                            *user_arg)
{
    eSTATUS status;
    int     thread_status;

    assert(NULL != user_cback);

    status = init_cblk(&k_master_cblk);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    k_master_cblk.user_cback = user_cback;
    k_master_cblk.user_arg   = user_arg;

    status = init_msg_queue(&k_master_cblk.message_queue);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    status = create_thread(chat_server_process_thread_entry, &k_master_cblk);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }
    
    return STATUS_SUCCESS;
}


eSTATUS chat_server_open(
    void)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE message;

    message.type = CHAT_SERVER_MESSAGE_OPEN;
    status       = message_queue_put(k_master_cblk.message_queue,
                                     &message,
                                     sizeof(message));

    return status;
}


eSTATUS chat_server_reset(void)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE message;

    message.type = CHAT_SERVER_MESSAGE_RESET;
    status       = message_queue_put(k_master_cblk.message_queue,
                                     &message,
                                     sizeof(message));

    return status;
}


eSTATUS chat_server_close(void)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE message;

    message.type = CHAT_SERVER_MESSAGE_CLOSE;
    status       = message_queue_put(k_master_cblk.message_queue,
                                     &message,
                                     sizeof(message));

    return status;
}
