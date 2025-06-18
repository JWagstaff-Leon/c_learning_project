#include "chat_client.h"
#include "chat_client_internal.h"
#include "chat_client_fsm.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "chat_event.h"
#include "common_types.h"


typedef void (*fEVENT_HANDLER) (
    sCHAT_CLIENT_CBLK*,
    const sCHAT_EVENT*);


static void handler_no_op(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    const sCHAT_EVENT* event)
{
    // Intentionally empty
    return;
}


static void handler_chat_message(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    const sCHAT_EVENT* event)
{
    eSTATUS                 status;
    sCHAT_CLIENT_CBACK_DATA cback_data;

    switch (master_cblk_ptr->state)
    {
        case CHAT_CLIENT_STATE_ACTIVE:
        {
            status = chat_event_copy(&cback_data.print_event.event, event);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_PRINT_EVENT,
                                        &cback_data);
            break;
        }
    }
}


static void handler_username_entry(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    const sCHAT_EVENT* event)
{
    eSTATUS                 status;
    sCHAT_CLIENT_CBACK_DATA cback_data;

    switch (master_cblk_ptr->state)
    {
        case CHAT_CLIENT_STATE_INIT:
        case CHAT_CLIENT_STATE_USERNAME_VERIFYING:
        {
            status = chat_event_copy(&cback_data.print_event.event, event);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_USERNAME_ENTRY;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_PRINT_EVENT,
                                        &cback_data);
            break;
        }
    }
}


static void handler_password_entry(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    const sCHAT_EVENT* event)
{
    eSTATUS                 status;
    sCHAT_CLIENT_CBACK_DATA cback_data;

    switch (master_cblk_ptr->state)
    {
        case CHAT_CLIENT_STATE_INIT:
        case CHAT_CLIENT_STATE_USERNAME_VERIFYING:
        case CHAT_CLIENT_STATE_PASSWORD_VERIFYING:
        {
            status = chat_event_copy(&cback_data.print_event.event, event);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_PASSWORD_ENTRY;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_PRINT_EVENT,
                                        &cback_data);
            break;
        }
    }
}


static void handler_authenticated(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    const sCHAT_EVENT* event)
{
    eSTATUS                 status;
    sCHAT_CLIENT_CBACK_DATA cback_data;

    switch (master_cblk_ptr->state)
    {
        case CHAT_CLIENT_STATE_INIT:
        case CHAT_CLIENT_STATE_USERNAME_VERIFYING:
        case CHAT_CLIENT_STATE_PASSWORD_VERIFYING:
        {
            status = chat_event_copy(&cback_data.print_event.event, event);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_CLIENT_STATE_ACTIVE;
            master_cblk_ptr->user_info.id = event->origin.id;
            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_PRINT_EVENT,
                                        &cback_data);
            break;
        }
    }
}


static void handler_active_message(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    const sCHAT_EVENT* event)
{
    eSTATUS                 status;
    sCHAT_CLIENT_CBACK_DATA cback_data;

    switch (master_cblk_ptr->state)
    {
        case CHAT_CLIENT_STATE_ACTIVE:
        {
            status = chat_event_copy(&cback_data.print_event.event, event);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CHAT_CLIENT_EVENT_PRINT_EVENT,
                                        &cback_data);

            break;
        }
    }
}


// This list should match up to the enum values in eCHAT_EVENT_TYPE
static const fEVENT_HANDLER event_handler_table[] = {
    handler_no_op,           // CHAT_EVENT_UNDEFINED
    handler_chat_message,    // CHAT_EVENT_CHAT_MESSAGE
    handler_no_op,           // CHAT_EVENT_CONNECTION_FAILED
    handler_no_op,           // CHAT_EVENT_SERVER_ERROR
    handler_no_op,           // CHAT_EVENT_OVERSIZED_CONTENT
    handler_username_entry,  // CHAT_EVENT_USERNAME_REQUEST
    handler_no_op,           // CHAT_EVENT_USERNAME_SUBMIT
    handler_username_entry,  // CHAT_EVENT_USERNAME_REJECTED
    handler_password_entry,  // CHAT_EVENT_PASSWORD_REQUEST
    handler_no_op,           // CHAT_EVENT_PASSWORD_SUBMIT
    handler_password_entry,  // CHAT_EVENT_PASSWORD_REJECTED
    handler_authenticated,   // CHAT_EVENT_AUTHENTICATED
    handler_active_message,  // CHAT_EVENT_USER_JOIN
    handler_active_message,  // CHAT_EVENT_USER_LEAVE
    handler_active_message,  // CHAT_EVENT_SERVER_SHUTDOWN
    handler_no_op,           // CHAT_EVENT_MAX
};

void chat_client_handle_incoming_event(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    const sCHAT_EVENT* event)
{
    event_handler_table[event->type](master_cblk_ptr, event);
}