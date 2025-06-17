#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_event.h"
#include "chat_user.h"
#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CONNECTION_EVENT_INCOMING_EVENT = 1 << 0,

    CHAT_CONNECTION_EVENT_CLOSED = 1 << 31
} bCHAT_CONNECTION_EVENT_TYPE;


typedef struct
{
    sCHAT_EVENT event;
} sCHAT_CONNECTION_CBACK_DATA_INCOMING_EVENT;


typedef union
{
    sCHAT_CONNECTION_CBACK_DATA_INCOMING_EVENT incoming_event;
} sCHAT_CONNECTION_CBACK_DATA;


typedef void (*fCHAT_CONNECTION_USER_CBACK)(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const sCHAT_CONNECTION_CBACK_DATA* data);


typedef void* CHAT_CONNECTION;


// Functions -------------------------------------------------------------------

eSTATUS chat_connection_create(
    CHAT_CONNECTION*            out_chat_connection,
    fCHAT_CONNECTION_USER_CBACK user_cback,
    void*                       user_arg,
    int                         fd);


eSTATUS chat_connection_queue_new_event(
    CHAT_CONNECTION  restrict chat_connection,
    eCHAT_EVENT_TYPE          event_type,
    sCHAT_USER                event_origin,
    const char*      restrict event_data);


eSTATUS chat_connection_queue_event(
    CHAT_CONNECTION    restrict chat_connection,
    const sCHAT_EVENT* restrict event);


eSTATUS chat_connection_close(
    CHAT_CONNECTION chat_connection);


eSTATUS chat_connection_destroy(
    CHAT_CONNECTION chat_connection);


#ifdef __cplusplus
}
#endif
