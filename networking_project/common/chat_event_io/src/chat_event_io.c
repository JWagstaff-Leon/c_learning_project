#include "chat_event_io.h"
#include "chat_event_io_internal.h"
#include "chat_event_io_fsm.h"
#include "chat_event_io_network_fsm.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"


static void init_cblk(
    sCHAT_EVENT_IO_CBLK* master_cblk_ptr)
{
    int fd_index;

    assert(NULL != master_cblk_ptr);
    memset(master_cblk_ptr, 0, sizeof(sCHAT_EVENT_IO_CBLK));
    master_cblk_ptr->state = CHAT_EVENT_IO_STATE_READY;
}


// Used to prevent calling back to the user in the event of failure
static void swallow_cback(
    void*                      user_arg,
    eCHAT_EVENT_IO_EVENT_TYPE  event,
    sCHAT_EVENT_IO_CBACK_DATA* data)
{
    (void)user_arg;
    (void)event;
    (void)data;
}


eSTATUS chat_event_io_create_reader(
    CHAT_EVENT_IO*            out_new_reader,
    sMODULE_PARAMETERS        reader_params,
    int                       reader_fd, // FIXME make this support multiple fds
    fCHAT_EVENT_IO_USER_CBACK user_cback,
    void*                     user_arg)
{
    sCHAT_EVENT_IO_CBLK*         new_chat_event_io;

    eSTATUS                status;
    sCHAT_EVENT_IO_MESSAGE message;

    assert(NULL != out_new_reader);
    assert(NULL != user_cback);

    assert(NULL != reader_params.allocator);
    assert(NULL != reader_params.deallocator);
    assert(NULL != reader_params.thread_creator);

    new_chat_event_io = (sCHAT_EVENT_IO_CBLK*)reader_params.allocator(sizeof(sCHAT_EVENT_IO_CBLK));
    if (NULL == new_chat_event_io)
    {
        return STATUS_ALLOC_FAILED;
    }

    init_cblk(new_chat_event_io);

    status = message_queue_create(&new_chat_event_io->message_queue,
                                  CHAT_EVENT_IO_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_EVENT_IO_MESSAGE),
                                  reader_params.allocator,
                                  reader_params.deallocator);
    if (STATUS_SUCCESS != status)
    {
        reader_params.deallocator(new_chat_event_io);
        return status;
    }

    new_chat_event_io->allocator   = reader_params.allocator;
    new_chat_event_io->deallocator = reader_params.deallocator;
    new_chat_event_io->user_cback  = user_cback;
    new_chat_event_io->user_arg    = user_arg;    

    status = reader_params.thread_creator(chat_event_io_thread_entry, new_chat_event_io);
    if (STATUS_SUCCESS != status)
    {
        message_queue_destroy(new_chat_event_io->message_queue);
        reader_params.deallocator(new_chat_event_io);
        return status;
    }

    *out_new_reader = (void*)new_chat_event_io;
    return STATUS_SUCCESS;
}
