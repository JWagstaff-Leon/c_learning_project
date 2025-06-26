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
    CLIENT_UI_MESSAGE_POST_EVENT,
    CLIENT_UI_MESSAGE_INPUT_CHAR,
    CLIENT_UI_MESSAGE_INPUT_BACKSPACE,
    CLIENT_UI_MESSAGE_INPUT_SEND,
    
    CLIENT_UI_MESSAGE_INPUT_THREAD_CLOSED,
    CLIENT_UI_MESSAGE_CLOSE
} eCLIENT_UI_MESSAGE_TYPE;


typedef struct
{
    sCHAT_EVENT event;
} sCLIENT_UI_PARAMS_POST_EVENT;


typedef struct
{
    char character;
} sCLIENT_UI_PARAMS_INPUT_CHAR;


typedef union
{
    sCLIENT_UI_PARAMS_POST_EVENT post_event;
    sCLIENT_UI_PARAMS_INPUT_CHAR input_char;
} uCLIENT_UI_MESSAGE_PARAMS;


typedef struct
{
    eCLIENT_UI_MESSAGE_TYPE   type;
    uCLIENT_UI_MESSAGE_PARAMS params;
} sCLIENT_UI_MESSAGE;


// Functions -------------------------------------------------------------------



#ifdef __cplusplus
}
#endif
