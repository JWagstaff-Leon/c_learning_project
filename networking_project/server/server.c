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
    MAIN_MESSAGE_SERVER_CLOSED,
    MAIN_MESSAGE_CLOSE_SERVER
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
} sMAIN_FSM_STATE;


static bCHAT_SERVER_EVENT_TYPE k_last_event;


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
    (void)data;

    k_last_event = event_mask;
}


int main(int argc, char *argv[])
{
    eSTATUS       status;
    MESSAGE_QUEUE message_queue;
    CHAT_SERVER   server;

    char*   user_input;
    ssize_t user_input_size;
    status = chat_server_create(&server,
                                chat_server_cback,
                                NULL);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Failed to create server with status %d\n", status);
        return 1;
    }

    while (CHAT_SERVER_EVENT_CLOSED != k_last_event)
    {
        getline(&user_input, &user_input_size, stdin);
        if (!strncmp(user_input, "close\n", sizeof("close\n")))
        {
            chat_server_close(server);
        }
    }

    return 0;
}
