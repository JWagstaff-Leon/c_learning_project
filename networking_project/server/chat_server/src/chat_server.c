// Includes --------------------------------------------------------------------

#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <pthread.h>
#include <string.h>

#include "common_types.h"


// Globals ---------------------------------------------------------------------

sCHAT_SERVER_CBLK k_master_cblk; // REVIEW add an allocator to allow dynamic allocation?


// Function Implementations ----------------------------------------------------

static eSTATUS init_cblk(
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);

    memset(master_cblk_ptr, 0, sizeof(master_cblk_ptr));
    master_cblk_ptr->state = CHAT_SERVER_STATE_INIT;

    master_cblk_ptr->listen_socket = -1;

    return STATUS_SUCCESS;
}


static eSTATUS init_msg_queue(
    void *message_queue_ptr)
{
    assert(NULL != message_queue_ptr);

    // TODO do the thing

    return STATUS_SUCCESS;
}


eSTATUS chat_server_init(
    fCHAT_SERVER_THREAD_CREATE_CBACK create_thread,
    fCHAT_SERVER_USER_CBACK          user_cback,
    void                            *user_arg)
{
    eSTATUS status;
    int     thread_status;

    assert(NULL != user_cback);

    status = init_cblk(&k_master_cblk.message_queue);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    k_master_cblk.user_cback = user_cback;
    k_master_cblk.user_arg   = user_arg;

    status = init_msg_queue(&k_master_cblk);
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
    sCHAT_SERVER_MESSAGE message;
    message.type = CHAT_SERVER_MESSAGE_OPEN;

    // TODO Queue message

    return STATUS_SUCCESS;
}


eSTATUS chat_server_reset(void)
{
    return STATUS_SUCCESS;
}


eSTATUS chat_server_close(void)
{
    return STATUS_SUCCESS;
}
