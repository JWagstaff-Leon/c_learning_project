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

#include "chat_connection.h"
#include "chat_event.h"
#include "chat_event_io.h"
#include "chat_user.h"
#include "message_queue.h"
#include "network_watcher.h"


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CONNECTIONS_STATE_OPEN,
    CHAT_CONNECTIONS_STATE_CLOSED
} eCHAT_CONNECTIONS_STATE;


typedef enum {
    CHAT_CONNECTIONS_CLIENT_STATE_INIT,
    CHAT_CONNECTIONS_CLIENT_STATE_USERNAME_ENTRY,
    CHAT_CONNECTIONS_CLIENT_STATE_PASSWORD_ENTRY,
    CHAT_CONNECTIONS_CLIENT_STATE_ACTIVE
}
eCHAT_CONNECTIONS_CLIENT_STATE;


typedef struct
{
    eCHAT_CONNECTIONS_CLIENT_STATE state;
    CHAT_CONNECTION                connection;
    sCHAT_USER                     user_info;
} sCHAT_CONNECTIONS_CLIENT;


typedef struct
{
    MESSAGE_QUEUE           message_queue;
    eCHAT_CONNECTIONS_STATE state;

    fCHAT_CONNECTIONS_USER_CBACK user_cback;
    void*                        user_arg;

    sCHAT_CONNECTIONS_CLIENT* client_list;
    uint32_t                  client_count;
    uint32_t                  max_clients;
} sCHAT_CONNECTIONS_CBLK;


// Constants -------------------------------------------------------------------



// Functions -------------------------------------------------------------------

void* chat_connections_thread_entry(
    void* arg);


void chat_connections_process_event(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr,
    const sCHAT_EVENT*      event);


eSTATUS chat_connections_accept_new_connection(
    int  listen_fd,
    int* out_new_fd);


#ifdef __cplusplus
}
#endif
