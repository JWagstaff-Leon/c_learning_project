#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_event.h"
#include "chat_user.h"
#include "common_types.h"
#include "network_watcher.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENTS_EVENT_REQUEST_AUTHENTICATION = 1 << 0,
    CHAT_CLIENTS_EVENT_CLIENT_OPEN_FAILED     = 1 << 1,

    CHAT_CLIENTS_EVENT_CLOSED = 1 << 31
} bCHAT_CLIENTS_EVENT_TYPE;


// REVIEW move this to internal?
typedef enum
{
    CHAT_CLIENTS_AUTH_RESULT_USERNAME_REQUIRED,
    CHAT_CLIENTS_AUTH_RESULT_USERNAME_REJECTED,

    CHAT_CLIENTS_AUTH_RESULT_PASSWORD_CREATION,
    CHAT_CLIENTS_AUTH_RESULT_PASSWORD_REQUIRED,
    CHAT_CLIENTS_AUTH_RESULT_PASSWORD_REJECTED,

    CHAT_CLIENTS_AUTH_RESULT_AUTHENTICATED
} eCHAT_CLIENTS_AUTH_RESULT;


typedef struct
{
    eCHAT_CLIENTS_AUTH_RESULT result;
    const sCHAT_USER*         user_info;
} sCHAT_CLIENTS_AUTH_EVENT;


typedef void* CHAT_CLIENTS_AUTH_OBJECT;

typedef struct
{
    CHAT_CLIENTS_AUTH_OBJECT      auth_object;
    const sCHAT_USER_CREDENTIALS* credentials;
} sCHAT_CLIENTS_CBACK_DATA_REQUEST_AUTHENTICATION;


typedef union
{
    sCHAT_CLIENTS_CBACK_DATA_REQUEST_AUTHENTICATION request_authentication;
} sCHAT_CLIENTS_CBACK_DATA;


typedef void (*fCHAT_CLIENTS_USER_CBACK) (
    void*                     user_arg,
    bCHAT_CLIENTS_EVENT_TYPE  event_mask,
    sCHAT_CLIENTS_CBACK_DATA* data);


typedef void* CHAT_CLIENTS;


// Functions -------------------------------------------------------------------

eSTATUS chat_clients_create(
    CHAT_CLIENTS*            out_new_chat_clients,
    fCHAT_CLIENTS_USER_CBACK user_cback,
    void*                    user_arg,
    uint32_t                 default_size);


eSTATUS chat_clients_open_client(
    CHAT_CLIENTS chat_clients,
    int fd);


eSTATUS chat_clients_auth_event(
    CHAT_CLIENTS             chat_clients,
    CHAT_CLIENTS_AUTH_OBJECT auth_object,
    sCHAT_CLIENTS_AUTH_EVENT auth_event);


eSTATUS chat_clients_close(
    CHAT_CLIENTS* chat_clients);


#ifdef __cplusplus
}
#endif
