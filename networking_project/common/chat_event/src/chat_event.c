#include "chat_event.h"
#include "chat_event_internal.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"

eSTATUS chat_event_io_init(
    sCHAT_EVENT_IO* chat_event_io)
{
    assert(NULL != chat_event_io);
    memset(chat_event_io, 0, sizeof(sCHAT_EVENT_IO));
}


eSTATUS chat_event_io_read_from_fd(
    sCHAT_EVENT_IO* reader,
    int             fd)
{
    ssize_t     read_bytes;
    uint32_t    empty_bytes;
    
    assert(NULL != reader);

    if (reader->flush_bytes)
    {
        empty_bytes = reader->flush_bytes > sizeof(reader->event) ?
                      sizeof(reader->event) : reader->flush_bytes;
    }
    else
    {
        empty_bytes = sizeof(reader->event) - reader->processed_bytes;
    }

    if (0 == empty_bytes)
    {
        return STATUS_NO_SPACE;
    }

    // Do the read
    read_bytes = read(fd,
                      (void*)(&reader->event),
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

    if (reader->flush_bytes)
    {
        reader->flush_bytes     -= reader->processed_bytes;
        reader->processed_bytes  = 0;
        return STATUS_INCOMPLETE;
    }

    // Check if the read is done
    if (reader->processed_bytes < CHAT_EVENT_HEADER_SIZE ||
        reader->processed_bytes < CHAT_EVENT_HEADER_SIZE + reader->event.length)
    {
        return STATUS_INCOMPLETE;
    }

    if (!reader->flush_bytes &&
        sizeof(reader->event) < CHAT_EVENT_HEADER_SIZE + reader->event.length)
    {
        reader->flush_bytes = sizeof(reader->event) < CHAT_EVENT_HEADER_SIZE + reader->event.length;
        reader->flush_bytes     -= reader->processed_bytes;
        reader->processed_bytes  = 0;
        return STATUS_INCOMPLETE;
    }

    return STATUS_SUCCESS;
}


eSTATUS chat_event_io_extract_read_event(
    sCHAT_EVENT_IO* restrict reader,
    sCHAT_EVENT*    restrict out_event)
{
    size_t extracted_event_size;

    assert(NULL != reader);
    assert(NULL != out_event);

    if (reader->processed_bytes < CHAT_EVENT_HEADER_SIZE ||
        reader->processed_bytes < CHAT_EVENT_HEADER_SIZE + reader->event.length)
    {
        return STATUS_INCOMPLETE;
    }

    extracted_event_size = CHAT_EVENT_HEADER_SIZE + reader->event.length;

    memcpy(out_event,
           &reader->event,
           extracted_event_size);

    reader->processed_bytes -= extracted_event_size;
    memmove(&(uint8_t*)(&reader->event)[0],
            &(uint8_t*)(&reader->event)[extracted_event_size],
            reader->processed_bytes);

    return STATUS_SUCCESS;
}


eSTATUS chat_event_io_write_to_fd(
    sCHAT_EVENT_IO* writer,
    int             fd)
{
    ssize_t  written_bytes;
    uint32_t remaining_bytes;

    assert(NULL != writer);

    remaining_bytes = (CHAT_EVENT_HEADER_SIZE + writer->event.length) - writer->processed_bytes;
    if(remaining_bytes <= 0)
    {
        return STATUS_INVALID_ARG;
    }

    // Do the write
    written_bytes = write(fd,
                          (void*)(&writer->event),
                          remaining_bytes);
    if (written_bytes <= 0)
    {
        return STATUS_FAILURE;
    }
    reader->processed_bytes += written_bytes;

    // Check if the read is done
    if (reader->processed_bytes < CHAT_EVENT_HEADER_SIZE + reader->event.length)
    {
        return STATUS_INCOMPLETE;
    }

    writer->processed_bytes = 0;
    return STATUS_SUCCESS;
}
