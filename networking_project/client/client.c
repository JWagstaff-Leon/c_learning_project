#include "client.h"
#include "client_fsm.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "chat_client.h"
#include "chat_event.h"
#include "common_types.h"
#include "message_queue.h"


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
    master_cblk.closing_states.client_open = true;

    status = client_ui_open(master_cblk.ui);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Unable to open UI\n");
        retcode = 1;
        chat_client_close(master_cblk.client);
        master_cblk.state = CLIENT_STATE_CLOSING;
    }
    master_cblk.closing_states.ui_open = true;

    while(CLIENT_STATE_CLOSED != master_cblk.state)
    {
        status = message_queue_get(master_cblk.message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        dispatch_message(&master_cblk, &message);
    }

fail_open_client:
    chat_client_destroy(master_cblk.client);

fail_create_client:
    client_ui_destroy(master_cblk.ui);

fail_create_ui:
    message_queue_destroy(master_cblk.message_queue);

fail_create_message_queue:
    return retcode;
}
