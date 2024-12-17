#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_client.h"

#include <poll.h>

#include "chat_event.h"
#include "message_queue.h"


// Constants -------------------------------------------------------------------

#define CHAT_CLIENT_MSG_QUEUE_SIZE 32


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENT_STATE_NOT_CONNECTED,
    CHAT_CLIENT_STATE_INACTIVE,
    CHAT_CLIENT_STATE_ACTIVE,
    CHAT_CLIENT_STATE_DISCONNECTING,
    CHAT_CLIENT_STATE_CLOSED
} eCHAT_CLIENT_STATE;


typedef struct
{

    MESSAGE_QUEUE           message_queue;
    fCHAT_CLIENT_USER_CBACK user_cback;
    void*                   user_arg;

    fGENERIC_ALLOCATOR   allocator;
    fGENERIC_DEALLOCATOR deallocator;

    struct pollfd server_connection;

    eCHAT_CLIENT_STATE state;
    sCHAT_EVENT_READER incoming_event_reader;
} sCHAT_CLIENT_CBLK;


// Functions -------------------------------------------------------------------

void* chat_client_thread_entry(
    void* arg);


eSTATUS chat_client_network_open(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    struct sockaddr_in address);


eSTATUS chat_client_network_send_username(
    int               server_fd,
    const char* const username);


eSTATUS chat_client_network_send_message(
    int               server_fd,
    const char* const message_text);

#ifdef __cplusplus
}
#endif
