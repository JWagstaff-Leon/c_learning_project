#include "client.h"
#include "client_fsm.h"

#include <assert.h>


static void open_processing(
    sCLIENT_MAIN_CBLK*          master_cblk_ptr,
    const sCLIENT_MAIN_MESSAGE* message)
{
    eSTATUS status;

    switch (message->type)
    {
        case CLIENT_MAIN_MESSAGE_PRINT_EVENT:
        {
            status = client_ui_post_event(master_cblk_ptr->ui,
                                          &message->params.print_event.event);
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CLIENT_MAIN_MESSAGE_USER_INPUT:
        {
            status = chat_client_user_input(master_cblk_ptr->client,
                                            message->params.user_input.buffer);
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CLIENT_MAIN_MESSAGE_CLIENT_CLOSED:
        {
            master_cblk_ptr->closing_states.client_open = false;

            status = client_ui_close(master_cblk_ptr->ui);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CLIENT_STATE_CLOSING;
            break;
        }
        case CLIENT_MAIN_MESSAGE_UI_CLOSED:
        {
            master_cblk_ptr->closing_states.ui_open = false;
            
            status = chat_client_close(master_cblk_ptr->client);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CLIENT_STATE_CLOSING;
            break;
        }
    }
}


#include <stdio.h>
static bool check_closed(
    sCLIENT_MAIN_CBLK* master_cblk_ptr)
{
    bool open = false;

fprintf(stderr, "Checking closed? %s\n", open ? "false" : "true");
    open |= master_cblk_ptr->closing_states.client_open;
fprintf(stderr, "Checking closed, after client_open? %s\n", open ? "false" : "true");
    open |= master_cblk_ptr->closing_states.ui_open;
fprintf(stderr, "Checking closed, after ui_open? %s\n", open ? "false" : "true");

fprintf(stderr, "Checking closed, returning? %s\n", !open ? "true" : "false");
    return !open;
}


static void closing_processing(
    sCLIENT_MAIN_CBLK*          master_cblk_ptr,
    const sCLIENT_MAIN_MESSAGE* message)
{
    eSTATUS status;

    switch (message->type)
    {
        case CLIENT_MAIN_MESSAGE_CLIENT_CLOSED:
        {
            master_cblk_ptr->closing_states.client_open = false;
            if (check_closed(master_cblk_ptr))
            {
                master_cblk_ptr->state = CLIENT_STATE_CLOSED;
            }

            break;
        }
        case CLIENT_MAIN_MESSAGE_UI_CLOSED:
        {
            master_cblk_ptr->closing_states.ui_open = false;
            if (check_closed(master_cblk_ptr))
            {
                master_cblk_ptr->state = CLIENT_STATE_CLOSED;
            }

            break;
        }
    }
}


void dispatch_message(
    sCLIENT_MAIN_CBLK*          master_cblk_ptr,
    const sCLIENT_MAIN_MESSAGE* message)
{
    switch (master_cblk_ptr->state)
    {
        case CLIENT_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
            break;
        }
        case CLIENT_STATE_CLOSING:
        {
            closing_processing(master_cblk_ptr, message);
            break;
        }
        case CLIENT_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
            break;
        }
    }
}