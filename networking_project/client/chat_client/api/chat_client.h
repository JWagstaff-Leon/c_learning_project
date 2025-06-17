#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include "common_types.h"


// Constants -------------------------------------------------------------------

#define CLIENT_MESSAGE_QUEUE_SIZE 32

// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENT_EVENT_INCOMING_EVENT = 1 << 0,
    CHAT_CLIENT_EVENT_OUTGOING_EVENT = 1 << 1,

    CHAT_CLIENT_EVENT_CLOSED = 1 << 31
} bCHAT_CLIENT_EVENT_TYPE;


typedef struct
{
    sCHAT_EVENT event;
} sCHAT_CLIENT_CBACK_DATA_INCOMING_EVENT;


typedef struct
{
    sCHAT_CLIENT_CBACK_DATA_INCOMING_EVENT incoming_event;
} sCHAT_CLIENT_CBACK_DATA;


typedef void (*fCHAT_CLIENT_USER_CBACK) (
    void*                    user_arg,
    bCHAT_CLIENT_EVENT_TYPE  event,
    sCHAT_CLIENT_CBACK_DATA* data);


typedef void* CHAT_CLIENT;


// Functions -------------------------------------------------------------------

eSTATUS chat_client_create(
    CHAT_CLIENT*            out_new_client,
    fCHAT_CLIENT_USER_CBACK user_cback,
    void*                   user_arg);


eSTATUS chat_client_open(
    CHAT_CLIENT client,
    const char* address,
    const char* port);


eSTATUS chat_client_send_text(
    CHAT_CLIENT client,
    const char* text);


eSTATUS chat_client_close(
    CHAT_CLIENT client);


#ifdef __cplusplus
}
#endif
