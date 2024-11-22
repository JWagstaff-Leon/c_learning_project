#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <poll.h>
#include <stdint.h>

#include "chat_event.h"


// Types -----------------------------------------------------------------------

typedef enum {
    CHAT_SERVER_CONNECTION_STATE_DISCONNECTED,
    CHAT_SERVER_CONNECTION_STATE_CONNECTED,
    CHAT_SERVER_CONNECTION_STATE_IN_SETUP,
    CHAT_SERVER_CONNECTION_STATE_ACTIVE
} eCHAT_SERVER_CONNECTION_STATE;


typedef enum
{
    CHAT_SERVER_CONNECTION_READ_STATE_NEW,
    CHAT_SERVER_CONNECTION_READ_STATE_HEADER,
    CHAT_SERVER_CONNECTION_READ_STATE_DATA,
    CHAT_SERVER_CONNECTION_READ_STATE_DONE,

    CHAT_SERVER_CONNECTION_READ_STATE_FLUSHING,
    CHAT_SERVER_CONNECTION_READ_STATE_FLUSHED
} eCHAT_SERVER_CONNECTION_READ_STATE;


typedef struct
{
    sCHAT_EVENT buffer;
    uint32_t    read_bytes;
    uint32_t    expected_bytes;

    eCHAT_SERVER_CONNECTION_READ_STATE state;
} sCHAT_SERVER_CONNECTION_READ_INFO;


typedef struct
{
    eCHAT_SERVER_CONNECTION_STATE state;
    struct pollfd                 pollfd;

    sCHAT_SERVER_CONNECTION_READ_INFO read_info;

    unsigned char *name;
    uint32_t       name_length;
} sCHAT_SERVER_CONNECTION;


typedef struct
{
    sCHAT_SERVER_CONNECTION *list;
    uint32_t                 count;
    uint32_t                 size;
} sCHAT_SERVER_CONNECTIONS;


// Constants -------------------------------------------------------------------

static const sCHAT_SERVER_CONNECTION k_new_user = {
    .name = NULL,
    .name_length = 0,
    .pollfd = {
        .fd = -1,
        .events = 0,
        .revents = 0
    },
    .state = CHAT_USER_STATE_DISCONNECTED
};


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
