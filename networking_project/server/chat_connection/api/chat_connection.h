#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_user.h"
#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CONNECTION_EVENT_INCOMING_EVENT    = 1 << 0,
    CHAT_CONNECTION_EVENT_CONNECTION_CLOSED = 1 << 1,

    CHAT_CONNECTION_EVENT_CLOSED = 1 << 31
} bCHAT_CONNECTION_EVENT_TYPE;


typedef struct
{
    sCHAT_EVENT event;
} sCHAT_CONNECTION_CBACK_DATA_INCOMING_EVENT;


typedef union
{
    sCHAT_CONNECTION_CBACK_DATA_INCOMING_EVENT incoming_event;
} uCHAT_CONNECTION_CBACK_DATA;


typedef void (fCHAT_CONNECTION_USER_CBACK)(
    void*                              user_arg,
    bCHAT_CONNECTION_EVENT_TYPE        event_mask,
    const uCHAT_CONNECTION_CBACK_DATA* data);


typedef void* CHAT_CONNECTION;


// Functions -------------------------------------------------------------------

eSTATUS chat_connection_create(
    CHAT_CONNECTION*            out_chat_connection,
    fCHAT_CONNECTION_USER_CBACK user_cback,
    void*                       user_arg,
    int                         fd);


// eSTATUS chat_connection_request_username(
//     CHAT_CONNECTION chat_connection,
//     sCHAT_EVENT*    request_event);


// eSTATUS chat_connection_reject_username(
//     CHAT_CONNECTION chat_connection,
//     sCHAT_EVENT*    request_event);
    
    
// eSTATUS chat_connection_request_password(
//     CHAT_CONNECTION chat_connection,
//     sCHAT_EVENT*    request_event);


// eSTATUS chat_connection_reject_password(
//     CHAT_CONNECTION chat_connection,
//     sCHAT_EVENT*    request_event);

            
// eSTATUS chat_connection_authenticated(
//     CHAT_CONNECTION chat_connection,
//     sCHAT_USER*     user_info);


eSTATUS chat_connection_queue_event(
    CHAT_CONNECTION chat_connection,
    sCHAT_EVENT*    event);


eSTATUS chat_connection_close(
    CHAT_CONNECTION chat_connection);

#ifdef __cplusplus
}
#endif
