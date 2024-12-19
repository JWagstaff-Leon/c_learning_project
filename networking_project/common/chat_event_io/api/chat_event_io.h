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
    CHAT_EVENT_IO_EVENT_PLACEHOLDER = 1 << 0
} eCHAT_EVENT_IO_EVENT_TYPE;


typedef void sCHAT_EVENT_IO_CBACK_DATA;


typedef void (fCHAT_EVENT_IO_USER_CBACK*) (
    void*                      user_arg,
    eCHAT_EVENT_IO_EVENT_TYPE  event,
    sCHAT_EVENT_IO_CBACK_DATA* data);


typedef void* CHAT_EVENT_IO;


// Functions -------------------------------------------------------------------

eSTATUS chat_event_io_create_reader(
    CHAT_EVENT_IO*            out_new_reader,
    sMODULE_PARAMETERS        reader_params,
    int                       reader_fd,
    fCHAT_EVENT_IO_USER_CBACK user_cback,
    void*                     user_arg);


eSTATUS chat_event_io_create_writer(
    CHAT_EVENT_IO*            out_new_writer,
    sMODULE_PARAMETERS        writer_params,
    int                       writer_fd,
    fCHAT_EVENT_IO_USER_CBACK user_cback,
    void*                     user_arg);


eSTATUS chat_event_io_make_reader(
    CHAT_EVENT_IO chat_event_io);

    
eSTATUS chat_event_io_make_writer(
    CHAT_EVENT_IO chat_event_io);


eSTATUS chat_event_io_change_fd(
    CHAT_EVENT_IO chat_event_io,
    int           new_fd);


eSTATUS chat_event_io_cancel_operation(
    CHAT_EVENT_IO chat_event_io);


eSTATUS chat_event_io_close(
    CHAT_EVENT_IO chat_event_io);


#ifdef __cplusplus
}
#endif
