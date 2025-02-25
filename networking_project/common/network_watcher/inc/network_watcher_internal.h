#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "network_watcher.h"

#include <pthread.h>

#include "message_queue.h"


// Constants -------------------------------------------------------------------

#define NETWORK_WATCHER_MESSAGE_QUEUE_SIZE 32

// The timeout in milliseconds for poll timeouts
// The longer the timeout, the less responsive the watcher will be in the worst case
// The shorter the timeout, the more process intensive the watcher will be in the worst case
#define NETWORK_WATCHER_DEFAULT_POLL_TIMEOUT 1000


// Types -----------------------------------------------------------------------

typedef enum
{
    NETWORK_WATCHER_STATE_OPEN,
    NETWORK_WATCHER_STATE_ACTIVE,
    NETWORK_WATCHER_STATE_CLOSED
} eNETWORK_WATCHER_STATE;


typedef struct
{
    MESSAGE_QUEUE          message_queue;
    eNETWORK_WATCHER_STATE state;

    sNETWORK_WATCHER_WATCH* watches;
    uint32_t                watch_count;

    struct pollfd* fds;
    uint32_t       fds_size;
    int            poll_timeout;

    fNETWORK_WATCHER_USER_CBACK user_cback;
    void*                       user_arg;
} sNETWORK_WATCHER_CBLK;


// Functions -------------------------------------------------------------------

void* network_watcher_thread_entry(
    void* arg);


#ifdef __cplusplus
}
#endif
