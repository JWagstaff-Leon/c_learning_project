#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <pthread.h>
#include <stdint.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef struct
{
    // User-controlled
    int* fd_ptr; // pointer to the fd; used to get the fd-containing object back
    bool active; // marks the fd as active; used to prevent watching

    // Module-controlled
    bool ready;  // marks if the fd is ready
    bool closed; // marks if the fd has closed
} sNETWORK_WATCHER_WATCH;


typedef enum
{
    NETWORK_WATCHER_MODE_READ,
    NETWORK_WATCHER_MODE_WRITE
} eNETWORK_WATCHER_MODE;


typedef enum
{
    NETWORK_WATCHER_EVENT_WATCH_COMPLETE,
    NETWORK_WATCHER_EVENT_WATCH_ERROR,
    NETWORK_WATCHER_EVENT_CANCELLED,
    NETWORK_WATCHER_EVENT_CLOSED
} eNETWORK_WATCHER_EVENT_TYPE;


typedef void uNETWORK_WATCHER_CBACK_DATA;


typedef void (fNETWORK_WATCHER_USER_CBACK*) (
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const uNETWORK_WATCHER_CBACK_DATA* data);


typedef void* NETWORK_WATCHER;


// Functions -------------------------------------------------------------------

eSTATUS network_watcher_create(
    NETWORK_WATCHER*            out_new_network_watcher,
    fNETWORK_WATCHER_USER_CBACK user_cback,
    void*                       user_arg);


eSTATUS network_watcher_start_watch(
    NETWORK_WATCHER         network_watcher,
    eNETWORK_WATCHER_MODE   mode,
    sNETWORK_WATCHER_WATCH* watches,
    uint32_t                watch_count);


eSTATUS network_watcher_stop_watch(
    NETWORK_WATCHER network_watcher);


eSTATUS network_watcher_close(
    NETWORK_WATCHER network_watcher);


#ifdef __cplusplus
}
#endif
