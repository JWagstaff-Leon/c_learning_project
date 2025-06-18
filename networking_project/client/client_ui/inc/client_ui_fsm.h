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
    CLIENT_UI_MESSAGE_TYPE_POST_EVENT,
    
    CLIENT_UI_MESSAGE_TYPE_INPUT_THREAD_CLOSED,
    CLIENT_UI_MESSAGE_TYPE_CLOSE
} eCLIENT_UI_MESSAGE_TYPE;


typedef struct
{
    sCHAT_EVENT event;
} sCLIENT_UI_PARAMS_POST_EVENT;


typedef union
{
    sCLIENT_UI_PARAMS_POST_EVENT post_event;
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
