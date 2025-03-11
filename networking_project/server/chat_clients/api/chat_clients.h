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
    CHAT_CLIENTS_EVENT_INCOMING_EVENT = 1 << 0,
    CHAT_CLIENTS_EVENT_CLOSED = 1 << 31
} bCHAT_CLIENTS_EVENT_TYPE;


typedef struct
{
    CHAT_CLIENTS_AUTH_EVENT_AUTHENTICATED,

    CHAT_CLIENTS_AUTH_EVENT_NEW_USER,
    CHAT_CLIENTS_AUTH_EVENT_PASSWORD_REQUIRED,
    CHAT_CLIENTS_AUTH_EVENT_PASSWORD_REJECTED
} sCHAT_CONNETIONS_AUTH_EVENT_TYPE;


typedef struct
{
    sCHAT_CONNETIONS_AUTH_EVENT_TYPE type;
    sCHAT_USER                       user_info;
} sCHAT_CLIENTS_AUTH_EVENT;


typedef struct
{
    sCHAT_EVENT       event;
    const sCHAT_USER* user;
} sCHAT_CONNECTION_EVENT_DATA_INCOMING_EVENT;


typedef union
{
    sCHAT_CONNECTION_EVENT_DATA_INCOMING_EVENT incoming_event;
} uCHAT_CLIENTS_CBACK_DATA;


typedef void (*fCHAT_CLIENTS_USER_CBACK) (
    void*                     user_arg,
    bCHAT_CLIENTS_EVENT_TYPE  event_mask,
    uCHAT_CLIENTS_CBACK_DATA* data);


typedef void* CHAT_CLIENTS;


// Functions -------------------------------------------------------------------

eSTATUS chat_clients_create(
    CHAT_CLIENTS*            out_new_chat_clients,
    fCHAT_CLIENTS_USER_CBACK user_cback,
    void*                    user_arg,
    uint32_t                 default_size);


eSTATUS chat_clients_auth_event(
    CHAT_CLIENTS             chat_clients,
    const sCHAT_USER*        user,
    sCHAT_CLIENTS_AUTH_EVENT user_id);


eSTATUS chat_clients_close(
    CHAT_CLIENTS* chat_clients);


#ifdef __cplusplus
}
#endif
