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
    sCHAT_EVENT_IO_CBLK* reader)
{
    ssize_t     read_bytes;
    uint32_t    empty_bytes;
    void*       buffer_tail;

    assert(NULL != reader);
    assert(CHAT_EVENT_IO_MODE_READER == reader->mode);

    empty_bytes = sizeof(reader->event) - reader->processed_bytes;

    if (0 == empty_bytes)
    {
        return STATUS_SUCCESS;
    }

    buffer_tail = (void*)((uint8_t*)&reader->event + reader->processed_bytes);
    read_bytes = read(reader->fd,
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


static eSTATUS extract_read_event(
    sCHAT_EVENT_IO_CBLK* restrict reader,
    sCHAT_EVENT*         restrict out_event)
{
    uint32_t extracted_event_size;

    assert(NULL != reader);
    assert(NULL != out_event);
    assert(CHAT_EVENT_IO_MODE_READER == reader->mode);

    if (reader->processed_bytes < CHAT_EVENT_HEADER_SIZE ||
        reader->processed_bytes < CHAT_EVENT_HEADER_SIZE + ntohs(reader->event.length))
    {
        return STATUS_INCOMPLETE;
    }

    out_event->type   = ntohl(reader->event.type);
    out_event->origin = ntohl(reader->event.origin);
    out_event->length = ntohs(reader->event.length);

    extracted_event_size  = CHAT_EVENT_HEADER_SIZE + out_event->length;
    memcpy(out_event->data,
           &reader->event.data,
           out_event->length);

    reader->processed_bytes -= extracted_event_size;
    memmove(&(uint8_t*)(&reader->event)[0],
            &(uint8_t*)(&reader->event)[extracted_event_size],
            reader->processed_bytes);

    return STATUS_SUCCESS;
}


static eSTATUS do_flush(
    sCHAT_EVENT_IO_CBLK* reader)
{
    ssize_t     read_bytes;
    uint32_t    bytes_to_flush;

    assert(NULL != reader);
    assert(CHAT_EVENT_IO_MODE_READER == reader->mode);

    bytes_to_flush = (reader->flush_bytes - reader->processed_bytes) > sizeof(reader->event) ?
                         sizeof(reader->event) : (reader->flush_bytes - reader->processed_bytes);

    read_bytes = read(reader->fd,
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


static sCHAT_EVENT_IO_RESULT ready_processing(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eSTATUS               status;
    sCHAT_EVENT_IO_RESULT result;

    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE:
        {
            status = do_read(master_cblk_ptr);

            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    status = extract_read_event(master_cblk_ptr, &result.data.read_finished.read_event);
                    assert(STATUS_SUCCESS == status);
                    
                    result.event = CHAT_EVENT_IO_EVENT_READ_FINISHED;
                    return result;
                }
                case STATUS_INCOMPLETE:
                {
                    result.event = CHAT_EVENT_IO_EVENT_INCOMPLETE;
                    return result;
                }
                case STATUS_CLOSED:
                {
                    result.event = CHAT_EVENT_IO_EVENT_FD_CLOSED;
                    return result;
                }
                case STATUS_FAILURE:
                {
                    result.event = CHAT_EVENT_IO_EVENT_FAILED;
                    return result;
                }
                case STATUS_NO_SPACE:
                {
                    master_cblk_ptr->state = CHAT_EVENT_IO_STATE_FLUSHING;

                    result.event = CHAT_EVENT_IO_EVENT_FLUSH_REQUIRED;
                    return result;
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
    result.event = CHAT_EVENT_IO_EVENT_UNDEFINED;
    return result;
}


static void flushing_processing(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eSTATUS               status;
    sCHAT_EVENT_IO_RESULT result;

    assert(NULL != master_cblk_ptr);
    assert(NULL != message);
    
    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE:
        {
            status = do_flush(master_cblk_ptr);
            switch (status)
            {
                case STATUS_SUCCESS:
                {
                    result.event = CHAT_EVENT_IO_EVENT_FLUSH_FINISHED;
                    return result;
                }
                case STATUS_INCOMPLETE:
                {
                    reuslt.event = CHAT_EVENT_IO_EVENT_INCOMPLETE;
                    return result;
                }
                case STATUS_CLOSED:
                {
                    result.event = CHAT_EVENT_IO_EVENT_FD_CLOSED;
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


eCHAT_EVENT_IO_EVENT_TYPE chat_event_io_reader_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    sCHAT_EVENT_IO_MESSAGE new_message;
    sCHAT_EVENT_IO_RESULT  result;

    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_POPULATE_WRITER:
        {
            result.event = CHAT_EVENT_IO_EVENT_CANT_POPULATE_READER;
            return result;
        }
    }

    switch (master_cblk_ptr->state)
    {
        case CHAT_EVENT_IO_STATE_READY:
        {
            return ready_processing(master_cblk_ptr, message);
        }
        case CHAT_EVENT_IO_STATE_FLUSHING:
        {
            return flushing_processing(master_cblk_ptr, message);
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
    return result;
}
