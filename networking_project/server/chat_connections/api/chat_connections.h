#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "common_types.h"
#include "network_watcher.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CONNECTIONS_EVENT_INCOMING_EVENT,
    CHAT_CONNECTIONS_EVENT_CLOSED
} eCHAT_CONNECTIONS_EVENT_TYPE;


typedef struct
{

} sCHAT_CONNECTION_EVENT_DATA_INCOMING_EVENT;


typedef union
{
    sCHAT_CONNECTION_EVENT_DATA_INCOMING_EVENT incoming_event;
} uCHAT_CONNECTIONS_CBACK_DATA;


typedef void (*fCHAT_CONNECTIONS_USER_CBACK) (
    void*                         user_arg,
    eCHAT_CONNECTIONS_EVENT_TYPE  event,
    uCHAT_CONNECTIONS_CBACK_DATA* data);


typedef void* CHAT_CONNECTIONS;


// Functions -------------------------------------------------------------------

eSTATUS chat_connections_create(
    CHAT_CONNECTIONS*            out_new_chat_connections,
    fCHAT_CONNECTIONS_USER_CBACK user_cback,
    void*                        user_arg,
    uint32_t                     default_size);


eSTATUS chat_connections_close(
    CHAT_CONNECTIONS* chat_connections);


#ifdef __cplusplus
}
#endif
