#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_SERVER_DEFAULT_MAX_CONNECTIONS 8

#define CHAT_SERVER_MESSAGE_QUEUE_SIZE 32


// Includes --------------------------------------------------------------------

#include "chat_server.h"
#include "chat_server_connections.h"

#include <stdint.h>

#include "chat_event_io.h"
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
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    eCHAT_SERVER_STATE state;
    MESSAGE_QUEUE      message_queue;

    // TODO add a mechanism for tracking which connections need to be watched for writing
    sCHAT_SERVER_CONNECTIONS connections;
    NETWORK_WATCHER          read_network_watcher;
    NETWORK_WATCHER          write_network_watcher;
} sCHAT_SERVER_CBLK;


// Functions -------------------------------------------------------------------

void* chat_server_thread_entry(
    void* arg);


eSTATUS chat_server_network_open(
    sCHAT_SERVER_CBLK* master_cblk_ptr);


eSTATUS chat_server_network_poll(
    CHAT_CONNECTION connections);


eSTATUS chat_server_process_connections_events(
    CHAT_CONNECTION connections);


void chat_server_network_close(
    CHAT_CONNECTION connections);


eSTATUS chat_server_network_start_network_watch(
    sCHAT_SERVER_CBLK* master_cblk_ptr);

    
eSTATUS chat_server_network_stop_network_watch(
    sCHAT_SERVER_CBLK* master_cblk_ptr);


void chat_server_network_watcher_read_cback(
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const sNETWORK_WATCHER_CBACK_DATA* data);


void chat_server_network_watcher_write_cback(
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const sNETWORK_WATCHER_CBACK_DATA* data);

#ifdef __cplusplus
}
#endif
