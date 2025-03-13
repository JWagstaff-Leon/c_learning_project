#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include <stdint.h>


// Constants -------------------------------------------------------------------

#define CHAT_USER_MAX_NAME_SIZE 65

#define CHAT_USER_ID_SERVER  0
#define CHAT_USER_INVALID_ID UINT64_MAX

const char g_server_username[CHAT_USER_MAX_NAME_SIZE] = "Server";


// Types -----------------------------------------------------------------------

typedef struct
{
    uint64_t id;
    char     name[CHAT_USER_MAX_NAME_SIZE];
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
