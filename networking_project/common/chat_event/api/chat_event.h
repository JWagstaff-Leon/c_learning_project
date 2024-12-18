#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------

#define CHAT_EVENT_MAX_DATA_SIZE 1024

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
    uint8_t  data[CHAT_EVENT_MAX_DATA_SIZE];
} sCHAT_EVENT;


typedef struct
{
    sCHAT_EVENT event;
    uint32_t    processed_bytes;
    uint32_t    flush_bytes;
} sCHAT_EVENT_IO;


#define CHAT_EVENT_HEADER_SIZE offsetof(sCHAT_EVENT, data)


// Functions -------------------------------------------------------------------

eSTATUS chat_event_io_init(
    sCHAT_EVENT_IO* chat_event_io);


eSTATUS chat_event_io_read_from_fd(
    sCHAT_EVENT_IO* reader,
    int             fd);

    
eSTATUS chat_event_io_extract_read_event(
    sCHAT_EVENT_IO* restrict reader,
    sCHAT_EVENT*    restrict out_event);


eSTATUS chat_event_io_write_to_fd(
    sCHAT_EVENT_IO* writer,
    int             fd);


#ifdef __cplusplus
}
#endif
