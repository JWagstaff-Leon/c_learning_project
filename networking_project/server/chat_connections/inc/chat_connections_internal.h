#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_CONNECTION_MESSAGE_QUEUE_SIZE 32


// Includes --------------------------------------------------------------------

#include "chat_connections_fsm.h"

#include <poll.h>
#include <stddef.h>
#include <stdint.h>

#include "chat_event.h"
#include "chat_event_io.h"
#include "chat_user.h"
#include "message_queue.h"
#include "network_watcher.h"


// Types -----------------------------------------------------------------------

typedef enum {
    CHAT_CONNECTION_STATE_DISCONNECTED,
    CHAT_CONNECTION_STATE_INIT,
    CHAT_CONNECTION_STATE_USERNAME_ENTRY,
    CHAT_CONNECTION_STATE_PASSWORD_ENTRY,
    CHAT_CONNECTION_STATE_ACTIVE
} eCHAT_CONNECTION_STATE;


typedef struct
{
    int                    fd;
    CHAT_EVENT_IO          io;
    eCHAT_CONNECTION_STATE state;
    sCHAT_USER             user;

    MESSAGE_QUEUE   message_queue;
    NETWORK_WATCHER network_watcher;
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


    sNETWORK_WATCHER_WATCH* read_watches;
    sNETWORK_WATCHER_WATCH* write_watches;
} sCHAT_CONNECTIONS_CBLK;


// Constants -------------------------------------------------------------------

static const sCHAT_CONNECTION k_blank_connection = {
    .fd          = -1,
    .io          = NULL,
    .state       = CHAT_CONNECTION_STATE_DISCONNECTED,
    .user        =
    {
        .id = CHAT_USER_INVALID_ID,
        .name = { 0 }
    },
    .event_queue = NULL
};


// Functions -------------------------------------------------------------------

void* chat_connections_thread_entry(
    void* arg);


void chat_connections_process_event(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr,
    const sCHAT_EVENT*      event);


void chat_connections_network_watcher_read_cback(
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const sNETWORK_WATCHER_CBACK_DATA* data);


void chat_connections_network_watcher_write_cback(
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const sNETWORK_WATCHER_CBACK_DATA* data);


eSTATUS chat_connections_accept_new_connection(
    int  listen_fd,
    int* out_new_fd);


#ifdef __cplusplus
}
#endif
