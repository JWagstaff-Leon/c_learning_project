#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chat_server.h"
#include "common_types.h"
#include "message_queue.h"


typedef enum
{
    MAIN_MESSAGE_SERVER_OPENED,
    MAIN_MESSAGE_SERVER_CLOSED
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
} sMAIN_CBLK;


#define MAIN_MESSAGE_QUEUE_SIZE 8


eSTATUS create_chat_server_thread(
    fGENERIC_THREAD_ENTRY thread_entry,
    void*                 thread_entry_arg)
{
    pthread_t unused;
    pthread_create(&unused,
                   NULL,
                   thread_entry,
                   thread_entry_arg);
    return STATUS_SUCCESS;
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
        master_cblk_ptr->state = MAIN_FSM_STATE_CLOSED;
        // message.type = MAIN_MESSAGE_SERVER_CLOSED;
        // status = message_queue_put(master_cblk_ptr->message_queue,
        //                            &message,
        //                            sizeof(message));
        // assert(STATUS_SUCCESS == status);
    }
}


int main(int argc, char *argv[])
{
    int     retcode = 0;
    eSTATUS status;

    sMAIN_CBLK master_cblk;

    char*   user_input;
    ssize_t user_input_size;

    status = chat_server_create(&master_cblk.chat_server,
                                chat_server_cback,
                                &master_cblk);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Failed to create server with status %d\n", status);
        return 1;
    }

    status = chat_server_open(master_cblk.chat_server);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Failed to open server with status %d\n", status);
        retcode = 1;
        goto fail_open_server;
    }

    while (MAIN_FSM_STATE_CLOSED != master_cblk.state)
    {
        getline(&user_input, &user_input_size, stdin);
        if (!strncmp(user_input, "close\n", sizeof("close\n")))
        {
            chat_server_close(master_cblk.chat_server);
        }
    }

fail_open_server:
    chat_server_destroy(master_cblk.chat_server);

    return retcode;
}
