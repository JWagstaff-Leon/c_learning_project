#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

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


static pthread_mutex_t server_mutex;


#define MAIN_MESSAGE_QUEUE_SIZE 8


eSTATUS create_chat_server_thread(
    fCHAT_SERVER_THREAD_ENTRY thread_entry,
    void*                     thread_entry_arg)
{
    pthread_t unused;
    pthread_create(&unused,
                   NULL,
                   thread_entry,
                   thread_entry_arg);
    return STATUS_SUCCESS;
}


void chat_server_cback(
    void*                    user_arg,
    eCHAT_SERVER_EVENT_TYPE  mask,
    sCHAT_SERVER_CBACK_DATA* data)
{
    (void)data;

    if (mask & CHAT_SERVER_EVENT_OPENED)
    {
        printf("Server has opened\n");
    }

    if (mask & CHAT_SERVER_EVENT_OPEN_FAILED)
    {
        printf("Server open failed\n");
    }

    if (mask & CHAT_SERVER_EVENT_RESET)
    {
        printf("Server has reset\n");
    }

    if (mask & CHAT_SERVER_EVENT_CLOSED)
    {
        printf("Server has closed\n");
    }
}


int main(
    int argc,
    char *argv[])
{
    eSTATUS          status;
    MESSAGE_QUEUE_ID message_queue;
    CHAT_SERVER      server;

    status = message_queue_create(&message_queue,
                                  MAIN_MESSAGE_QUEUE_SIZE,
                                  sizeof(sMAIN_MESSAGE),
                                  malloc,
                                  free);

    status = chat_server_create(&server,
                                malloc,
                                free,
                                create_chat_server_thread,
                                chat_server_cback,
                                NULL);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "Failed to create server with status %d\n", status);
        return 1;
    }

    chat_server_open(server);

    return 0;
}
