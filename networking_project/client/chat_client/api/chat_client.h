#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------



// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENT_EVENT_CONNECTED         = 1 << 0,
    CHAT_CLIENT_EVENT_CONNECT_FAILED    = 1 << 1,
    CHAT_CLIENT_EVENT_USERNAME_ACCEPTED = 1 << 2,
    CHAT_CLIENT_EVENT_USERNAME_REJECTED = 1 << 3,
    CHAT_CLIENT_EVENT_DISCONNECTED      = 1 << 4,
    CHAT_CLIENT_EVENT_CLOSED            = 1 << 5
} eCHAT_CLIENT_EVENT_TYPE;


typedef void* (*fCHAT_CLIENT_THREAD_ENTRY) (void*);


typedef eSTATUS (*fCHAT_CLIENT_THREAD_CREATE_CBACK) (fCHAT_CLIENT_THREAD_ENTRY, void*);


typedef void sCHAT_CLIENT_CBACK_DATA;


typedef void (*fCHAT_CLIENT_USER_CBACK) (
    void*                    user_arg,
    eCHAT_CLIENT_EVENT_TYPE  mask,
    sCHAT_CLIENT_CBACK_DATA* data);


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
