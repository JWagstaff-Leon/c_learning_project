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
    if(remaining_bytes <= 0)
    {
        return STATUS_INVALID_ARG;
    }

    // Do the write
    written_bytes = write(writer->fd,
                          (void*)(&writer->event),
                          remaining_bytes);
    if (written_bytes <= 0)
    {
        return STATUS_FAILURE;
    }
    writer->processed_bytes += written_bytes;

    // Check if the read is done
    if (writer->processed_bytes < CHAT_EVENT_HEADER_SIZE + ntohs(writer->event.length))
    {
        return STATUS_INCOMPLETE;
    }

    return STATUS_SUCCESS;
}


static void ready_processing(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eSTATUS status;

    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_POPULATE_WRITER:
        {
            master_cblk_ptr->event.type   = htonl(event->type);
            master_cblk_ptr->event.origin = htonl(event->origin);
            master_cblk_ptr->event.length = htons(event->length);
            memcpy(master_cblk_ptr->event.data,
                   event->data,
                   event->length);
            break;
        }
        case CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE:
        {
            status = do_write(master_cblk_ptr);
            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_WRITE_FINISHED,
                                                NULL);
                    master_cblk_ptr->processed_bytes = 0;
                    break;
                }
            }
            break;
        }
        case CHAT_EVENT_IO_MESSAGE_TYPE_CLOSE:
        {
            master_cblk_ptr->state = CHAT_EVENT_IO_STATE_CLOSED;
            break;
        }
    }
}
    


void chat_event_io_writer_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (master_cblk_ptr->state)
    {
        case CHAT_EVENT_IO_STATE_READY:
        {
            ready_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_EVENT_IO_STATE_IN_PROGRESS:
        {
            in_progress_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_EVENT_IO_STATE_FLUSHING:
        case CHAT_EVENT_IO_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}
