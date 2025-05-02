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
    CHAT_AUTH_STATE_NO_DATABASE,
    CHAT_AUTH_STATE_OPEN,
    CHAT_AUTH_STATE_CLOSED
} eCHAT_AUTH_STATE;


typedef struct
{
    eCHAT_AUTH_STATE state;
    MESSAGE_QUEUE    message_queue;

    fCHAT_AUTH_USER_CBACK user_cback;
    void*                 user_arg;
} sCHAT_AUTH_CBLK;

// Functions -------------------------------------------------------------------

void* chat_auth_thread_entry(
    void* arg);


eSTATUS chat_auth_read_database_file(
    sCHAT_AUTH_CBLK* master_cblk_ptr,
    const char*      path);

    
eSTATUS chat_auth_save_database_file(
    sCHAT_AUTH_CBLK* master_cblk_ptr,
    const char*      path);

#ifdef __cplusplus
}
#endif
