#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <arpa/inet.h>

#include "chat_event.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENT_MESSAGE_CONNECT,
    CHAT_CLIENT_MESSAGE_SEND_TEXT,
    CHAT_CLIENT_MESSAGE_POLL,
    CHAT_CLIENT_MESSAGE_DISCONNECT,
    CHAT_CLIENT_MESSAGE_CLOSE
} eCHAT_CLIENT_MESSAGE_TYPE;


typedef struct
{
    struct sockaddr_in address;
} sCHAT_CLIENT_CONNECT_PARAMS;


typedef struct
{
    char text[CHAT_EVENT_MAX_LENGTH];
} sCHAT_CLIENT_SEND_TEXT_PARAMS;


typedef union
{
    sCHAT_CLIENT_CONNECT_PARAMS   connect;
    sCHAT_CLIENT_SEND_TEXT_PARAMS send_text;
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
