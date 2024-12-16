#include "chat_client.h"
#include "chat_client_internal.h"
#include "chat_client_fsm.h"


static eSTATUS init_cblk(
    sCHAT_CLIENT_CBLK *master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);

    memset(master_cblk_ptr, 0, sizeof(master_cblk_ptr));
    master_cblk_ptr->state = CHAT_CLIENT_STATE_NOT_CONNECTED;

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
