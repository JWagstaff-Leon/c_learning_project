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


typedef enum
{
    NETWORK_WATCHER_MODE_READ  = 1 << 0,
    NETWORK_WATCHER_MODE_WRITE = 1 << 1
} bNETWORK_WATCHER_MODE;


typedef enum
{
    NETWORK_WATCHER_EVENT_WATCH_COMPLETE  = 1 << 0,
    NETWORK_WATCHER_EVENT_WATCH_ERROR     = 1 << 1,
    NETWORK_WATCHER_EVENT_WATCH_CANCELLED = 1 << 2,

    NETWORK_WATCHER_EVENT_CLOSED = 1 << 31
} bNETWORK_WATCHER_EVENT_TYPE;


typedef struct
{
    bNETWORK_WATCHER_MODE modes_ready;
} sNETWORK_WATCHER_CBACK_DATA_WATCH_COMPLETE;


typedef union
{
    sNETWORK_WATCHER_CBACK_DATA_WATCH_COMPLETE watch_complete;
} sNETWORK_WATCHER_CBACK_DATA;


typedef void (*fNETWORK_WATCHER_USER_CBACK) (
    void*                              user_arg,
    bNETWORK_WATCHER_EVENT_TYPE        event_mask,
    const sNETWORK_WATCHER_CBACK_DATA* data);


typedef void* NETWORK_WATCHER;


// Functions -------------------------------------------------------------------

eSTATUS network_watcher_create(
    NETWORK_WATCHER*            out_new_network_watcher,
    fNETWORK_WATCHER_USER_CBACK user_cback,
    void*                       user_arg);


eSTATUS network_watcher_open(
    NETWORK_WATCHER network_watcher);


eSTATUS network_watcher_start_watch(
    NETWORK_WATCHER       network_watcher,
    bNETWORK_WATCHER_MODE mode,
    int                   fd);


eSTATUS network_watcher_cancel_watch(
    NETWORK_WATCHER network_watcher);


eSTATUS network_watcher_close(
    NETWORK_WATCHER network_watcher);


eSTATUS network_watcher_destroy(
    NETWORK_WATCHER network_watcher);


#ifdef __cplusplus
}
#endif
