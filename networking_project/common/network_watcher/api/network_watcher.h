#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum 
{
    NETWORK_WATCHER_EVENT_READ_READY
} eNETWORK_WATCHER_EVENT_TYPE;


typedef struct
{
    struct pollfd* pollfds;
    nfds_t         nfds;
} sNETWORK_WATCHER_CBACK_DATA;


typedef void (fNETWORK_WATCHER_USER_CBACK*) (
    void*                        arg,
    eNETWORK_WATCHER_EVENT_TYPE  event,
    sNETWORK_WATCHER_CBACK_DATA* data);


typedef void* NETWORK_WATCHER;


// Functions -------------------------------------------------------------------

eSTATUS network_watcher_create(
    NETWORK_WATCHER*            out_new_network_watcher,
    sMODULE_PARAMETERS          network_watcher_params,
    fNETWORK_WATCHER_USER_CBACK user_cback,
    void*                       user_arg);


void* network_watcher_thread_entry(
    void* arg);


#ifdef __cplusplus
}
#endif
