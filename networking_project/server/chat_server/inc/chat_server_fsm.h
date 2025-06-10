#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_server.h"

#include <stdint.h>

#include "chat_auth.h"
#include "chat_user.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_MESSAGE_CLOSE,

    CHAT_SERVER_MESSAGE_START_AUTH_TRANSACTION,
    CHAT_SERVER_MESSAGE_FINISH_AUTH_TRANSACTION,
    CHAT_SERVER_MESSAGE_CLIENTS_CLOSED,

    CHAT_SERVER_MESSAGE_INCOMING_CONNECTION,
    CHAT_SERVER_MESSAGE_LISTENER_ERROR,
    CHAT_SERVER_MESSAGE_LISTENER_CANCELLED,
    CHAT_SERVER_MESSAGE_LISTENER_CLOSED,

    CHAT_SERVER_MESSAGE_AUTH_RESULT,
    CHAT_SERVER_MESSAGE_AUTH_CLOSED
} eCHAT_SERVER_MESSAGE_TYPE;


typedef struct
{
    void*                  client_ptr;
    void**                 auth_transaction_container;
    sCHAT_USER_CREDENTIALS credentials;
} sCHAT_SERVER_MESSAGE_PARAMS_CLIENTS_START_AUTH_TRANSACTION;


typedef struct
{
    void* auth_transaction;
} sCHAT_SERVER_MESSAGE_PARAMS_CLIENTS_FINISH_AUTH_TRANSACTION;


typedef union
{
    sCHAT_SERVER_MESSAGE_PARAMS_CLIENTS_START_AUTH_TRANSACTION  start_auth_transaction;
    sCHAT_SERVER_MESSAGE_PARAMS_CLIENTS_FINISH_AUTH_TRANSACTION finish_auth_transaction;
} uCHAT_SERVER_MESSAGE_PARAMS_CLIENTS;


typedef struct
{
    sCHAT_USER        user_info;
    eCHAT_AUTH_RESULT result;
    void*             client_ptr;
} sCHAT_SERVER_MESSAGE_PARAMS_AUTH_RESULT;


typedef struct
{
    void* consumer_arg;
} sCHAT_SERVER_MESSAGE_PARAMS_AUTH_TRANSACTION_DONE;


typedef struct
{
    sCHAT_SERVER_MESSAGE_PARAMS_AUTH_RESULT           auth_result;
    sCHAT_SERVER_MESSAGE_PARAMS_AUTH_TRANSACTION_DONE transaction_done;
} uCHAT_SERVER_MESSAGE_PARAMS_AUTH;


typedef union
{
    uCHAT_SERVER_MESSAGE_PARAMS_CLIENTS clients;
    uCHAT_SERVER_MESSAGE_PARAMS_AUTH    auth;
} uCHAT_SERVER_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_SERVER_MESSAGE_TYPE   type;
    uCHAT_SERVER_MESSAGE_PARAMS params;
} sCHAT_SERVER_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
