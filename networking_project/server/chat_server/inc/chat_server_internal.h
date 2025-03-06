#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_SERVER_DEFAULT_MAX_CONNECTIONS 8

#define CHAT_SERVER_MESSAGE_QUEUE_SIZE 32


// Includes --------------------------------------------------------------------

#include "chat_server.h"

#include <stdint.h>

#include "chat_connections.h"
#include "common_types.h"
#include "message_queue.h"


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_STATE_INIT = 0,
    CHAT_SERVER_STATE_OPEN,
    CHAT_SERVER_STATE_CLOSED
} eCHAT_SERVER_STATE;


typedef struct
{
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    eCHAT_SERVER_STATE state;
    MESSAGE_QUEUE      message_queue;

    CHAT_CONNECTIONS connections;
} sCHAT_SERVER_CBLK;


// Functions -------------------------------------------------------------------

void* chat_server_thread_entry(
    void* arg);


void chat_server_connections_cback(
    void*                               user_arg,
    bCHAT_CONNECTIONS_EVENT_TYPE        event_mask,
    const uCHAT_CONNECTIONS_CBACK_DATA* data);


#ifdef __cplusplus
}
#endif
