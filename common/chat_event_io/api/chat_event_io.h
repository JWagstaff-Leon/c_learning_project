#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include <stdbool.h>

#include "chat_event.h"
#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_EVENT_IO_RESULT_READ_FINISHED    = 1 << 0,
    CHAT_EVENT_IO_RESULT_WRITE_FINISHED   = 1 << 1,
    CHAT_EVENT_IO_RESULT_INCOMPLETE       = 1 << 2,
    CHAT_EVENT_IO_RESULT_EXTRACT_FINISHED = 1 << 3,
    CHAT_EVENT_IO_RESULT_EXTRACT_MORE     = 1 << 4,
    CHAT_EVENT_IO_RESULT_FD_CLOSED        = 1 << 5,
    CHAT_EVENT_IO_RESULT_FAILED           = 1 << 6,
    CHAT_EVENT_IO_RESULT_POPULATE_SUCCESS = 1 << 7,
    CHAT_EVENT_IO_RESULT_POPULATE_FAILED  = 1 << 8,
    CHAT_EVENT_IO_RESULT_NOT_POPULATED    = 1 << 9,

    CHAT_EVENT_IO_RESULT_CLOSED    = 1 << 30,
    CHAT_EVENT_IO_RESULT_UNDEFINED = 1 << 31
} bCHAT_EVENT_IO_RESULT;


typedef void* CHAT_EVENT_IO;


// Functions -------------------------------------------------------------------

eSTATUS chat_event_io_create(
    CHAT_EVENT_IO* out_new_chat_event_io);


bCHAT_EVENT_IO_RESULT chat_event_io_populate_writer(
    CHAT_EVENT_IO      chat_event_io_writer,
    const sCHAT_EVENT* event);


bCHAT_EVENT_IO_RESULT chat_event_io_read_from_fd(
    CHAT_EVENT_IO chat_event_io,
    int           fd);


bCHAT_EVENT_IO_RESULT chat_event_io_extract_read_event(
    CHAT_EVENT_IO restrict chat_event_io,
    sCHAT_EVENT*  restrict event_buffer);


bCHAT_EVENT_IO_RESULT chat_event_io_write_to_fd(
    CHAT_EVENT_IO      chat_event_io,
    int                fd);


eSTATUS chat_event_io_destroy(
    CHAT_EVENT_IO chat_event_io);


bool chat_event_io_read_finished(
    const CHAT_EVENT_IO chat_event_io);


bool chat_event_io_write_finished(
    const CHAT_EVENT_IO chat_event_io);


#ifdef __cplusplus
}
#endif
