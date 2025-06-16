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
#include "shared_ptr.h"


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
    CHAT_CLIENT_STATE_AUTHENTICATING_PROCESSING,

    CHAT_CLIENT_STATE_ACTIVE,
    CHAT_CLIENT_STATE_DISCONNECTING
} eCHAT_CLIENT_STATE;


typedef struct
{
    eCHAT_CLIENT_STATE state;

    void*      auth_transaction;
    SHARED_PTR auth_credentials_ptr;

    CHAT_CONNECTION connection;
    sCHAT_USER      user_info;

    void* master_cblk_ptr;

    SHARED_PTR prev;
    SHARED_PTR next;
} sCHAT_CLIENT;


typedef struct
{
    MESSAGE_QUEUE       message_queue;
    eCHAT_CLIENTS_STATE state;

    fCHAT_CLIENTS_USER_CBACK user_cback;
    void*                    user_arg;

    SHARED_PTR* client_list_head;
    SHARED_PTR* client_list_tail;
} sCHAT_CLIENTS_CBLK;



// Constants -------------------------------------------------------------------



// Functions -------------------------------------------------------------------

void* chat_clients_thread_entry(
    void* arg);


void chat_clients_process_event(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          source_client_ptr,
    const sCHAT_EVENT*  event);


eSTATUS chat_clients_introduce_user(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          client_ptr);


eSTATUS chat_clients_client_create(
    SHARED_PTR*         out_new_client_ptr,
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    int                 fd);


eSTATUS chat_clients_client_close(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    SHARED_PTR          client_ptr);


void chat_clients_connection_cback(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const sCHAT_CONNECTION_CBACK_DATA* data);


#ifdef __cplusplus
}
#endif
