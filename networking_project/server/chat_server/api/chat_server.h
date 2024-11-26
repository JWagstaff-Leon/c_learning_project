#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>

#include "common_types.h"


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_EVENT_OPENED      = 1 << 0,
    CHAT_SERVER_EVENT_OPEN_FAILED = 1 << 1,
    CHAT_SERVER_EVENT_RESET       = 1 << 2,
    CHAT_SERVER_EVENT_CLOSED      = 1 << 3
} eCHAT_SERVER_EVENT_TYPE;


typedef void* (*fCHAT_SERVER_THREAD_ENTRY) (void*);


typedef eSTATUS (*fCHAT_SERVER_THREAD_CREATE_CBACK) (fCHAT_SERVER_THREAD_ENTRY, void*);


typedef void sCHAT_SERVER_CBLK_DATA;


typedef void (*fCHAT_SERVER_USER_CBACK) (
    void* user_arg,
    eCHAT_SERVER_EVENT_TYPE mask,
    sCHAT_SERVER_CBLK_DATA* data);


typedef enum
{
    PLACEHOLDER
} eCONNECTION_EVENT;


// Functions -------------------------------------------------------------------

eSTATUS chat_server_init(
    fCHAT_SERVER_THREAD_CREATE_CBACK create_thread,
    fCHAT_SERVER_USER_CBACK          user_cback,
    void*                            user_arg);


eSTATUS chat_server_open(
    void);


eSTATUS chat_server_reset(
    void);


eSTATUS chat_server_close(
    void);


#ifdef __cplusplus
}
#endif
