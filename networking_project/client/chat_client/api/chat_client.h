#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENT_EVENT_CONNECTED         = 1 << 0,
    CHAT_CLIENT_EVENT_CONNECT_FAILED    = 1 << 1,
    CHAT_CLIENT_EVENT_USERNAME_ACCEPTED = 1 << 2,
    CHAT_CLIENT_EVENT_USERNAME_REJECTED = 1 << 3,
    CHAT_CLIENT_EVENT_SEND_STARTED      = 1 << 4,
    CHAT_CLIENT_EVENT_SEND_FINISHED     = 1 << 5,
    CHAT_CLIENT_EVENT_SEND_REJECTED     = 1 << 6,
    CHAT_CLIENT_EVENT_DISCONNECTED      = 1 << 7,
    CHAT_CLIENT_EVENT_CLOSED            = 1 << 8
} eCHAT_CLIENT_EVENT_TYPE;


typedef void sCHAT_CLIENT_CBACK_DATA;


typedef void (*fCHAT_CLIENT_USER_CBACK) (
    void*                    user_arg,
    eCHAT_CLIENT_EVENT_TYPE  event,
    sCHAT_CLIENT_CBACK_DATA* data);


typedef void* CHAT_CLIENT;


// Functions -------------------------------------------------------------------

eSTATUS chat_client_create(
    CHAT_CLIENT*            out_new_client,
    sMODULE_PARAMETERS      chat_client_params,
    sMODULE_PARAMETERS      chat_client_io_params,
    fCHAT_CLIENT_USER_CBACK user_cback,
    void*                   user_arg);


eSTATUS chat_client_connect(
    CHAT_CLIENT client,
    const char* address,
    uint16_t    port);


eSTATUS chat_client_send_text(
    CHAT_CLIENT client,
    const char* text);


eSTATUS chat_client_disconnect(
    CHAT_CLIENT client);


eSTATUS chat_client_close(
    CHAT_CLIENT client);


#ifdef __cplusplus
}
#endif
