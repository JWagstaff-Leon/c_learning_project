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
    CHAT_CLIENT_MESSAGE_SEND_NEW,
    CHAT_CLIENT_MESSAGE_SEND_CONTINUE,
    CHAT_CLIENT_MESSAGE_POLL,
    CHAT_CLIENT_MESSAGE_INCOMING_EVENT,
    CHAT_CLIENT_MESSAGE_DISCONNECT,
    CHAT_CLIENT_MESSAGE_CLOSE
} eCHAT_CLIENT_MESSAGE_TYPE;


typedef struct
{
    struct sockaddr_storage address;
} sCHAT_CLIENT_CONNECT_PARAMS;


typedef struct
{
    char             text[CHAT_EVENT_MAX_DATA_SIZE];
    eCHAT_EVENT_TYPE event_type;
} sCHAT_CLIENT_SEND_NEW_PARAMS;


typedef struct
{
    sCHAT_EVENT event;
} sCHAT_CLIENT_INCOMING_EVENT_PARAMS;


typedef union
{
    sCHAT_CLIENT_CONNECT_PARAMS        connect;
    sCHAT_CLIENT_SEND_NEW_PARAMS       send_new;
    sCHAT_CLIENT_INCOMING_EVENT_PARAMS incoming_event;
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
