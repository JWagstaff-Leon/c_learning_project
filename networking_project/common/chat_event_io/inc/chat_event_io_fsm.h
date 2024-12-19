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
    CHAT_EVENT_IO_MESSAGE_TYPE_READ,
    CHAT_EVENT_IO_MESSAGE_TYPE_WRITE,

    CHAT_EVENT_IO_MESSAGE_TYPE_CHANGE_MODE,
    CHAT_EVENT_IO_MESSAGE_TYPE_CHANGE_FD,
    CHAT_EVENT_IO_MESSAGE_TYPE_CANCEL,
    CHAT_EVENT_IO_MESSAGE_TYPE_CLOSE
} eCHAT_EVENT_IO_MESSAGE_TYPE;


typedef struct
{
    eCHAT_EVENT_IO_MODE new_mode;
} sCHAT_EVENT_IO_CHANGE_MODE_PARAMS;


typedef struct
{
    int new_fd;
} sCHAT_EVENT_IO_CHANGE_FD_PARAMS;


typedef union
{
    sCHAT_EVENT_IO_CHANGE_MODE_PARAMS change_mode;
    sCHAT_EVENT_IO_CHANGE_FD_PARAMS   change_fd;
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
