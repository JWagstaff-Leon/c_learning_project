#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------



// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENT_MESSAGE_CONNECT,
    CHAT_CLIENT_MESSAGE_DISCONNECT,
    CHAT_CLIENT_MESSAGE_CLOSE
} eCHAT_CLIENT_MESSAGE_TYPE;


typedef struct
{
    unsigned char* address;
} sCHAT_CLIENT_CONNECT_PARAMS;


typedef struct
{
    unsigned char* username;
} sCHAT_CLIENT_INACTIVE_PARAMS;


typedef union
{
    sCHAT_CLIENT_CONNECT_PARAMS  connect;
    sCHAT_CLIENT_INACTIVE_PARAMS inactive
} uCHAT_CLIENT_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_CLIENT_MESSAGE_TYPE   type;
    uCHAT_CLIENT_MESSAGE_PARAMS params;
} sCHAT_CLIENT_MESSAGE;


typedef struct
{

} sCHAT_CLIENT_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
