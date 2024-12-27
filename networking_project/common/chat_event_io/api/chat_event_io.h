#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include "chat_event.h"
#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_EVENT_IO_MODE_READER,
    CHAT_EVENT_IO_MODE_WRITER
} eCHAT_EVENT_IO_MODE;


typedef enum
{
    CHAT_EVENT_IO_EVENT_READ_FINISHED        = 1 <<  0,
    CHAT_EVENT_IO_EVENT_WRITE_FINISHED       = 1 <<  1,
    CHAT_EVENT_IO_EVENT_INCOMPLETE           = 1 <<  2,
    CHAT_EVENT_IO_EVENT_FD_CLOSED            = 1 <<  3,
    CHAT_EVENT_IO_EVENT_FAILED               = 1 <<  4,
    CHAT_EVENT_IO_EVENT_FLUSH_REQUIRED       = 1 <<  5,
    CHAT_EVENT_IO_EVENT_FLUSH_FINISHED       = 1 <<  6,
    CHAT_EVENT_IO_EVENT_CANT_POPULATE_READER = 1 <<  7,
    CHAT_EVENT_IO_EVENT_POPULATE_SUCCESS     = 1 <<  8,
    CHAT_EVENT_IO_EVENT_POPULATE_FAILED      = 1 <<  9,
    CHAT_EVENT_IO_EVENT_CLOSED               = 1 << 10,
    CHAT_EVENT_IO_EVENT_UNDEFINED            = 1 << 31
} eCHAT_EVENT_IO_EVENT_TYPE;


typedef struct
{
    sCHAT_EVENT read_event;
} sCHAT_EVENT_IO_CBACK_READ_FINISHED_DATA;


typedef union
{
    sCHAT_EVENT_IO_CBACK_READ_FINISHED_DATA read_finished;
} uCHAT_EVENT_IO_CBACK_DATA;


typedef struct
{
    eCHAT_EVENT_IO_EVENT_TYPE event;
    uCHAT_EVENT_IO_CBACK_DATA data;
} sCHAT_EVENT_IO_RESULT;

typedef void* CHAT_EVENT_IO;


// Functions -------------------------------------------------------------------

eSTATUS chat_event_io_create(
    CHAT_EVENT_IO*            out_new_chat_event_io,
    fGENERIC_ALLOCATOR        allocator,
    eCHAT_EVENT_IO_MODE       mode,
    int                       fd);


sCHAT_EVENT_IO_RESULT chat_event_io_populate_writer(
    CHAT_EVENT_IO      chat_event_io_writer,
    const sCHAT_EVENT* event);


sCHAT_EVENT_IO_RESULT chat_event_io_do_operation(
    CHAT_EVENT_IO chat_event_io,
    sCHAT_EVENT*  out_event);


eSTATUS chat_event_io_close(
    CHAT_EVENT_IO        chat_event_io,
    fGENERIC_DEALLOCATOR deallocator);


#ifdef __cplusplus
}
#endif
