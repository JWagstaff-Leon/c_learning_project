#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "client_fsm.h"

#include <stdbool.h>

#include "chat_client.h"
#include "client_ui.h"
#include "message_queue.h"


// Constants -------------------------------------------------------------------



// Types -----------------------------------------------------------------------

typedef enum
{
    CLIENT_STATE_OPEN,
    CLIENT_STATE_CLOSING,

    CLIENT_STATE_CLOSED
} eCLIENT_STATE;


typedef struct
{
    bool ui_open;
    bool client_open;
} sCLIENT_MAIN_CBLK_CLOSING_STATES;


typedef struct
{
    eCLIENT_STATE state;
    MESSAGE_QUEUE message_queue;

    CLIENT_UI   ui;
    CHAT_CLIENT client;

    sCLIENT_MAIN_CBLK_CLOSING_STATES closing_states;
} sCLIENT_MAIN_CBLK;


// Functions -------------------------------------------------------------------

void dispatch_message(
    sCLIENT_MAIN_CBLK*          master_cblk_ptr,
    const sCLIENT_MAIN_MESSAGE* message);


void chat_client_cback(
    void*                          user_arg,
    bCHAT_CLIENT_EVENT_TYPE        event,
    const sCHAT_CLIENT_CBACK_DATA* data);


void client_ui_cback(
    void*                        user_arg,
    bCLIENT_UI_EVENT_TYPE        event_mask,
    const sCLIENT_UI_CBACK_DATA* data);


#ifdef __cplusplus
}
#endif
