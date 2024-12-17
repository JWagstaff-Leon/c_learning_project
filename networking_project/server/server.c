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


static pthread_mutex_t k_server_mutex;
static pthread_cond_t  k_server_cond_var;

static eCHAT_SERVER_EVENT_TYPE k_last_event;


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
    void*                    user_arg,
    eCHAT_SERVER_EVENT_TYPE  mask,
    sCHAT_SERVER_CBACK_DATA* data)
{
    (void)data;

    pthread_mutex_lock(&k_server_mutex);

    k_last_event = mask;
    pthread_cond_signal(&k_server_cond_var);

    pthread_mutex_unlock(&k_server_mutex);
}


int main(
    int argc,
    char *argv[])
{
    eSTATUS       status;
    MESSAGE_QUEUE message_queue;
    CHAT_SERVER   server;

    char*   user_input;
    ssize_t user_input_size;
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

    pthread_mutex_init(&k_server_mutex, NULL);

    pthread_mutex_lock(&k_server_mutex);
    chat_server_open(server);
    pthread_cond_wait(&k_server_cond_var, &k_server_mutex);

    switch (k_last_event)
    {
        case CHAT_SERVER_EVENT_OPEN_FAILED:
        {
            fprintf(stderr, "Server open failed\n");
            return 1;
        }
        case CHAT_SERVER_EVENT_OPENED:
        {
            printf("Server has opened\n");
            break;
        }
        case CHAT_SERVER_EVENT_CLOSED:
        {
            fprintf(stderr, "Server has closed instead of opening\n");
            return 1;
        }
        default:
        {
            fprintf(stderr, "Server has sent an unexpected message when opening\n");
            return 1;
        }
    }

    while (CHAT_SERVER_EVENT_CLOSED != k_last_event)
    {
        getline(&user_input, &user_input_size, stdin);
        if (!strncmp(user_input, "close\n", sizeof("close\n")))
        {
            chat_server_close(server);
            while (CHAT_SERVER_EVENT_CLOSED != k_last_event)
            {
                pthread_cond_wait(&k_server_cond_var, &k_server_mutex);
            }
            continue;
        }
    }
    pthread_mutex_unlock(&k_server_mutex);

    return 0;
}
