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
    CHAT_AUTH_EVENT_TYPE_AUTH_RESULT = 1 >> 0
} bCHAT_AUTH_EVENT_TYPE;


typedef enum {
    CHAT_AUTH_RESULT_USERNAME_REQUIRED,
    CHAT_AUTH_RESULT_USERNAME_REJECTED,

    CHAT_AUTH_RESULT_PASSWORD_CREATION,
    CHAT_AUTH_RESULT_PASSWORD_REQUIRED,
    CHAT_AUTH_RESULT_PASSWORD_REJECTED,

    CHAT_AUTH_RESULT_AUTHENTICATED
} eCHAT_AUTH_RESULT;


typedef struct
{
    void*             auth_object;
    sCHAT_USER        user_info;
    eCHAT_AUTH_RESULT result;
} sCHAT_AUTH_CBACK_DATA_AUTH_RESULT;


typedef struct {
    sCHAT_AUTH_CBACK_DATA_AUTH_RESULT auth_result;
} sCHAT_AUTH_CBACK_DATA;


typedef void (*fCHAT_AUTH_USER_CBACK) (
    void*                  user_arg,
    bCHAT_AUTH_EVENT_TYPE  event_mask,
    sCHAT_AUTH_CBACK_DATA* data);


typedef void* CHAT_AUTH;


// Functions -------------------------------------------------------------------

eSTATUS chat_auth_create(
    CHAT_AUTH*            out_chat_auth,
    fCHAT_AUTH_USER_CBACK user_back,
    void*                 user_arg);


eSTATUS chat_auth_open_db(
    CHAT_AUTH   chat_auth,
    const char* db_path);


eSTATUS chat_auth_close_db(
    CHAT_AUTH   chat_auth,
    const char* db_path);


eSTATUS chat_auth_submit_credentials(
    CHAT_AUTH              chat_auth,
    void*                  auth_object,
    sCHAT_USER_CREDENTIALS credentials);


eSTATUS chat_auth_close(
    void);


#ifdef __cplusplus
}
#endif
