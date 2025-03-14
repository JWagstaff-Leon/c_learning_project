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
    CHAT_CLIENTS_STATE_CLOSING,
    CHAT_CLIENTS_STATE_CLOSED
} eCHAT_CLIENTS_STATE;


typedef enum
{
    CHAT_CLIENT_STATE_INIT,
    CHAT_CLIENT_STATE_AUTHENTICATING,
    CHAT_CLIENT_STATE_ACTIVE
} eCHAT_CLIENT_STATE;


typedef enum
{
    CHAT_CLIENT_STATE_USERNAME_ENTRY,
    CHAT_CLIENT_STATE_PASSWORD_ENTRY
} eCHAT_CLIENT_AUTH_SUBSTATE;


typedef struct
{
    pthread_mutex_t mutex;

    bool active;   // Set internally to determine if the auth operation still needs to be done
    bool complete; // Set externally to determine if the auth operation is complete

    sCHAT_USER                user_info;   // Set externally to the resultant user info on completed auth
    eCHAT_CLIENTS_AUTH_RESULT auth_result; // Set externally to what the result of the auth operation was

    void* client_ptr;
} sCHAT_CLIENT_AUTH_OPERATION;


typedef struct
{
    eCHAT_CLIENT_STATE           state;
    sCHAT_CLIENT_AUTH_OPERATION* auth;

    CHAT_CONNECTION connection;
    sCHAT_USER      user_info;
} sCHAT_CLIENT;


typedef struct
{
    MESSAGE_QUEUE       message_queue;
    eCHAT_CLIENTS_STATE state;

    fCHAT_CLIENTS_USER_CBACK user_cback;
    void*                    user_arg;

    NETWORK_WATCHER new_connection_watch;

    sCHAT_CLIENT* client_list;
    uint32_t      client_count;
    uint32_t      max_clients;
} sCHAT_CLIENTS_CBLK;


typedef struct
{
    sCHAT_CLIENTS_CBLK* master_cblk_ptr;
    sCHAT_CLIENT*       client_ptr;
} sCHAT_CLIENTS_CLIENT_CBACK_ARG;


// Constants -------------------------------------------------------------------



// Functions -------------------------------------------------------------------

void* chat_clients_thread_entry(
    void* arg);


void chat_clients_process_event(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       source_client,
    const sCHAT_EVENT*  event);


eSTATUS chat_clients_accept_new_connection(
    int  listen_fd,
    int* out_new_fd);


eSTATUS chat_clients_client_open(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    sCHAT_CLIENT*       client,
    int                 fd);


void chat_clients_new_connections_cback(
    void*                              user_arg,
    bNETWORK_WATCHER_EVENT_TYPE        event_mask,
    const sNETWORK_WATCHER_CBACK_DATA* data);


void chat_clients_connection_cback(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const uCHAT_CONNECTION_CBACK_DATA* data);


#ifdef __cplusplus
}
#endif
