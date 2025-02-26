#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>


// Constants -------------------------------------------------------------------

#define CHAT_USER_MAX_NAME_SIZE 65
#define CHAT_USER_ID_SERVER 0

#define CHAT_USER_INVALID_ID UINT64_MAX


// Types -----------------------------------------------------------------------

typedef struct
{
    uint64_t id_value;
} sCHAT_USER_ID;


typedef struct
{
    sCHAT_USER_ID id;
    char          name[CHAT_USER_MAX_NAME_SIZE];
} sCHAT_USER;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
