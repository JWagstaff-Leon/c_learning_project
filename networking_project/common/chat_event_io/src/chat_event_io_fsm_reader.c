#include "chat_event_io.h"
#include "chat_event_io_internal.h"
#include "chat_event_io_fsm.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "chat_event.h"
#include "common_types.h"


static eSTATUS read_from_fd(
    sCHAT_EVENT_IO_CBLK* reader,
    int                  fd)
{
    ssize_t     read_bytes;
    uint32_t    empty_bytes;
    
    assert(NULL != reader);
    assert(CHAT_EVENT_IO_READER == reader->type);

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


static eSTATUS extract_read_event(
    sCHAT_EVENT_READER* restrict reader,
    sCHAT_EVENT*        restrict out_event)
{
    size_t extracted_event_size;

    assert(NULL != reader);
    assert(NULL != out_event);
    assert(CHAT_EVENT_IO_READER == reader->type);

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

void chat_event_io_reader_dispatch_message(
    sCHAT_EVENT_IO_CBLK    master_cblk_ptr,
    sCHAT_EVENT_IO_MESSAGE message)
{

}
