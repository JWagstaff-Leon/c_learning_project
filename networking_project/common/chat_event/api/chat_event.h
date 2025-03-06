#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include "chat_user.h"
#include "common_types.h"


// Constants -------------------------------------------------------------------

#define CHAT_EVENT_MAX_DATA_SIZE 1024


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_EVENT_MIN = 0,
    CHAT_EVENT_UNDEFINED = CHAT_EVENT_MIN,

    CHAT_EVENT_CHAT_MESSAGE,

    CHAT_EVENT_CONNECTION_FAILED,
    CHAT_EVENT_OVERSIZED_CONTENT,

    CHAT_EVENT_USERNAME_REQUEST,
    CHAT_EVENT_USERNAME_SUBMIT,
    CHAT_EVENT_USERNAME_REJECTED,

    CHAT_EVENT_PASSWORD_REQUEST,
    CHAT_EVENT_PASSWORD_SUBMIT,
    CHAT_EVENT_PASSWORD_REJECTED,

    CHAT_EVENT_AUTHENTICATED,

    CHAT_EVENT_USER_LIST,
    CHAT_EVENT_USER_JOIN,
    CHAT_EVENT_USER_LEAVE,

    CHAT_EVENT_SERVER_SHUTDOWN,

    CHAT_EVENT_MAX // NOTE add all event types above this

} eCHAT_EVENT_TYPE;


typedef struct
{
    eCHAT_EVENT_TYPE type;
    sCHAT_USER       origin;

    uint16_t length;
    uint8_t  data[CHAT_EVENT_MAX_DATA_SIZE];
} sCHAT_EVENT;


#define CHAT_EVENT_HEADER_SIZE offsetof(sCHAT_EVENT, data)


// Functions -------------------------------------------------------------------

eSTATUS chat_event_copy(
    sCHAT_EVENT*       restrict dst,
    const sCHAT_EVENT* restrict src);


#ifdef __cplusplus
}
#endif
