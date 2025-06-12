#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_user.h"
#include "shared_ptr.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_AUTH_MESSAGE_PROCESS_CREDENTIALS,
    CHAT_AUTH_MESSAGE_CLOSE
} eCHAT_AUTH_MESSAGE_TYPE;


typedef struct
{
    SHARED_PTR              credentials_ptr;
    sCHAT_AUTH_TRANSACTION* auth_transaction;
} sCHAT_AUTH_MESSAGE_PARAMS_PROCESS_CREDENTIALS;


typedef union
{
    sCHAT_AUTH_MESSAGE_PARAMS_PROCESS_CREDENTIALS process_credentials;
} uCHAT_AUTH_MESSAGE_PARAMS;

typedef struct
{
    eCHAT_AUTH_MESSAGE_TYPE   type;
    uCHAT_AUTH_MESSAGE_PARAMS params;
} sCHAT_AUTH_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
