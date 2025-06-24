#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_client.h"

#include "chat_connection.h"
#include "chat_user.h"
#include "message_queue.h"


// Constants -------------------------------------------------------------------

#define CHAT_CLIENT_MSG_QUEUE_SIZE 32


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENT_STATE_INIT,
    CHAT_CLIENT_STATE_CONNECTED,
    CHAT_CLIENT_STATE_USERNAME_ENTRY,
    CHAT_CLIENT_STATE_USERNAME_VERIFYING,
    CHAT_CLIENT_STATE_PASSWORD_ENTRY,
    CHAT_CLIENT_STATE_PASSWORD_VERIFYING,
    CHAT_CLIENT_STATE_AUTHENTICATED,
    CHAT_CLIENT_STATE_ACTIVE,
    CHAT_CLIENT_STATE_DISCONNECTING,
    CHAT_CLIENT_STATE_CLOSED
} eCHAT_CLIENT_STATE;


typedef struct
{
    eCHAT_CLIENT_STATE state;
    MESSAGE_QUEUE      message_queue;

    fCHAT_CLIENT_USER_CBACK user_cback;
    void*                   user_arg;

    int             server_fd;
    CHAT_CONNECTION server_connection;

    sCHAT_USER user_info;
} sCHAT_CLIENT_CBLK;


// Functions -------------------------------------------------------------------

void* chat_client_thread_entry(
    void* arg);


void chat_client_handle_incoming_event(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    const sCHAT_EVENT* event);


void chat_client_connection_cback(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const sCHAT_CONNECTION_CBACK_DATA* data);


#ifdef __cplusplus
}
#endif
