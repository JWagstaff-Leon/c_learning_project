#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_event.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_EVENT_IO_MESSAGE_TYPE_POPULATE_WRITER,
    CHAT_EVENT_IO_MESSAGE_TYPE_OPERATE,
    CHAT_EVENT_IO_MESSAGE_TYPE_RESET
} eCHAT_EVENT_IO_MESSAGE_TYPE;


typedef struct
{
    sCHAT_EVENT event;
} sCHAT_EVENT_IO_POPULATE_WRITER_PARAMS;


typedef struct
{
    int fd;
} sCHAT_EVENT_IO_OPERATE_PARAMS;


typedef union
{
    sCHAT_EVENT_IO_POPULATE_WRITER_PARAMS populate_writer;
    sCHAT_EVENT_IO_OPERATE_PARAMS         operate;
} uCHAT_EVENT_IO_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_EVENT_IO_MESSAGE_TYPE   type;
    uCHAT_EVENT_IO_MESSAGE_PARAMS params;
} sCHAT_EVENT_IO_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
