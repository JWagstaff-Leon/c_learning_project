#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------



// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENTS_MESSAGE_OPEN_CLIENT,
    CHAT_CLIENTS_MESSAGE_AUTH_EVENT,

    CHAT_CLIENTS_MESSAGE_INCOMING_EVENT,
    CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED,

    CHAT_CLIENTS_MESSAGE_CLOSE
} eCHAT_CLIENTS_MESSAGE_TYPE;


typedef struct
{
    int fd;
} sCHAT_CLIENTS_OPEN_CLIENT_PARAMS;


typedef struct
{
    sCHAT_EVENT   event;
    sCHAT_CLIENT* client_ptr;
} sCHAT_CLIENTS_INCOMING_EVENT_PARAMS;


typedef struct
{
    sCHAT_CLIENT* client_ptr;
} sCHAT_CLIENTS_CLIENT_CLOSED_PARAMS;


typedef struct
{
    sCHAT_CLIENT_AUTH*       auth_object;
    sCHAT_CLIENTS_AUTH_EVENT auth_event;
} sCHAT_CLIENTS_AUTH_RESULT_PARAMS;


typedef union
{
    sCHAT_CLIENTS_OPEN_CLIENT_PARAMS    open_client;
    sCHAT_CLIENTS_INCOMING_EVENT_PARAMS incoming_event;
    sCHAT_CLIENTS_CLIENT_CLOSED_PARAMS  client_closed;
    sCHAT_CLIENTS_AUTH_RESULT_PARAMS    auth_result;
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
