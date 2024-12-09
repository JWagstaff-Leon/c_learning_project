#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------

#define CHAT_EVENT_MAX_LENGTH 65535

#define CHAT_EVENT_ORIGIN_SERVER 0


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
    eCHAT_EVENT_TYPE type;
    uint32_t         origin;

    uint16_t length;
    uint8_t  data[CHAT_EVENT_MAX_LENGTH];
} sCHAT_EVENT;


typedef enum
{
    CHAT_EVENT_READER_STATE_NEW,
    CHAT_EVENT_READER_STATE_HEADER,
    CHAT_EVENT_READER_STATE_DATA,
    CHAT_EVENT_READER_STATE_DONE,

    CHAT_EVENT_READER_STATE_FLUSHING,
    CHAT_EVENT_READER_STATE_FLUSHED
} eCHAT_EVENT_READER_STATE;


typedef struct
{
    sCHAT_EVENT event;
    uint32_t    read_bytes;
    uint32_t    expected_bytes;

    eCHAT_EVENT_READER_STATE state;
} sCHAT_EVENT_READER;

#define CHAT_EVENT_HEADER_SIZE offsetof(sCHAT_EVENT, data)

// Functions -------------------------------------------------------------------

eSTATUS chat_event_read_from_fd(
    int                 fd,
    sCHAT_EVENT_READER* reader);

#ifdef __cplusplus
}
#endif
