#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "common_types.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CLIENT_UI_EVENT_USER_INPUT = 1 << 0,

    CLIENT_UI_EVENT_CLOSED = 1 << 31
} bCLIENT_UI_EVENT_TYPE;


typedef struct
{

} sCLIENT_UI_CBACK_DATA;


typedef void (*fCLIENT_UI_CBACK)(
    void*                 user_arg,
    bCLIENT_UI_EVENT_TYPE event_mask,
    sCLIENT_UI_CBACK_DATA data);


typedef void* CLIENT_UI;


// Functions -------------------------------------------------------------------

eSTATUS client_ui_create(
    CLIENT_UI*       out_new_client_ui,
    fCLIENT_UI_CBACK user_cback,
    void*            user_arg);


#ifdef __cplusplus
}
#endif
