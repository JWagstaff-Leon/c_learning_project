#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_MESSAGE_CLOSE,

    CHAT_SERVER_MESSAGE_CLIENTS_CLIENT_OPEN_FAILED,
    CHAT_SERVER_MESSAGE_CLIENTS_START_AUTH_TRANSACTION,
    CHAT_SERVER_MESSAGE_CLIENTS_FINISH_AUTH_TRANSACTION,
    CHAT_SERVER_MESSAGE_CLIENTS_CLOSED
} eCHAT_SERVER_MESSAGE_TYPE;


typedef struct
{
    void*                  client_ptr;
    void**                 auth_transaction_container;
    sCHAT_USER_CREDENTIALS credentials;
} sCHAT_SERVER_MESSAGE_PARAMS_CLIENTS_START_AUTH_TRANSACTION;


typedef struct
{
    void* client_ptr;
    void* auth_transaction;
} sCHAT_SERVER_MESSAGE_PARAMS_CLIENTS_FINISH_AUTH_TRANSACTION;


typedef union
{
    sCHAT_SERVER_MESSAGE_PARAMS_CLIENTS_START_AUTH_TRANSACTION  start_auth_transaction;
    sCHAT_SERVER_MESSAGE_PARAMS_CLIENTS_FINISH_AUTH_TRANSACTION finish_auth_transaction;
} uCHAT_SERVER_MESSAGE_PARAMS_CLIENTS;


typedef union
{
    uCHAT_SERVER_MESSAGE_PARAMS_CLIENTS clients;
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
