#include "client.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "chat_client.h"
#include "chat_event.h"
#include "common_types.h"
#include "message_queue.h"


void chat_client_cback(
    void*                          user_arg,
    bCHAT_CLIENT_EVENT_TYPE        event,
    const sCHAT_CLIENT_CBACK_DATA* data)
{
    eSTATUS              status;
    sCLIENT_MAIN_MESSAGE message;
    sCLIENT_MAIN_CBLK*   master_cblk_ptr = (sCLIENT_MAIN_CBLK*)user_arg;

    if (CHAT_CLIENT_EVENT_PRINT_EVENT)
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

    if (CHAT_CLIENT_EVENT_CLOSED)
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


void dispatch_message(
    sCLIENT_MAIN_CBLK*          master_cblk_ptr,
    const sCLIENT_MAIN_MESSAGE* message)
{
    eSTATUS status;

    sCHAT_EVENT outgoing_event;

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
            break;
        }
        case CLIENT_MAIN_MESSAGE_UI_CLOSED:
        {
            master_cblk_ptr->open = false;
            break;
        }
        default:
        {
            // Should never get here
            assert(0);
            break;
        }
    }
}


int main(int argc, char *argv[])
{
    int retcode = 0;

    eSTATUS status;
    int stdlib_status;

    char* input_buffer;
    size_t input_size;

    sCLIENT_MAIN_MESSAGE message;

    sCLIENT_MAIN_CBLK master_cblk;
    memset(&master_cblk, 0, sizeof(master_cblk));

    master_cblk.open = true;

    status = message_queue_create(&master_cblk.message_queue,
                                  CLIENT_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCLIENT_MAIN_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Unable to create message queue\n");
        retcode = 1;
        goto fail_create_message_queue;
    }

    status = client_ui_create(&master_cblk.ui,
                              client_ui_cback,
                              &master_cblk);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Unable to init UI\n");
        retcode = 1;
        goto fail_create_ui;
    }

    status = chat_client_create(&master_cblk.client,
                                chat_client_cback,
                                &master_cblk);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Unable to init client\n");
        retcode = 1;
        goto fail_create_client;
    }
    
    status = chat_client_open(master_cblk.client,
                              "127.0.0.1", "8080");
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Unable to open client\n");
        retcode = 1;
        goto fail_open_client;
    }

    while(master_cblk.open)
    {
        status = message_queue_get(master_cblk.message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        dispatch_message(&master_cblk, &message);
    }

    retcode = 0;

fail_open_client:
    chat_client_close(master_cblk.client);

fail_create_client:
    // TODO make client kill

fail_create_ui:
    message_queue_destroy(master_cblk.message_queue);

fail_create_message_queue:
    return retcode;
}
