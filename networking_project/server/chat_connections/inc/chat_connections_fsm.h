#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------



// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CONNECTIONS_MESSAGE_READ_READY,
    CHAT_CONNECTIONS_MESSAGE_WRITE_READY
} eCHAT_CONNECTIONS_MESSAGE_TYPE;


typedef union
{
    void* placeholder;
} uCHAT_CONNECTIONS_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_CONNECTIONS_MESSAGE_TYPE   type;
    uCHAT_CONNECTIONS_MESSAGE_PARAMS params;
} sCHAT_CONNECTIONS_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
