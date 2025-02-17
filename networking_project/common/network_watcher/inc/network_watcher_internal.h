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
    bool open;
    bool watching;

    int cancel_pipe[2];
    int close_pipe[2];

    const int** fd_containers;
    uint32_t    fd_count;
    uint32_t    max_fds; // holds the number of fds; DOES NOT include the 2 control pipes


    struct pollfd* fds;

    pthread_mutex_t watch_mutex;
    pthread_cond_t  watch_condvar;

    fNETWORK_WATCHER_USER_CBACK user_cback;
    void*                       user_arg;
} sNETWORK_WATCHER_CBLK;


// Functions -------------------------------------------------------------------

void* network_watcher_thread_entry(
    void* arg);


#ifdef __cplusplus
}
#endif
