#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>


// Constants -------------------------------------------------------------------

#define CHAT_EVENT_MAX_LENGTH 65535


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
    CHAT_EVENT_USERNAME_ACCEPTED,
    CHAT_EVENT_USERNAME_REJECTED,

    CHAT_EVENT_SERVER_SHUTDOWN,

    CHAT_EVENT_USER_LIST,
    CHAT_EVENT_USER_JOIN,
    CHAT_EVENT_USER_LEAVE,

    CHAT_EVENT_MAX // NOTE add all event types above this

} eCHAT_EVENT_TYPE;


typedef struct
{
    eCHAT_EVENT_TYPE event_type;
    uint32_t         event_origin;

    uint16_t event_length;
    uint8_t  event_data[CHAT_EVENT_MAX_LENGTH];
} sCHAT_EVENT;


#define CHAT_EVENT_HEADER_SIZE offsetof(sCHAT_EVENT, event_data)

// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
