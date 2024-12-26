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
    CHAT_EVENT_IO_EVENT_READ_FINISHED        = 1 << 0,
    CHAT_EVENT_IO_EVENT_WRITE_FINISHED       = 1 << 1,
    CHAT_EVENT_IO_EVENT_INCOMPLETE           = 1 << 2,
    CHAT_EVENT_IO_EVENT_FD_CLOSED            = 1 << 3,
    CHAT_EVENT_IO_EVENT_FALIED               = 1 << 4,
    CHAT_EVENT_IO_EVENT_FLUSH_REQUIRED       = 1 << 5,
    CHAT_EVENT_IO_EVENT_FLUSH_FINISHED       = 1 << 6,
    CHAT_EVENT_IO_EVENT_CANT_POPULATE_READER = 1 << 7,
    CHAT_EVENT_IO_EVENT_CLOSED               = 1 << 8
} eCHAT_EVENT_IO_EVENT_TYPE;


typedef struct
{
    sCHAT_EVENT read_event;
} sCHAT_EVENT_IO_CBACK_READ_FINISHED_DATA;


typedef union
{
    sCHAT_EVENT_IO_CBACK_READ_FINISHED_DATA read_finished;
} uCHAT_EVENT_IO_CBACK_DATA;


typedef void (fCHAT_EVENT_IO_USER_CBACK*) (
    void*                      user_arg,
    eCHAT_EVENT_IO_EVENT_TYPE  event,
    uCHAT_EVENT_IO_CBACK_DATA* data);


typedef void* CHAT_EVENT_IO;


// Functions -------------------------------------------------------------------

eSTATUS chat_event_io_create(
    CHAT_EVENT_IO*            out_new_reader,
    sMODULE_PARAMETERS        reader_params,
    eCHAT_EVENT_IO_MODE       mode,
    int                       fd,
    fCHAT_EVENT_IO_USER_CBACK user_cback,
    void*                     user_arg);


eSTATUS chat_event_io_populate_writer(
    CHAT_EVENT_IO      chat_event_io_writer,
    const sCHAT_EVENT* event);


eSTATUS chat_event_io_do_operation(
    CHAT_EVENT_IO chat_event_io);


eSTATUS chat_event_io_close(
    CHAT_EVENT_IO chat_event_io);


#ifdef __cplusplus
}
#endif
