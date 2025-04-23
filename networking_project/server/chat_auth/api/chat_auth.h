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


// Functions -------------------------------------------------------------------

// REVIEW leave this as a singleton, or make it dynamic?
eSTATUS chat_auth_open(
    void);


eSTATUS chat_auth_submit_credentials(
    void*                  auth_object,
    sCHAT_USER_CREDENTIALS credentials);


eSTATUS chat_auth_shutdown(
    void);


#ifdef __cplusplus
}
#endif
