#include "chat_event_io.h"
#include "chat_event_io_internal.h"
#include "chat_event_io_fsm.h"
#include "chat_event_io_network_fsm.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"
#include "message_queue.h"


static void init_cblk(
    sCHAT_EVENT_IO_CBLK* master_cblk_ptr)
{
    int fd_index;

    assert(NULL != master_cblk_ptr);
    memset(master_cblk_ptr, 0, sizeof(sCHAT_EVENT_IO_CBLK));
    master_cblk_ptr->state = CHAT_EVENT_IO_STATE_READY;
}


eSTATUS chat_event_io_create(
    CHAT_EVENT_IO*            out_new_chat_event_io,
    sMODULE_PARAMETERS        chat_event_io_params,
    eCHAT_EVENT_IO_MODE       mode,
    int                       fd,
    fCHAT_EVENT_IO_USER_CBACK user_cback,
    void*                     user_arg)
{
    sCHAT_EVENT_IO_CBLK*         new_chat_event_io;

    eSTATUS                status;
    sCHAT_EVENT_IO_MESSAGE message;

    assert(NULL != out_new_chat_event_io);
    assert(NULL != user_cback);

    assert(NULL != chat_event_io_params.allocator);
    assert(NULL != chat_event_io_params.deallocator);
    assert(NULL != chat_event_io_params.thread_creator);

    new_chat_event_io = (sCHAT_EVENT_IO_CBLK*)chat_event_io_params.allocator(sizeof(sCHAT_EVENT_IO_CBLK));
    if (NULL == new_chat_event_io)
    {
        return STATUS_ALLOC_FAILED;
    }

    init_cblk(new_chat_event_io);

    status = message_queue_create(&new_chat_event_io->message_queue,
                                  CHAT_EVENT_IO_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_EVENT_IO_MESSAGE),
                                  chat_event_io_params.allocator,
                                  chat_event_io_params.deallocator);
    if (STATUS_SUCCESS != status)
    {
        chat_event_io_params.deallocator(new_chat_event_io);
        return status;
    }

    new_chat_event_io->mode        = mode;
    new_chat_event_io->fd          = fd;
    new_chat_event_io->allocator   = chat_event_io_params.allocator;
    new_chat_event_io->deallocator = chat_event_io_params.deallocator;
    new_chat_event_io->user_cback  = user_cback;
    new_chat_event_io->user_arg    = user_arg;

    status = chat_event_io_params.thread_creator(chat_event_io_thread_entry, new_chat_event_io);
    if (STATUS_SUCCESS != status)
    {
        message_queue_destroy(new_chat_event_io->message_queue);
        chat_event_io_params.deallocator(new_chat_event_io);
        return status;
    }

    *out_new_chat_event_io = (void*)new_chat_event_io;
    return STATUS_SUCCESS;
}


eSTATUS chat_event_io_populate_writer(
    CHAT_EVENT_IO      chat_event_io_writer,
    const sCHAT_EVENT* event)
{
    sCHAT_EVENT_IO_CBLK*   master_cblk_ptr;
    eSTATUS                status;
    sCHAT_EVENT_IO_MESSAGE message;
    
    assert(NULL != chat_event_io_writer);
    assert(NULL != event);

    master_cblk_ptr = (sCHAT_EVENT_IO_CBLK*)chat_event_io_writer;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    
    return STATUS_SUCCESS;
}


eSTATUS chat_event_io_do_operation(
    CHAT_EVENT_IO chat_event_io)
{
    sCHAT_EVENT_IO_CBLK*   master_cblk_ptr;
    sCHAT_EVENT_IO_MESSAGE message;
    eSTATUS                status;

    assert(NULL != chat_event_io);
    master_cblk_ptr = (sCHAT_EVENT_IO_CBLK*)chat_event_io;

    message.type              = CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    assert(STATUS_SUCCESS == status);

    return STATUS_SUCCESS;
}


eSTATUS chat_event_io_close(
    CHAT_EVENT_IO chat_event_io)
{
    sCHAT_EVENT_IO_CBLK*   master_cblk_ptr;
    sCHAT_EVENT_IO_MESSAGE message;
    eSTATUS                status;

    assert(NULL != chat_event_io);
    master_cblk_ptr = (sCHAT_EVENT_IO_CBLK*)chat_event_io;

    message.type = CHAT_EVENT_IO_MESSAGE_TYPE_CLOSE;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    assert(STATUS_SUCCESS == status);

    return STATUS_SUCCESS;
}
