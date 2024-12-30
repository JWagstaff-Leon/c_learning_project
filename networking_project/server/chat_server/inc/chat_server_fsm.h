#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_MESSAGE_OPEN,
    CHAT_SERVER_MESSAGE_CLOSE,

    CHAT_SERVER_MESSAGE_READ_READY,
    CHAT_SERVER_MESSAGE_READ_READY_ALLOC_FAILED,

    CHAT_SERVER_MESSAGE_WRITE_READY,
    CHAT_SERVER_MESSAGE_WRITE_READY_ALLOC_FAILED
} eCHAT_SERVER_MESSAGE_TYPE;


typedef struct
{
    uint32_t* connection_indecies;
    uint32_t  index_count;
} sCHAT_SERVER_READ_READY_PARAMS;


typedef struct
{
    uint32_t* connection_indecies;
    uint32_t  index_count;
} sCHAT_SERVER_WRITE_READY_PARAMS;


typedef union
{
    sCHAT_SERVER_READ_READY_PARAMS  read_ready;
    sCHAT_SERVER_WRITE_READY_PARAMS write_ready;
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
