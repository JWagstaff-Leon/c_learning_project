#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------



// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_MESSAGE_OPEN,
    CHAT_SERVER_MESSAGE_RESET,
    CHAT_SERVER_MESSAGE_CLOSE,

    CHAT_SERVER_MESSAGE_POLL
} eCHAT_SERVER_MESSAGE_TYPE;


typedef union
{
    void *placeholder;
} uCHAT_SERVER_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_SERVER_MESSAGE_TYPE   type;
    uCHAT_SERVER_MESSAGE_PARAMS params;
} sCHAT_SERVER_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
