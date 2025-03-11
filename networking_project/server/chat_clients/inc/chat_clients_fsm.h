#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------



// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENTS_MESSAGE_READ_READY,
    CHAT_CLIENTS_MESSAGE_WRITE_READY
} eCHAT_CLIENTS_MESSAGE_TYPE;


typedef union
{
    void* placeholder;
} uCHAT_CLIENTS_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_CLIENTS_MESSAGE_TYPE   type;
    uCHAT_CLIENTS_MESSAGE_PARAMS params;
} sCHAT_CLIENTS_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
