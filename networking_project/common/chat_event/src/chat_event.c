#include "chat_event.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"

eSTATUS chat_event_read_from_fd(
    int                 fd,
    sCHAT_EVENT_READER* reader)
{
    ssize_t  read_bytes;
    uint32_t flush_bytes;

    switch (reader->state)
    {
        case CHAT_EVENT_READER_STATE_NEW:
        {
            reader->expected_bytes = sizeof(reader->event);
            reader->read_bytes     = 0;
            memset(&reader->event,
                   0,
                   sizeof(reader->event));

            read_bytes = read(fd,
                              (void*)(&reader->event),
                              CHAT_EVENT_HEADER_SIZE);
            if (read_bytes < 0)
            {
                return STATUS_FAILURE;
            }

            reader->read_bytes = read_bytes;
            if (reader->read_bytes >= CHAT_EVENT_HEADER_SIZE)
            {
                reader->expected_bytes = CHAT_EVENT_HEADER_SIZE + reader->event.length;
                assert(reader->read_bytes <= reader->expected_bytes);

                if(reader->expected_bytes > sizeof(reader->event))
                {
                    reader->state = CHAT_EVENT_READER_STATE_FLUSHING;
                    return STATUS_NO_SPACE;
                }
                
                if (reader->read_bytes >= reader->expected_bytes)
                {
                    reader->state = CHAT_EVENT_READER_STATE_DONE;
                }
                else
                {
                    reader->state = CHAT_EVENT_READER_STATE_DATA;
                }
            }
            else
            {
                reader->state = CHAT_EVENT_READER_STATE_HEADER;
            }

            break;
        }
        case CHAT_EVENT_READER_STATE_HEADER:
        {
            read_bytes = read(fd,
                              (void*)(((uint8_t*)&reader->event) + reader->read_bytes),
                              CHAT_EVENT_HEADER_SIZE - reader->read_bytes);
            if (read_bytes < 0)
            {
                return STATUS_FAILURE;
            }

            reader->read_bytes += read_bytes;
            if (reader->read_bytes >= CHAT_EVENT_HEADER_SIZE)
            {
                reader->expected_bytes = CHAT_EVENT_HEADER_SIZE + reader->event.length;
                assert(reader->read_bytes <= reader->expected_bytes);

                if(reader->expected_bytes > sizeof(reader->event))
                {
                    reader->state = CHAT_EVENT_READER_STATE_FLUSHING;
                    return STATUS_NO_SPACE;
                }

                if (reader->read_bytes >= reader->expected_bytes)
                {
                    reader->state = CHAT_EVENT_READER_STATE_DONE;
                }
                else
                {
                    reader->state = CHAT_EVENT_READER_STATE_DATA;
                }
            }

            break;
        }
        case CHAT_EVENT_READER_STATE_DATA:
        {
            read_bytes = read(fd,
                              (void*)(((uint8_t*)&reader->event) + reader->read_bytes),
                              reader->expected_bytes - reader->read_bytes);
            if (read_bytes < 0)
            {
                return STATUS_FAILURE;
            }

            reader->read_bytes += read_bytes;
            assert(reader->read_bytes <= reader->expected_bytes);

            if (reader->read_bytes >= reader->expected_bytes)
            {
                reader->state = CHAT_EVENT_READER_STATE_DONE;
            }

            break;
        }
        case CHAT_EVENT_READER_STATE_DONE:
        {
            break;
        }
        case CHAT_EVENT_READER_STATE_FLUSHING:
        {
            flush_bytes = reader->expected_bytes - reader->read_bytes;
            if (flush_bytes > sizeof(reader->event))
            {
                flush_bytes = sizeof(reader->event);
            }
            read_bytes = read(fd,
                              &reader->event,
                              flush_bytes);
            if (read_bytes > 0)
            {
                reader->read_bytes += read_bytes;
            }

            if (reader->read_bytes >= reader->expected_bytes)
            {
                reader->state = CHAT_EVENT_READER_STATE_FLUSHED;
                return STATUS_SUCCESS;
            }
            return STATUS_FAILURE;
        }
    }

    return STATUS_SUCCESS;
}
