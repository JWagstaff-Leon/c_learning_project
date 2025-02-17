#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_CONNECTION_MAX_NAME_SIZE      65
#define CHAT_CONNECTION_MESSAGE_QUEUE_SIZE 32


// Includes --------------------------------------------------------------------

#include "chat_connections_fsm.h"

#include <poll.h>
#include <stddef.h>
#include <stdint.h>

#include "chat_event.h"
#include "chat_event_io.h"
#include "message_queue.h"
#include "network_watcher.h"


// Types -----------------------------------------------------------------------

typedef enum {
    CHAT_CONNECTION_STATE_DISCONNECTED,
    CHAT_CONNECTION_STATE_INIT,
    CHAT_CONNECTION_STATE_SETUP,
    CHAT_CONNECTION_STATE_ACTIVE
} eCHAT_CONNECTION_STATE;


typedef struct
{
    int                    fd;
    CHAT_EVENT_IO          io;
    eCHAT_CONNECTION_STATE state;
    char                   name[CHAT_CONNECTION_MAX_NAME_SIZE];  // TODO make a chat user object which has at least a name and id
    MESSAGE_QUEUE          event_queue;
} sCHAT_CONNECTION;


typedef enum
{
    CHAT_CONNECTIONS_STATE_OPEN,
    CHAT_CONNECTIONS_STATE_CLOSED
} eCHAT_CONNECTIONS_STATE;


typedef struct
{
    MESSAGE_QUEUE           message_queue;
    eCHAT_CONNECTIONS_STATE state;

    fCHAT_CONNECTIONS_USER_CBACK user_cback;
    void*                        user_arg;

    sCHAT_CONNECTION* connections;
    uint32_t          connection_count;
    uint32_t          max_connections;

    int* read_fd_buffer;
    int* write_fd_buffer;

    NETWORK_WATCHER read_network_watcher;
    NETWORK_WATCHER write_network_watcher;
} sCHAT_CONNECTIONS_CBLK;


// Constants -------------------------------------------------------------------

static const sCHAT_CONNECTION k_blank_connection = {
    .fd          = -1,
    .io          = NULL,
    .state       = CHAT_CONNECTION_STATE_DISCONNECTED,
    .name        = { 0 },
    .event_queue = NULL
};


// Functions -------------------------------------------------------------------

void* chat_connections_thread_entry(
    void* arg);


void chat_connections_network_watcher_read_cback(
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const sNETWORK_WATCHER_CBACK_DATA* data);


void chat_connections_network_watcher_write_cback(
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const sNETWORK_WATCHER_CBACK_DATA* data);


#ifdef __cplusplus
}
#endif
