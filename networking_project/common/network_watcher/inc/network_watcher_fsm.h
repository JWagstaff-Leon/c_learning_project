#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "network_watcher.h"
#include "network_watcher_internal.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    NETWORK_WATCHER_MESSAGE_TYPE_START_WATCH
} eNETWORK_WATCHER_MESSAGE_TYPE;


typedef union
{
    void* placeholder;
} uNETWORK_WATCHER_MESSAGE_PARAMS;


typedef struct
{
    eNETWORK_WATCHER_MESSAGE_TYPE   type;
    uNETWORK_WATCHER_MESSAGE_PARAMS params;
} sNETWORK_WATCHER_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
