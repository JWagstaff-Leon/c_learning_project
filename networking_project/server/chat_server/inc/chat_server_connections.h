#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_SERVER_CONNECTION_MAX_NAME_SIZE      65
#define CHAT_SERVER_CONNECTION_MESSAGE_QUEUE_SIZE 32


// Includes --------------------------------------------------------------------

#include <poll.h>
#include <stddef.h>
#include <stdint.h>

#include "chat_event.h"
#include "chat_event_io.h"


// Types -----------------------------------------------------------------------

typedef enum {
    CHAT_SERVER_CONNECTION_STATE_DISCONNECTED,
    CHAT_SERVER_CONNECTION_STATE_CONNECTED,
    CHAT_SERVER_CONNECTION_STATE_IN_SETUP,
    CHAT_SERVER_CONNECTION_STATE_ACTIVE
} eCHAT_SERVER_CONNECTION_STATE;


typedef enum
{
    CHAT_SERVER_CONNECTION_MESSAGE_SEND_EVENT
} eCHAT_SERVER_CONNECTION_MESSAGE_TYPE;


typedef struct
{
    const sCHAT_EVENT* event;
} sCHAT_SERVER_CONNECTION_SEND_EVENT_PARAMS;


typedef union
{
    sCHAT_SERVER_CONNECTION_SEND_EVENT_PARAMS send_event;
} uCHAT_SERVER_CONNECTION_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_SERVER_CONNECTION_MESSAGE_TYPE   type;
    uCHAT_SERVER_CONNECTION_MESSAGE_PARAMS params;
} sCHAT_SERVER_CONNECTION_MESSAGE;


// TODO add user callback
typedef struct
{
    int fd;

    eCHAT_SERVER_CONNECTION_STATE state;
    MESSAGE_QUEUE                 message_queue;

    CHAT_EVENT_IO event_reader;
    CHAT_EVENT_IO event_writer;

    char name[CHAT_SERVER_CONNECTION_MAX_NAME_SIZE];
} sCHAT_SERVER_CONNECTION;


typedef struct
{
    sCHAT_SERVER_CONNECTION* list;
    uint32_t                 count;
    uint32_t                 size;
} sCHAT_SERVER_CONNECTIONS;


// Constants -------------------------------------------------------------------

static const sCHAT_SERVER_CONNECTION k_blank_user = {
    .fd = -1,
    .state = CHAT_SERVER_CONNECTION_STATE_DISCONNECTED,
    .name = ""
};


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
