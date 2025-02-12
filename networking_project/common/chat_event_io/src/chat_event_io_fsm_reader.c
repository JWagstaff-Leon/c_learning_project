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


static eSTATUS do_read(
    sCHAT_EVENT_IO_OPERATOR* reader,
    int                      fd)
{
    ssize_t     read_bytes;
    uint32_t    empty_bytes;
    void*       buffer_tail;

    assert(NULL != reader);

    empty_bytes = sizeof(reader->event) - reader->processed_bytes;

    if (0 == empty_bytes)
    {
        return STATUS_SUCCESS;
    }

    buffer_tail = (void*)((uint8_t*)&reader->event + reader->processed_bytes);
    read_bytes = read(fd,
                      buffer_tail,
                      empty_bytes);
    if (read_bytes < 0)
    {
        return STATUS_FAILURE;
    }
    if (read_bytes == 0)
    {
        return STATUS_CLOSED;
    }
    reader->processed_bytes += read_bytes;

    if (reader->processed_bytes < CHAT_EVENT_HEADER_SIZE)
    {
        return STATUS_INCOMPLETE;
    }

    if (sizeof(reader->event) < CHAT_EVENT_HEADER_SIZE + ntohs(reader->event.length))
    {
        reader->flush_bytes = CHAT_EVENT_HEADER_SIZE + ntohs(reader->event.length);
        return STATUS_NO_SPACE;
    }

    if (reader->processed_bytes < CHAT_EVENT_HEADER_SIZE + ntohs(reader->event.length))
    {
        return STATUS_INCOMPLETE;
    }

    return STATUS_SUCCESS;
}


static eSTATUS do_flush(
    sCHAT_EVENT_IO_OPERATOR* reader,
    int                      fd)
{
    ssize_t     read_bytes;
    uint32_t    bytes_to_flush;

    assert(NULL != reader);

    bytes_to_flush = (reader->flush_bytes - reader->processed_bytes) > sizeof(reader->event)
                         ? sizeof(reader->event)
                         : (reader->flush_bytes - reader->processed_bytes);

    read_bytes = read(fd,
                      (void*)(&reader->event),
                      bytes_to_flush);
    if (read_bytes < 0)
    {
        return STATUS_FAILURE;
    }
    if (read_bytes == 0)
    {
        return STATUS_CLOSED;
    }

    reader->processed_bytes += read_bytes;

    assert(reader->processed_bytes <= reader->flush_bytes);
    if (reader->processed_bytes == reader->flush_bytes)
    {
        return STATUS_SUCCESS;
    }

    return STATUS_INCOMPLETE;
}


static eCHAT_EVENT_IO_RESULT ready_processing(
    sCHAT_EVENT_IO_OPERATOR*      reader,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eSTATUS status;

    assert(NULL != reader);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE:
        {
            status = do_read(reader,
                             message->params.operate.fd);

            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    return CHAT_EVENT_IO_RESULT_READ_FINISHED;
                }
                case STATUS_INCOMPLETE:
                {
                    return CHAT_EVENT_IO_RESULT_INCOMPLETE;
                }
                case STATUS_CLOSED:
                {
                    return CHAT_EVENT_IO_RESULT_FD_CLOSED;
                }
                case STATUS_FAILURE:
                {
                    return CHAT_EVENT_IO_RESULT_FAILED;
                }
                case STATUS_NO_SPACE:
                {
                    reader->state = CHAT_EVENT_IO_OPERATOR_STATE_FLUSHING;

                    return CHAT_EVENT_IO_RESULT_FLUSH_REQUIRED;
                }
                default:
                {
                    // Should never get here
                    assert(0);
                }
            }
            break;
        }
    }

    // Should not get here
    assert(0);
    return CHAT_EVENT_IO_RESULT_UNDEFINED;
}


static void flushing_processing(
    sCHAT_EVENT_IO_OPERATOR*      reader,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eSTATUS               status;
    eCHAT_EVENT_IO_RESULT result;

    assert(NULL != reader);
    assert(NULL != message);
    
    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE:
        {
            status = do_flush(reader,
                              message->params.operate.fd);
            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    return CHAT_EVENT_IO_RESULT_FLUSH_FINISHED;
                }
                case STATUS_INCOMPLETE:
                {
                    return CHAT_EVENT_IO_RESULT_INCOMPLETE;
                }
                case STATUS_CLOSED:
                {
                    return CHAT_EVENT_IO_RESULT_FD_CLOSED;
                }
                case STATUS_FAILURE:
                {
                    return CHAT_EVENT_IO_RESULT_FAILED;
                }
                default:
                {
                    // Should never get here
                    assert(0);
                }
            }
            break;
        }
    }

    // Should not get here
    assert(0);
    return CHAT_EVENT_IO_RESULT_UNDEFINED;
}


eCHAT_EVENT_IO_RESULT chat_event_io_reader_dispatch_message(
    sCHAT_EVENT_IO_OPERATOR*      reader,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eCHAT_EVENT_IO_RESULT  result;

    assert(NULL != reader);
    assert(NULL != message);

    switch (reader->state)
    {
        case CHAT_EVENT_IO_OPERATOR_STATE_READY:
        {
            return ready_processing(reader, message);
        }
        case CHAT_EVENT_IO_OPERATOR_STATE_FLUSHING:
        {
            return flushing_processing(reader, message);
        }
        default:
        {
            // Should never get here
            assert(0);
        }
    }

    // Should never get here
    assert(0);
    return CHAT_EVENT_IO_RESULT_UNDEFINED;
}
