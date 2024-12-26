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

    if (sizeof(reader->event) < CHAT_EVENT_HEADER_SIZE + reader->event.length)
    {
        reader->flush_bytes = CHAT_EVENT_HEADER_SIZE + reader->event.length;
        return STATUS_NO_SPACE;
    }

    if (reader->processed_bytes < CHAT_EVENT_HEADER_SIZE + reader->event.length)
    {
        return STATUS_INCOMPLETE;
    }

    return STATUS_SUCCESS;
}


// FIXME add endianness conversions
static eSTATUS extract_read_event(
    sCHAT_EVENT_IO_CBLK* restrict reader,
    sCHAT_EVENT*         restrict out_event)
{
    size_t extracted_event_size;

    assert(NULL != reader);
    assert(NULL != out_event);
    assert(CHAT_EVENT_IO_MODE_READER == reader->mode);

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


static void ready_processing(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eSTATUS                   status;
    uCHAT_EVENT_IO_CBACK_DATA cback_data;

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
                    status = extract_read_event(master_cblk_ptr, &cback_data.read_finished.read_event);
                    assert(STATUS_SUCCESS == status);
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_READ_FINISHED,
                                                &cback_data);
                    master_cblk_ptr->processed_bytes = 0;
                    break;
                }
                case STATUS_INCOMPLETE:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_INCOMPLETE,
                                                &cback_data);
                    break;
                }
                case STATUS_CLOSED:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_FD_CLOSED,
                                                &cback_data);
                    break;
                }
                case STATUS_FAILURE:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_FALIED,
                                                &cback_data);
                    break;
                }
                case STATUS_NO_SPACE:
                {
                    master_cblk_ptr->state = CHAT_EVENT_IO_STATE_FLUSHING;
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_FLUSH_REQUIRED,
                                                &cback_data);
                    break;
                }
                default:
                {
                    // Should never get here
                    assert(0);
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


static void flushing_processing(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    eSTATUS status;

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
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_FLUSH_FINISHED,
                                                &cback_data);
                    break;
                }
                case STATUS_INCOMPLETE:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_INCOMPLETE,
                                                &cback_data);
                    break;
                }
                case STATUS_CLOSED:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_FD_CLOSED,
                                                &cback_data);
                    break;
                }
                case STATUS_FAILURE:
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_EVENT_IO_EVENT_FALIED,
                                                &cback_data);
                    break;
                }
                default:
                {
                    // Should never get here
                    assert(0);
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


void chat_event_io_reader_dispatch_message(
    sCHAT_EVENT_IO_CBLK*          master_cblk_ptr,
    const sCHAT_EVENT_IO_MESSAGE* message)
{
    sCHAT_EVENT_IO_MESSAGE new_message;

    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_EVENT_IO_MESSAGE_TYPE_POPULATE_WRITER:
        {
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_EVENT_IO_EVENT_CANT_POPULATE_READER,
                                        NULL);
            return;
        }
    }

    switch (master_cblk_ptr->state)
    {
        case CHAT_EVENT_IO_STATE_READY:
        {
            ready_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_EVENT_IO_STATE_FLUSHING:
        {
            flushing_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_EVENT_IO_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}
