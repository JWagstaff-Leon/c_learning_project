#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_SERVER_CONNECTION_MAX_NAME_SIZE 65


// Includes --------------------------------------------------------------------

#include <poll.h>
#include <stddef.h>
#include <stdint.h>

#include "chat_event.h"



// Types -----------------------------------------------------------------------

typedef enum {
    CHAT_SERVER_CONNECTION_STATE_DISCONNECTED,
    CHAT_SERVER_CONNECTION_STATE_CONNECTED,
    CHAT_SERVER_CONNECTION_STATE_IN_SETUP,
    CHAT_SERVER_CONNECTION_STATE_ACTIVE
} eCHAT_SERVER_CONNECTION_STATE;


typedef struct
{
    eCHAT_SERVER_CONNECTION_STATE state;
    struct pollfd                 pollfd;
    sCHAT_EVENT_IO                event_reader;
    
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
    .name = "",
    .pollfd = {
        .fd = -1,
        .events = 0,
        .revents = 0
    },
    .state = CHAT_SERVER_CONNECTION_STATE_DISCONNECTED
};


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
