#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "network_watcher.h"

#include <stdint.h>


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    NETWORK_WATCHER_MESSAGE_WATCH,
    NETWORK_WATCHER_MESSAGE_CLOSE
} eNETWORK_WATCHER_MESSAGE_TYPE;


typedef struct
{
    int                   fd;
    bNETWORK_WATCHER_MODE mode;
} sNETWORK_WATCHER_NEW_WATCH_PARAMS;


typedef union
{
    sNETWORK_WATCHER_NEW_WATCH_PARAMS new_watch;
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
