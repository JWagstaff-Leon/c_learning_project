#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_auth.h"

#include "message_queue.h"


// Constants -------------------------------------------------------------------

#define CHAT_AUTH_MESSAGE_QUEUE_SIZE 32


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_AUTH_STATE_OPEN,
    CHAT_AUTH_STATE_CLOSED
} eCHAT_AUTH_STATE;


typedef struct
{
    eCHAT_AUTH_STATE state;
    MESSAGE_QUEUE    message_queue;
} sCHAT_AUTH_CBLK;

// Functions -------------------------------------------------------------------

void* chat_auth_thread_entry(
    void* arg);

#ifdef __cplusplus
}
#endif
