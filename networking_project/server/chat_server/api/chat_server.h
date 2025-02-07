#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include "common_types.h"


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_EVENT_OPENED      = 1 << 0,
    CHAT_SERVER_EVENT_OPEN_FAILED = 1 << 1,
    CHAT_SERVER_EVENT_CLOSED      = 1 << 2
} eCHAT_SERVER_EVENT_TYPE;


typedef void uCHAT_SERVER_CBACK_DATA;


typedef void (*fCHAT_SERVER_USER_CBACK) (
    void*                    user_arg,
    eCHAT_SERVER_EVENT_TYPE  event,
    uCHAT_SERVER_CBACK_DATA* data);


typedef enum
{
    PLACEHOLDER
} eCONNECTION_EVENT;


typedef void* CHAT_SERVER;


// Functions -------------------------------------------------------------------

eSTATUS chat_server_create(
    CHAT_SERVER*            out_new_chat_server,
    fCHAT_SERVER_USER_CBACK user_cback,
    void*                   user_arg);


eSTATUS chat_server_open(
    CHAT_SERVER chat_server);


eSTATUS chat_server_reset(
    CHAT_SERVER chat_server);


eSTATUS chat_server_close(
    CHAT_SERVER chat_server);


#ifdef __cplusplus
}
#endif
