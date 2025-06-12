#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_event.h"
#include "chat_user.h"
#include "common_types.h"
#include "network_watcher.h"
#include "shared_ptr.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENTS_EVENT_START_AUTH_TRANSACTION  = 1 << 0,
    CHAT_CLIENTS_EVENT_FINISH_AUTH_TRANSACTION = 1 << 1,

    CHAT_CLIENTS_EVENT_CLOSED = 1 << 31
} bCHAT_CLIENTS_EVENT_TYPE;


typedef enum
{
    CHAT_CLIENTS_AUTH_STEP_USERNAME_REQUIRED,
    CHAT_CLIENTS_AUTH_STEP_USERNAME_REJECTED,
    CHAT_CLIENTS_AUTH_STEP_PASSWORD_CREATION,
    CHAT_CLIENTS_AUTH_STEP_PASSWORD_REQUIRED,
    CHAT_CLIENTS_AUTH_STEP_PASSWORD_REJECTED,
    CHAT_CLIENTS_AUTH_STEP_AUTHENTICATED,
    CHAT_CLIENTS_AUTH_STEP_CLOSED
} eCHAT_CLIENTS_AUTH_STEP;


typedef struct
{
    SHARED_PTR client_ptr;
    void**     auth_transaction_container;
    SHARED_PTR credentials_ptr;
} sCHAT_CLIENTS_CBACK_DATA_START_AUTH_TRANSACTION;


typedef struct
{
    void* auth_transaction;
} sCHAT_CLIENTS_CBACK_DATA_FINISH_AUTH_TRANSACTION;


typedef struct
{
    sCHAT_CLIENTS_CBACK_DATA_START_AUTH_TRANSACTION  start_auth_transaction;
    sCHAT_CLIENTS_CBACK_DATA_FINISH_AUTH_TRANSACTION finish_auth_transaction;
} sCHAT_CLIENTS_CBACK_DATA;


typedef void (*fCHAT_CLIENTS_USER_CBACK) (
    void*                           user_arg,
    bCHAT_CLIENTS_EVENT_TYPE        event_mask,
    const sCHAT_CLIENTS_CBACK_DATA* data);


typedef void* CHAT_CLIENTS;


// Functions -------------------------------------------------------------------

eSTATUS chat_clients_create(
    CHAT_CLIENTS*            out_new_chat_clients,
    fCHAT_CLIENTS_USER_CBACK user_cback,
    void*                    user_arg);


eSTATUS chat_clients_open_client(
    CHAT_CLIENTS chat_clients,
    int fd);


eSTATUS chat_clients_auth_event(
    CHAT_CLIENTS            chat_clients,
    eCHAT_CLIENTS_AUTH_STEP auth_step,
    sCHAT_USER              user_info,
    SHARED_PTR              client_ptr);


eSTATUS chat_clients_close(
    CHAT_CLIENTS* chat_clients);


#ifdef __cplusplus
}
#endif
