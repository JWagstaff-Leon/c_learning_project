#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_connection.h"
#include "chat_connection_fsm.h"

#include "chat_event_io.h"
#include "chat_user.h"
#include "message_queue.h"
#include "network_watcher.h"


// Constants -------------------------------------------------------------------

#define CHAT_CONNECTION_MESSAGE_QUEUE_SIZE  8
#define CHAT_CONNECTION_EVENT_QUEUE_SIZE   32


// Types -----------------------------------------------------------------------

typedef enum {
    CHAT_CONNECTION_STATE_OPEN,
    CHAT_CONNECTION_STATE_CANCELLING,
    CHAT_CONNECTION_STATE_CLOSING,
    CHAT_CONNECTION_STATE_CLOSED
} eCHAT_CONNECTION_STATE;


typedef struct
{
    eCHAT_CONNECTION_STATE state;
    int                    fd;
    CHAT_EVENT_IO          io;

    fCHAT_CONNECTION_USER_CBACK user_cback;
    void*                       user_arg;

    MESSAGE_QUEUE   message_queue;    // message queue for FSM operations
    MESSAGE_QUEUE   event_queue;      // message queue for outgoing events
    NETWORK_WATCHER network_watcher;
} sCHAT_CONNECTION_CBLK;


// Functions -------------------------------------------------------------------

void* chat_connection_thread_entry(
    void* arg);
    

void chat_connection_watcher_cback(
    void*                              arg,
    bNETWORK_WATCHER_EVENT_TYPE        event_mask,
    const sNETWORK_WATCHER_CBACK_DATA* data);

#ifdef __cplusplus
}
#endif
