#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_clients.h"
#include "chat_clients_internal.h"


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
    sCHAT_EVENT         event;
    sCHAT_CLIENT_ENTRY* client_entry;
} sCHAT_CLIENTS_INCOMING_EVENT_PARAMS;


typedef struct
{
    sCHAT_CLIENT_ENTRY* client_entry;
} sCHAT_CLIENTS_CLIENT_CLOSED_PARAMS;


typedef struct
{
    eCHAT_CLIENTS_AUTH_STEP auth_step;
    sCHAT_USER              user_info;
    sCHAT_CLIENT_ENTRY**    client_entry_ptr;
} sCHAT_CLIENTS_AUTH_EVENT_PARAMS;


typedef union
{
    sCHAT_CLIENTS_OPEN_CLIENT_PARAMS    open_client;
    sCHAT_CLIENTS_INCOMING_EVENT_PARAMS incoming_event;
    sCHAT_CLIENTS_CLIENT_CLOSED_PARAMS  client_closed;
    sCHAT_CLIENTS_AUTH_EVENT_PARAMS     auth_event;
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
