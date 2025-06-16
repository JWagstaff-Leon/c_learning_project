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
    CHAT_CLIENT_MESSAGE_USER_INPUT,
    CHAT_CLIENT_MESSAGE_INCOMING_EVENT,
    CHAT_CLIENT_MESSAGE_CONNECTION_CLOSED,
    CHAT_CLIENT_MESSAGE_CLOSE
} eCHAT_CLIENT_MESSAGE_TYPE;


typedef struct
{
    char text[CHAT_EVENT_MAX_DATA_SIZE];
} sCHAT_CLIENT_MESSAGE_PARAMS_USER_INPUT;


typedef struct
{
    sCHAT_EVENT event;
} sCHAT_CLIENT_MESSAGE_PARAMS_INCOMING_EVENT;


typedef union
{
    sCHAT_CLIENT_MESSAGE_PARAMS_USER_INPUT     user_input;
    sCHAT_CLIENT_MESSAGE_PARAMS_INCOMING_EVENT incoming_event;
} uCHAT_CLIENT_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_CLIENT_MESSAGE_TYPE   type;
    uCHAT_CLIENT_MESSAGE_PARAMS params;
} sCHAT_CLIENT_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
