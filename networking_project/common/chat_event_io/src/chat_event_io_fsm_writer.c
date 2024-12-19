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


static eSTATUS write_to_fd(
    sCHAT_EVENT_IO_CBLK* writer,
    int                  fd)
{
    ssize_t  written_bytes;
    uint32_t remaining_bytes;

    assert(NULL != writer);
    assert(CHAT_EVENT_IO_WRITER == writer->type);

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


void chat_event_io_writer_dispatch_message(
    sCHAT_EVENT_IO_CBLK*   master_cblk_ptr,
    sCHAT_EVENT_IO_MESSAGE message)
{

}
