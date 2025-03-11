#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_CONNECTION_MESSAGE_QUEUE_SIZE 32


// Includes --------------------------------------------------------------------

#include "chat_clients_fsm.h"

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
    CHAT_CLIENTS_STATE_OPEN,
    CHAT_CLIENTS_STATE_CLOSED
} eCHAT_CLIENTS_STATE;


typedef enum {
    CHAT_CLIENTS_CLIENT_STATE_INIT,
    CHAT_CLIENTS_CLIENT_STATE_USERNAME_ENTRY,
    CHAT_CLIENTS_CLIENT_STATE_PASSWORD_ENTRY,
    CHAT_CLIENTS_CLIENT_STATE_ACTIVE
}
eCHAT_CLIENTS_CLIENT_STATE;


typedef struct
{
    eCHAT_CLIENTS_CLIENT_STATE state;
    CHAT_CONNECTION            connection;
    sCHAT_USER                 user_info;
} sCHAT_CLIENTS_CLIENT;


typedef struct
{
    MESSAGE_QUEUE       message_queue;
    eCHAT_CLIENTS_STATE state;

    fCHAT_CLIENTS_USER_CBACK user_cback;
    void*                    user_arg;

    sCHAT_CLIENTS_CLIENT* client_list;
    uint32_t              client_count;
    uint32_t              max_clients;
} sCHAT_CLIENTS_CBLK;


// Constants -------------------------------------------------------------------



// Functions -------------------------------------------------------------------

void* chat_clients_thread_entry(
    void* arg);


void chat_clients_process_event(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    const sCHAT_EVENT*      event);


eSTATUS chat_clients_accept_new_connection(
    int  listen_fd,
    int* out_new_fd);


#ifdef __cplusplus
}
#endif
