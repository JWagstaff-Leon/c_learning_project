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
    sCHAT_EVENT_IO_OPERATOR* writer,
    int                      fd)
{
    ssize_t  written_bytes;
    uint32_t remaining_bytes;

    assert(NULL != writer);

    remaining_bytes = (CHAT_EVENT_HEADER_SIZE + ntohs(writer->event.length)) - writer->processed_bytes;
    if(remaining_bytes == 0)
    {
        return STATUS_SUCCESS;
    }

    written_bytes = write(fd,
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
    sCHAT_EVENT_IO_OPERATOR*      writer,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eSTATUS               status;
    sCHAT_EVENT_IO_RESULT result;

    assert(NULL != writer);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_POPULATE_WRITER:
        {
            writer->event.type   = htonl(message->params.populate_writer.event.type);
            writer->event.origin = htonl(message->params.populate_writer.event.origin);
            writer->event.length = htons(message->params.populate_writer.event.length);
            memcpy(&writer->event.data[0],
                   message->params.populate_writer.event.data,
                   message->params.populate_writer.event.length);

            result.event = CHAT_EVENT_IO_EVENT_POPULATE_SUCCESS;
            return result;
        }
        case CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE:
        {
            status = do_write(writer,
                              message->params.operate.fd);
            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    writer->state = CHAT_EVENT_IO_STATE_READY;
                    writer->processed_bytes = 0;

                    result.event = CHAT_EVENT_IO_EVENT_WRITE_FINISHED;
                    return result;
                }
                case STATUS_INCOMPLETE:
                {
                    writer->state = CHAT_EVENT_IO_STATE_IN_PROGRESS;

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
    sCHAT_EVENT_IO_OPERATOR*      writer,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    sCHAT_EVENT_IO_RESULT result;
    
    assert(NULL != writer);
    assert(NULL != message);

    switch (writer->state)
    {
        case CHAT_EVENT_IO_STATE_READY:
        case CHAT_EVENT_IO_STATE_IN_PROGRESS:
        {
            return omni_processing(writer, message);
        }
        case CHAT_EVENT_IO_STATE_FLUSHING:
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
