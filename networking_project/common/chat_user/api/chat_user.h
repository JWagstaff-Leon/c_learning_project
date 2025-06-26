#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stddef.h>

// Constants -------------------------------------------------------------------

#define CHAT_USER_MAX_NAME_SIZE 65


// Types -----------------------------------------------------------------------

typedef struct
{
    char name[CHAT_USER_MAX_NAME_SIZE];
} sCHAT_USER;


typedef struct
{
    char*  username;
    size_t username_size;

    char*  password;
    size_t password_size;
} sCHAT_USER_CREDENTIALS;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
