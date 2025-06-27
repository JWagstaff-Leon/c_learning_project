#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_event.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CLIENT_MAIN_MESSAGE_PRINT_EVENT,
    CLIENT_MAIN_MESSAGE_USER_INPUT,

    CLIENT_MAIN_MESSAGE_CLIENT_CLOSED,
    CLIENT_MAIN_MESSAGE_UI_CLOSED
} eCLIENT_MAIN_MESSAGE_TYPE;


typedef struct
{
    sCHAT_EVENT event;
} sCLIENT_MAIN_MESSAGE_PARAMS_PRINT_EVENT;


typedef struct
{
    char buffer[CHAT_EVENT_MAX_DATA_SIZE];
} sCLIENT_MAIN_MESSAGE_PARAMS_USER_INPUT;


typedef union
{
    sCLIENT_MAIN_MESSAGE_PARAMS_PRINT_EVENT print_event;
    sCLIENT_MAIN_MESSAGE_PARAMS_USER_INPUT  user_input;
} uCLIENT_MAIN_MESSAGE_PARAMS;


typedef struct
{
    eCLIENT_MAIN_MESSAGE_TYPE   type;
    uCLIENT_MAIN_MESSAGE_PARAMS params;
} sCLIENT_MAIN_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
