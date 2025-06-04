#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_event.h"
#include "chat_user.h"
#include "network_watcher.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CONNECTION_MESSAGE_TYPE_WATCH_COMPLETE,
    CHAT_CONNECTION_MESSAGE_TYPE_WATCH_CANCELLED,
    CHAT_CONNECTION_MESSAGE_TYPE_QUEUE_EVENT,

    CHAT_CONNECTION_MESSAGE_TYPE_CLOSE, // REVIEW add a message to closing?
    CHAT_CONNECTION_MESSAGE_TYPE_CLOSE_IMMEDIATELY
} eCHAT_CONNECTION_MESSAGE_TYPE;


typedef struct
{
    bNETWORK_WATCHER_MODE modes_ready;
} sCHAT_CONNECTION_MESSAGE_PARAMS_WATCH_COMPLETE;


typedef struct
{
    sCHAT_EVENT event;
} sCHAT_CONNECTION_MESSAGE_PARAMS_QUEUE_EVENT;


typedef union
{
    sCHAT_CONNECTION_MESSAGE_PARAMS_WATCH_COMPLETE watch_complete;
    sCHAT_CONNECTION_MESSAGE_PARAMS_QUEUE_EVENT    queue_event;
} uCHAT_CONNECTION_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_CONNECTION_MESSAGE_TYPE   type;
    uCHAT_CONNECTION_MESSAGE_PARAMS params;
} sCHAT_CONNECTION_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
