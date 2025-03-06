#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "common_types.h"
#include "network_watcher.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CONNECTIONS_EVENT_INCOMING_EVENT = 1 << 0,
    CHAT_CONNECTIONS_EVENT_CLOSED = 1 << 31
} bCHAT_CONNECTIONS_EVENT_TYPE;


typedef struct
{
    CHAT_CONNECTIONS_AUTH_EVENT_AUTHENTICATED,

    CHAT_CONNECTIONS_AUTH_EVENT_NEW_USER,
    CHAT_CONNECTIONS_AUTH_EVENT_PASSWORD_REQUIRED,
    CHAT_CONNECTIONS_AUTH_EVENT_PASSWORD_REJECTED
} sCHAT_CONNETIONS_AUTH_EVENT_TYPE;


typedef struct
{
    sCHAT_CONNETIONS_AUTH_EVENT_TYPE type;
    sCHAT_USER                       user_info;
} sCHAT_CONNECTIONS_AUTH_EVENT;


typedef struct
{
    sCHAT_EVENT       event;
    const sCHAT_USER* user;
} sCHAT_CONNECTION_EVENT_DATA_INCOMING_EVENT;


typedef union
{
    sCHAT_CONNECTION_EVENT_DATA_INCOMING_EVENT incoming_event;
} uCHAT_CONNECTIONS_CBACK_DATA;


typedef void (*fCHAT_CONNECTIONS_USER_CBACK) (
    void*                         user_arg,
    bCHAT_CONNECTIONS_EVENT_TYPE  event_mask,
    uCHAT_CONNECTIONS_CBACK_DATA* data);


typedef void* CHAT_CONNECTIONS;


// Functions -------------------------------------------------------------------

eSTATUS chat_connections_create(
    CHAT_CONNECTIONS*            out_new_chat_connections,
    fCHAT_CONNECTIONS_USER_CBACK user_cback,
    void*                        user_arg,
    uint32_t                     default_size);


eSTATUS chat_connections_auth_event(
    CHAT_CONNECTIONS             chat_connections,
    const sCHAT_USER*            user,
    sCHAT_CONNECTIONS_AUTH_EVENT user_id);


eSTATUS chat_connections_close(
    CHAT_CONNECTIONS* chat_connections);


#ifdef __cplusplus
}
#endif
