#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "common_types.h"
#include "chat_user.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum {
    CHAT_AUTH_EVENT_DATABASE_OPENED       = 1 << 0,
    CHAT_AUTH_EVENT_DATABASE_OPEN_FAILED  = 1 << 1,
    CHAT_AUTH_EVENT_DATABASE_CLOSED       = 1 << 2,
    CHAT_AUTH_EVENT_DATABASE_CLOSE_FAILED = 1 << 3,
    CHAT_AUTH_EVENT_AUTH_RESULT           = 1 << 4,
    CHAT_AUTH_EVENT_TRANSACTION_DONE      = 1 << 5
} bCHAT_AUTH_EVENT_TYPE;


typedef enum {
    CHAT_AUTH_RESULT_USERNAME_REQUIRED,
    CHAT_AUTH_RESULT_USERNAME_REJECTED,

    CHAT_AUTH_RESULT_PASSWORD_CREATION,
    CHAT_AUTH_RESULT_PASSWORD_REQUIRED,
    CHAT_AUTH_RESULT_PASSWORD_REJECTED,

    CHAT_AUTH_RESULT_AUTHENTICATED,

    CHAT_AUTH_RESULT_FAILURE // General failure of auth; will not be retried
} eCHAT_AUTH_RESULT;


typedef struct
{
    sCHAT_USER        user_info;
    eCHAT_AUTH_RESULT result;
} sCHAT_AUTH_CBACK_DATA_AUTH_RESULT;


typedef struct
{
    void* consumer_arg;
} sCHAT_AUTH_CBACK_DATA_TRANSACTION_DONE;


typedef struct {
    sCHAT_AUTH_CBACK_DATA_AUTH_RESULT      auth_result;
    sCHAT_AUTH_CBACK_DATA_TRANSACTION_DONE transaction_done;
} sCHAT_AUTH_CBACK_DATA;


typedef void (*fCHAT_AUTH_USER_CBACK) (
    void*                        user_arg,
    bCHAT_AUTH_EVENT_TYPE        event_mask,
    const sCHAT_AUTH_CBACK_DATA* data);


typedef void* CHAT_AUTH;
typedef void* CHAT_AUTH_TRANSACTION;


// Functions -------------------------------------------------------------------

eSTATUS chat_auth_create(
    CHAT_AUTH*            out_chat_auth,
    fCHAT_AUTH_USER_CBACK user_cback,
    void*                 user_arg);


eSTATUS chat_auth_open_database(
    CHAT_AUTH   chat_auth,
    const char* path);


eSTATUS chat_auth_close_database(
    CHAT_AUTH chat_auth);


eSTATUS chat_auth_submit_credentials(
    CHAT_AUTH              chat_auth,
    sCHAT_USER_CREDENTIALS credentials,
    void*                  consumer_arg,
    CHAT_AUTH_TRANSACTION* out_auth_transaction);


eSTATUS chat_auth_finish_transaction(
    CHAT_AUTH_TRANSACTION auth_transaction);


eSTATUS chat_auth_close(
    CHAT_AUTH chat_auth);


#ifdef __cplusplus
}
#endif
