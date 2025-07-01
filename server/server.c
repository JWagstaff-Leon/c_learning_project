#include <assert.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chat_server.h"
#include "common_types.h"
#include "message_queue.h"


#define MAIN_MESSAGE_QUEUE_SIZE 8


typedef enum
{
    MAIN_MESSAGE_SERVER_OPENED,
    MAIN_MESSAGE_SERVER_CLOSED,

    MAIN_MESSAGE_CLOSE
} eMAIN_MESSAGE_TYPE;


typedef struct
{
    eMAIN_MESSAGE_TYPE type;
} sMAIN_MESSAGE;


typedef enum
{
    MAIN_FSM_STATE_OPEN,
    MAIN_FSM_STATE_CLOSING,
    MAIN_FSM_STATE_CLOSED
} eMAIN_FSM_STATE;


typedef struct
{
    eMAIN_FSM_STATE state;
    MESSAGE_QUEUE   message_queue;

    CHAT_SERVER chat_server;
    THREAD_ID   input_thread;
} sMAIN_CBLK;


static MESSAGE_QUEUE signal_message_queue = NULL;
static const sMAIN_MESSAGE signal_message = { .type = MAIN_MESSAGE_CLOSE };

static void close_signal_handler(
    int signal)
{
    // FIXME with no timeout this is vulnerable to a deadlock
    if (NULL != signal_message_queue)
    {
        message_queue_put(signal_message_queue,
                          &signal_message,
                          sizeof(signal_message));
    }
}


void* server_input_thread_entry(
    void* arg)
{
    eSTATUS       status;
    sMAIN_CBLK*   master_cblk_ptr;
    sMAIN_MESSAGE message;

    bool done = false;

    ssize_t getline_status;
    char*   user_input;
    size_t  user_input_size;

    master_cblk_ptr = (sMAIN_CBLK*)arg;

    while (!done)
    {
        user_input      = NULL;
        user_input_size = 0;

        printf("> ");
        getline_status = getline(&user_input, &user_input_size, stdin);
        assert(getline_status > 0);

        if (!strncmp(user_input, "close\n", sizeof("close\n")))
        {
            done = true;
        }

        // NOTE this does *technically* contain a race-condition in which this
        //      thread closes before freeing due to the main thread closing.
        //      That is fine with me as the odds of that happening are very
        //      slim; on the off-chance it does occur, the OS *should* clean up
        //      the one missed free on its own.
        free(user_input);
    }

    message.type = MAIN_MESSAGE_CLOSE;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    assert(STATUS_SUCCESS == status);

    return NULL;
}


void chat_server_cback(
    void*                          user_arg,
    bCHAT_SERVER_EVENT_TYPE        event_mask,
    const sCHAT_SERVER_CBACK_DATA* data)
{
    eSTATUS       status;
    sMAIN_CBLK*   master_cblk_ptr;
    sMAIN_MESSAGE message;

    assert(NULL != user_arg);
    master_cblk_ptr = (sMAIN_CBLK*)user_arg;

    (void)data;

    if (event_mask & CHAT_SERVER_EVENT_CLOSED)
    {
        message.type = MAIN_MESSAGE_SERVER_CLOSED;
        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
    }
}


int main(int argc, char *argv[])
{
    int     retcode = 0;
    eSTATUS status;

    sMAIN_CBLK    master_cblk;
    sMAIN_MESSAGE message;

    struct sigaction close_signal_action = { .sa_handler = &close_signal_handler, .sa_flags = 0 };
    sigemptyset(&close_signal_action.sa_mask);
    sigaction(SIGHUP,  &close_signal_action, NULL);
    sigaction(SIGINT,  &close_signal_action, NULL);
    sigaction(SIGABRT, &close_signal_action, NULL);
    sigaction(SIGTERM, &close_signal_action, NULL);

    status = message_queue_create(&master_cblk.message_queue,
                                  MAIN_MESSAGE_QUEUE_SIZE,
                                  sizeof(sMAIN_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Failed to create message queue with status %d\n", status);
        retcode = 1;
        goto fail_create_message_queue;
    }
    signal_message_queue = master_cblk.message_queue;

    status = chat_server_create(&master_cblk.chat_server,
                                chat_server_cback,
                                &master_cblk);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Failed to create chat server with status %d\n", status);
        retcode = 1;
        goto fail_create_chat_server;
    }

    status = generic_create_thread(server_input_thread_entry,
                                   &master_cblk,
                                   &master_cblk.input_thread);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Failed to open input thread with status %d\n", status);
        retcode = 1;
        goto fail_open_input_thread;
    }

    status = chat_server_open(master_cblk.chat_server);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Failed to open chat server with status %d\n", status);
        retcode = 1;
        goto fail_open_server;
    }

    while (MAIN_FSM_STATE_CLOSED != master_cblk.state)
    {
        status = message_queue_get(master_cblk.message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        switch (message.type)
        {
            case MAIN_MESSAGE_SERVER_CLOSED:
            {
                master_cblk.state = MAIN_FSM_STATE_CLOSED;
                break;
            }
            case MAIN_MESSAGE_CLOSE:
            {
                chat_server_close(master_cblk.chat_server);
                break;
            }
        }
    }

fail_open_server:
fail_open_input_thread:
    chat_server_destroy(master_cblk.chat_server);

fail_create_chat_server:
    signal_message_queue = NULL;
    message_queue_destroy(master_cblk.message_queue);

fail_create_message_queue:
    return retcode;
}
