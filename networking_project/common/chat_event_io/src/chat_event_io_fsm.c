#include "chat_event_io.h"
#include "chat_event_io_internal.h"
#include "chat_event_io_fsm.h"

#include <assert.h>
#include <unistd.h>

#include "message_queue.h"


sCHAT_EVENT_IO_RESULT chat_event_io_operation_entry(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    sCHAT_EVENT_IO_RESULT result;

    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    pthread_mutex_lock(master_cblk_ptr->mutex);

    switch (master_cblk_ptr->mode)
    {
        case CHAT_EVENT_IO_MODE_READER:
        {
            result = chat_event_io_reader_dispatch_message(master_cblk_ptr, &message);
            goto func_exit;
        }
        case CHAT_EVENT_IO_MODE_WRITER:
        {
            result = chat_event_io_writer_dispatch_message(master_cblk_ptr, &message);
            goto func_exit;
        }
        default:
        {
            // Should never get here
            assert(0);
        }
    }

    // Should never get here
    assert(0);
    result.event = CHAT_EVENT_IO_EVENT_UNDEFINED;

func_exit:
    pthread_mutex_unlock(master_cblk_ptr->mutex);
    return result;
}
