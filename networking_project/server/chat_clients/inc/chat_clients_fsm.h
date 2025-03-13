#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------



// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENTS_MESSAGE_INCOMING_EVENT,
    CHAT_CLIENTS_MESSAGE_CLIENT_CLOSED,

    CHAT_CLIENTS_MESSAGE_NEW_CONNECTION,
    CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_CANCELLED,
    CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_ERROR,
    CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_CLOSED,

    CHAT_CLIENTS_MESSAGE_CLOSE
} eCHAT_CLIENTS_MESSAGE_TYPE;


typedef struct
{
    sCHAT_EVENT   event;
    sCHAT_CLIENT* client_ptr;
} sCHAT_CLIENT_INCOMING_EVENT_PARAMS;


typedef union
{
    sCHAT_CLIENT_INCOMING_EVENT_PARAMS incoming_event;
} uCHAT_CLIENTS_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_CLIENTS_MESSAGE_TYPE   type;
    uCHAT_CLIENTS_MESSAGE_PARAMS params;
} sCHAT_CLIENTS_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
