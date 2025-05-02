#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "network_watcher.h"

#include <pthread.h>

#include "message_queue.h"


// Constants -------------------------------------------------------------------

#define NETWORK_WATCHER_MESSAGE_QUEUE_SIZE 8


// Types -----------------------------------------------------------------------

typedef enum
{
    NETWORK_WATCHER_STATE_OPEN,
    NETWORK_WATCHER_STATE_CLOSED
} eNETWORK_WATCHER_STATE;


typedef struct
{
    MESSAGE_QUEUE          message_queue;
    eNETWORK_WATCHER_STATE state;

    struct pollfd fds[2];

    int             cancel_pipe[2];
    pthread_mutex_t cancel_mutex;

    fNETWORK_WATCHER_USER_CBACK user_cback;
    void*                       user_arg;
} sNETWORK_WATCHER_CBLK;


// Functions -------------------------------------------------------------------

void* network_watcher_thread_entry(
    void* arg);


#ifdef __cplusplus
}
#endif
