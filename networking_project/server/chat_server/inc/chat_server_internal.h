#pragma once

#ifdef __cplusplus
extern "C" {
#endif


// Includes --------------------------------------------------------------------

#include "chat_server.h"

#include <stdint.h>

#include "chat_auth.h"
#include "chat_clients.h"
#include "common_types.h"
#include "message_queue.h"
#include "network_watcher.h"


// Defines ---------------------------------------------------------------------

#ifndef CHAT_SERVER_DEFAULT_MAX_CONNECTIONS
#define CHAT_SERVER_DEFAULT_MAX_CONNECTIONS 8
#endif

#define CHAT_SERVER_MESSAGE_QUEUE_SIZE 32


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_STATE_OPEN,
    CHAT_SERVER_STATE_CLOSING,
    CHAT_SERVER_STATE_CLOSED
} eCHAT_SERVER_STATE;


typedef struct
{
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    eCHAT_SERVER_STATE state;
    MESSAGE_QUEUE      message_queue;

    CHAT_CLIENTS clients;
    CHAT_AUTH    auth;

    NETWORK_WATCHER connection_listener;
    int             listen_fd;
} sCHAT_SERVER_CBLK;


// Functions -------------------------------------------------------------------

void* chat_server_thread_entry(
    void* arg);


eSTATUS open_listen_socket(
    int* out_fd);


eSTATUS chat_server_accept_connection(
    sCHAT_SERVER_CBLK* master_cblk_ptr,
    int*               new_connection_fd);


void chat_server_clients_cback(
    void*                           user_arg,
    bCHAT_CLIENTS_EVENT_TYPE        event_mask,
    const sCHAT_CLIENTS_CBACK_DATA* data);


void chat_server_auth_cback(
    void*                  user_arg,
    bCHAT_AUTH_EVENT_TYPE  event_mask,
    const sCHAT_AUTH_CBACK_DATA* data);


void chat_server_connection_listener_cback(
    void*                              user_arg,
    bNETWORK_WATCHER_EVENT_TYPE        event_mask,
    const sNETWORK_WATCHER_CBACK_DATA* data);


#ifdef __cplusplus
}
#endif
