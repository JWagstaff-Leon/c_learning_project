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

#define NETWORK_WATCHER_CANCEL_PIPE_INDEX NETWORK_WATCHER_MAX_FD_COUNT
#define NETWORK_WATCHER_CLOSE_PIPE_INDEX  NETWORK_WATCHER_MAX_FD_COUNT + 1


// Types -----------------------------------------------------------------------

typedef enum
{
    PIPE_END_READ  = 0,
    PIPE_END_WRITE = 1
} ePIPE_END;


typedef struct
{
    // Controlled by the module thread
    bool open;
    bool watching;

    int control_pipe[2];

    sNETWORK_WATCHER_WATCH* watches;
    uint32_t                watch_count; // Holds the number of watches; DOES NOT include the control pipe
    uint32_t                watches_size; // Holds the current size of watches; used to know if reallocation is needed; DOES NOT include the control pipe

    struct pollfd* fds;
    struct pollfd  control_fd;

    pthread_mutex_t watch_mutex;
    pthread_mutex_t control_mutex;

    fNETWORK_WATCHER_USER_CBACK user_cback;
    void*                       user_arg;
} sNETWORK_WATCHER_CBLK;


// Functions -------------------------------------------------------------------

void* network_watcher_thread_entry(
    void* arg);


#ifdef __cplusplus
}
#endif
