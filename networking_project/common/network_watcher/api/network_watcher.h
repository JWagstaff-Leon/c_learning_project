#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    NETOWRK_WATCHER_MODE_READ,
    NETOWRK_WATCHER_MODE_WRITE
} eNETWORK_WATCHER_MODE;


typedef enum 
{
    NETWORK_WATCHER_EVENT_READY,
    NETWORK_WATCHER_EVENT_CANCELED,
    NETWORK_WATCHER_EVENT_CLOSED
} eNETWORK_WATCHER_EVENT_TYPE;


typedef struct
{
    uint32_t* connection_indecies;
    uint32_t  index_count;
}sNETWORK_WATCHER_CBACK_READY_DATA;


typedef struct
{
    sNETWORK_WATCHER_CBACK_READY_DATA ready;
} sNETWORK_WATCHER_CBACK_DATA;


typedef void (fNETWORK_WATCHER_USER_CBACK*) (
    void*                              arg,
    eNETWORK_WATCHER_EVENT_TYPE        event,
    const sNETWORK_WATCHER_CBACK_DATA* data);


typedef void* NETWORK_WATCHER;


// Functions -------------------------------------------------------------------

eSTATUS network_watcher_create(
    NETWORK_WATCHER*            out_new_network_watcher,
    fNETWORK_WATCHER_USER_CBACK user_cback,
    void*                       user_arg);


eSTATUS network_watcher_start_watch(
    NETWORK_WATCHER restrict network_watcher,
    eNETWORK_WATCHER_MODE    mode,
    int*            restrict fd_list,
    uint32_t                 fd_count);


eSTATUS network_watcher_stop_watch(
    NETWORK_WATCHER network_watcher);


eSTATUS network_watcher_close(
    NETWORK_WATCHER network_watcher);


// TODO add means of changing the fds

#ifdef __cplusplus
}
#endif
