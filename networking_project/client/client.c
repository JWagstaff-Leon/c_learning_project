#include "client.h"

#include <stdbool.h>
#include <stdio.h>

#include "chat_client.h"
#include "chat_event.h"
#include "common_types.h"
#include "message_queue.h"


void chat_client_cback(
    void*                    user_arg,
    bCHAT_CLIENT_EVENT_TYPE  event,
    sCHAT_CLIENT_CBACK_DATA* data)
{
    eSTATUS              status;
    sCLIENT_MAIN_MESSAGE message;
    sCLIENT_MAIN_CBLK*   master_cblk_ptr = (sCLIENT_MAIN_CBLK*)user_arg;

    if (CHAT_CLIENT_EVENT_INCOMING_EVENT)
    {
        message.type = CLIENT_MAIN_MESSAGE_INCOMING_EVENT;

        status = chat_event_copy(&message.params.incoming_event.event,
                                 &data->incoming_event.event);
        assert(STATUS_SUCCESS == status);

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (CHAT_CLIENT_EVENT_OUTGOING_EVENT)
    {
        message.type = CLIENT_MAIN_MESSAGE_OUTGOING_EVENT;

        status = chat_event_copy(&message.params.outgoing_event.event,
                                 &data->outgoing_event.event);
        assert(STATUS_SUCCESS == status);

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }

    if (CHAT_CLIENT_EVENT_CLOSED)
    {
        message.type = CLIENT_MAIN_MESSAGE_CLOSE;

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
    switch (message->type)
    {
        case CLIENT_MAIN_MESSAGE_INCOMING_EVENT:
        {
            break;
        }
        case CLIENT_MAIN_MESSAGE_OUTGOING_EVENT:
        {
            break;
        }
        case CLIENT_MAIN_MESSAGE_CLOSE:
        {
            break;
        }
    }
}


int main(int argc, char *argv[])
{
    eSTATUS status;
    int stdlib_status;

    char* input_buffer;
    size_t input_size;

    sCLIENT_MAIN_MESSAGE message;

    sCLIENT_MAIN_CBLK master_cblk;
    memset(&master_cblk, 0, sizeof(master_cblk));

    master_cblk.open = true;

    status = message_queue_create(&master_cblk.messages_window,
                                  CLIENT_MESSAGE_QUEUE_SIZE,  
                                  sizeof(sCLIENT_MAIN_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Unable to open client\n");
        return 1;
    }

    while(master_cblk.open)
    {
        status = message_queue_get(master_cblk.message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        dispatch_message(&master_cblk, message);
    }

    return 0;
}
