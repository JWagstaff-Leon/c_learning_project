#include "chat_event_io.h"
#include "chat_event_io_internal.h"
#include "chat_event_io_fsm.h"

#include <assert.h>
#include <unistd.h>

#include "message_queue.h"


sCHAT_EVENT_IO_RESULT chat_event_io_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    sCHAT_EVENT_IO_RESULT result;

    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE:
        {
            switch (message->params.operate.mode)
            {
                case CHAT_EVENT_IO_OPERATION_MODE_READ:
                {
                    return chat_event_io_reader_dispatch_message(&master_cblk_ptr->reader, &message);
                }
                case CHAT_EVENT_IO_OPERATION_MODE_WRITE:
                {
                    return chat_event_io_writer_dispatch_message(&master_cblk_ptr->writer, &message);
                }
                default:
                {
                    // Should never get here
                    assert(0);
                }
            }
        }
        case CHAT_EVENT_IO_MESSAGE_TYPE_POPULATE_WRITER:
        {
            return chat_event_io_writer_dispatch_message(&master_cblk_ptr->writer, &message);
        }
    }

    // Should never get here
    assert(0);
    result.event = CHAT_EVENT_IO_EVENT_UNDEFINED;
    return result;

}
