#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_CONNECTION_MESSAGE_QUEUE_SIZE 32


// Includes --------------------------------------------------------------------

#include "chat_clients.h"

#include <poll.h>
#include <pthread.h>
#include <stddef.h>
#include <stdint.h>

#include "chat_auth.h"
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
    CHAT_CLIENTS_STATE_CLOSING,
    CHAT_CLIENTS_STATE_CLOSED
} eCHAT_CLIENTS_STATE;


typedef enum
{
    CHAT_CLIENT_STATE_INIT,

    CHAT_CLIENT_STATE_AUTHENTICATING_INIT,
    CHAT_CLIENT_STATE_AUTHENTICATING_USERNAME,
    CHAT_CLIENT_STATE_AUTHENTICATING_PASSWORD,

    CHAT_CLIENT_STATE_DISCONNECTING,
    CHAT_CLIENT_STATE_ACTIVE
} eCHAT_CLIENT_STATE;


typedef struct
{
    eCHAT_CLIENT_STATE state;

    void*                   auth_transaction;
    sCHAT_USER_CREDENTIALS* auth_credentials;

    CHAT_CONNECTION connection;
    sCHAT_USER      user_info;
} sCHAT_CLIENT;


typedef struct s_chat_client_entry
{
    sCHAT_CLIENT client;
    void* master_cblk_ptr;

    struct s_chat_client_entry* prev;
    struct s_chat_client_entry* next;
} sCHAT_CLIENT_ENTRY;


typedef struct
{
    MESSAGE_QUEUE       message_queue;
    eCHAT_CLIENTS_STATE state;

    fCHAT_CLIENTS_USER_CBACK user_cback;
    void*                    user_arg;

    sCHAT_CLIENT_ENTRY* client_list_head;
    sCHAT_CLIENT_ENTRY* client_list_tail;
} sCHAT_CLIENTS_CBLK;



// Constants -------------------------------------------------------------------



// Functions -------------------------------------------------------------------

void* chat_clients_thread_entry(
    void* arg);


void chat_clients_process_event(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event);


eSTATUS chat_clients_client_create(
    sCHAT_CLIENT_ENTRY** out_new_entry,
    sCHAT_CLIENTS_CBLK*  master_cblk_ptr,
    int                  fd);


void chat_clients_connection_cback(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const sCHAT_CONNECTION_CBACK_DATA* data);


#ifdef __cplusplus
}
#endif
