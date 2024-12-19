#include "chat_event_io.h"
#include "chat_event_io_internal.h"
#include "chat_event_io_fsm.h"

#include <assert.h>

#include "message_queue.h"


void* chat_event_io_thread_entry(
    void* arg)
{
    sCHAT_EVENT_IO_CBLK*   master_cblk_ptr;
    sCHAT_EVENT_IO_MESSAGE message;
    eSTATUS                status;

    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_EVENT_IO_CBLK*)arg;

    while (CHAT_EVENT_IO_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        switch (master_cblk_ptr->mode)
        {
            case CHAT_EVENT_IO_MODE_READER:
            {
                chat_event_io_reader_dispatch_message(master_cblk_ptr, message);
                break;
            }
            case CHAT_EVENT_IO_MODE_WRITER:
            {
                chat_event_io_writer_dispatch_message(master_cblk_ptr, message);
                break;
            }
        }
    }

    message_queue_destroy(master_cblk_ptr->message_queue);
    master_cblk_ptr->deallocator(master_cblk_ptr);
    return NULL;
}
