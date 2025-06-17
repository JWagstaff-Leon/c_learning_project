#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdbool.h>

#include "chat_event.h"
#include "message_queue.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CLIENT_MAIN_MESSAGE_INCOMING_EVENT,
    CLIENT_MAIN_MESSAGE_OUTGOING_EVENT,

    CLIENT_MAIN_MESSAGE_CLOSE
} eCLIENT_MAIN_MESSAGE_TYPE;


typedef struct
{
    sCHAT_EVENT event;
} sCLIENT_MAIN_MESSAGE_PARAMS_INCOMING_EVENT;


typedef struct
{
    sCHAT_EVENT event;
} sCLIENT_MAIN_MESSAGE_PARAMS_OUTGOING_EVENT;


typedef union
{
    sCLIENT_MAIN_MESSAGE_PARAMS_INCOMING_EVENT incoming_event;
    sCLIENT_MAIN_MESSAGE_PARAMS_OUTGOING_EVENT outgoing_event;
} uCLIENT_MAIN_MESSAGE_PARAMS;


typedef struct
{
    eCLIENT_MAIN_MESSAGE_TYPE   type;
    uCLIENT_MAIN_MESSAGE_PARAMS params;
} sCLIENT_MAIN_MESSAGE;


typedef struct
{
    bool          open;
    MESSAGE_QUEUE message_queue;

    
} sCLIENT_MAIN_CBLK;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
