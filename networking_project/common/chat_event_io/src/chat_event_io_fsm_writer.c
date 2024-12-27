#include "chat_event_io.h"
#include "chat_event_io_internal.h"
#include "chat_event_io_fsm.h"

#include <arpa/inet.h>
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "chat_event.h"
#include "common_types.h"


static eSTATUS do_write(
    sCHAT_EVENT_IO_CBLK* writer)
{
    ssize_t  written_bytes;
    uint32_t remaining_bytes;

    assert(NULL != writer);
    assert(CHAT_EVENT_IO_MODE_WRITER == writer->mode);

    remaining_bytes = (CHAT_EVENT_HEADER_SIZE + ntohs(writer->event.length)) - writer->processed_bytes;
    if(remaining_bytes == 0)
    {
        return STATUS_SUCCESS;
    }

    written_bytes = write(writer->fd,
                          (void*)(&writer->event),
                          remaining_bytes);
    if (written_bytes <= 0)
    {
        return STATUS_FAILURE;
    }
    writer->processed_bytes += written_bytes;

    if (writer->processed_bytes < CHAT_EVENT_HEADER_SIZE + ntohs(writer->event.length))
    {
        return STATUS_INCOMPLETE;
    }

    return STATUS_SUCCESS;
}


static sCHAT_EVENT_IO_RESULT omni_processing(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eSTATUS               status;
    sCHAT_EVENT_IO_RESULT result;

    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_POPULATE_WRITER:
        {
            master_cblk_ptr->event.type   = htonl(message->params.populate_writer.event.type);
            master_cblk_ptr->event.origin = htonl(message->params.populate_writer.event.origin);
            master_cblk_ptr->event.length = htons(message->params.populate_writer.event.length);
            memcpy(&master_cblk_ptr->event.data[0],
                   message->params.populate_writer.event.data,
                   message->params.populate_writer.event.length);

            result.event = CHAT_EVENT_IO_EVENT_POPULATE_SUCCESS;
            return result;
        }
        case CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE:
        {
            status = do_write(master_cblk_ptr);
            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    master_cblk_ptr->state = CHAT_EVENT_IO_STATE_READY;
                    master_cblk_ptr->processed_bytes = 0;

                    result.event = CHAT_EVENT_IO_EVENT_WRITE_FINISHED;
                    return result;
                }
                case STATUS_INCOMPLETE:
                {
                    master_cblk_ptr->state = CHAT_EVENT_IO_STATE_IN_PROGRESS;

                    result.event = CHAT_EVENT_IO_EVENT_INCOMPLETE;
                    return result;
                }
                case STATUS_FAILURE:
                {
                    result.event = CHAT_EVENT_IO_EVENT_FAILED;
                    return result;
                }
                default:
                {
                    // Should never get here
                    assert(0);
                    result.event = CHAT_EVENT_IO_EVENT_UNDEFINED;
                    return result;
                }
            }
            break;
        }
    }
    // Should not get here
    assert(0);
    result.event = CHAT_EVENT_IO_EVENT_UNDEFINED;
    return result;
}
    


eCHAT_EVENT_IO_EVENT_TYPE chat_event_io_writer_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    sCHAT_EVENT_IO_RESULT result;
    
    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (master_cblk_ptr->state)
    {
        case CHAT_EVENT_IO_STATE_READY:
        case CHAT_EVENT_IO_STATE_IN_PROGRESS:
        {
            return omni_processing(master_cblk_ptr, message);
        }
        case CHAT_EVENT_IO_STATE_FLUSHING:
        default:
        {
            // Should never get here
            assert(0);
        }
    }
    // Should never get here
    assert(0);
    result.event = CHAT_EVENT_IO_EVENT_UNDEFINED;
    return result;
}
