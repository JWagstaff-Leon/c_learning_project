#include "client.h"
#include "client_fsm.h"

#include <assert.h>

#include "common_types.h"
#include "message_queue.h"


void chat_client_cback(
    void*                          user_arg,
    bCHAT_CLIENT_EVENT_TYPE        event_mask,
    const sCHAT_CLIENT_CBACK_DATA* data)
{
    eSTATUS              status;
    sCLIENT_MAIN_MESSAGE message;
    sCLIENT_MAIN_CBLK*   master_cblk_ptr = (sCLIENT_MAIN_CBLK*)user_arg;

    if (event_mask & CHAT_CLIENT_EVENT_PRINT_EVENT)
    {
        message.type = CLIENT_MAIN_MESSAGE_PRINT_EVENT;

        status = chat_event_copy(&message.params.print_event.event,
                                 &data->print_event.event);
        assert(STATUS_SUCCESS == status);

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & CHAT_CLIENT_EVENT_CLOSED)
    {
        message.type = CLIENT_MAIN_MESSAGE_CLIENT_CLOSED;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}


void client_ui_cback(
    void*                        user_arg,
    bCLIENT_UI_EVENT_TYPE        event_mask,
    const sCLIENT_UI_CBACK_DATA* data)
{
    eSTATUS              status;
    sCLIENT_MAIN_CBLK*   master_cblk_ptr;
    sCLIENT_MAIN_MESSAGE message;

    assert(NULL != user_arg);
    master_cblk_ptr = (sCLIENT_MAIN_CBLK*)user_arg;

    if (event_mask & CLIENT_UI_EVENT_USER_INPUT)
    {
        message.type = CLIENT_MAIN_MESSAGE_USER_INPUT;

        status = print_string_to_buffer(message.params.user_input.buffer,
                                        data->user_input.buffer,
                                        sizeof(message.params.user_input.buffer),
                                        NULL);
        assert(STATUS_SUCCESS == status);

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (event_mask & CLIENT_UI_EVENT_CLOSED)
    {
        message.type = CLIENT_MAIN_MESSAGE_UI_CLOSED;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}