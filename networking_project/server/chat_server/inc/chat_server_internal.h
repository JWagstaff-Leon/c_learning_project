#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_SERVER_MAX_CONNECTIONS 8

#define CHAT_SERVER_MSG_QUEUE_SIZE 32


// Includes --------------------------------------------------------------------

#include "chat_server.h"
#include "chat_server_connections.h"

#include <stdint.h>

#include "common_types.h"
#include "message_queue.h"
#include "network_watcher.h"


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_STATE_INIT = 0,
    CHAT_SERVER_STATE_OPEN,
    CHAT_SERVER_STATE_CLOSED
} eCHAT_SERVER_STATE;


typedef struct
{
    fGENERIC_ALLOCATOR   allocator;
    fGENERIC_DEALLOCATOR deallocator;

    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    eCHAT_SERVER_STATE state;
    MESSAGE_QUEUE      message_queue;

    sCHAT_SERVER_CONNECTIONS connections;
    NETWORK_WATCHER          network_watcher;
} sCHAT_SERVER_CBLK;


// Functions -------------------------------------------------------------------

void* chat_server_thread_entry(
    void* arg);


eSTATUS chat_server_network_open(
    sCHAT_SERVER_CONNECTION* connection);


eSTATUS chat_server_network_poll(
    sCHAT_SERVER_CONNECTIONS* connections);


eSTATUS chat_server_process_connections_events(
    sCHAT_SERVER_CONNECTIONS* connections,
    fGENERIC_ALLOCATOR        allocator);


void chat_server_network_close(
    sCHAT_SERVER_CONNECTIONS* connections);


#ifdef __cplusplus
}
#endif
