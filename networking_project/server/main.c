#include <pthread.h>

#include "chat_server.h"
#include "common_types.h"

static pthread_mutex_t server_mutex;

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

    }

    if (mask & CHAT_SERVER_EVENT_OPEN_FAILED)
    {

    }

    if (mask & CHAT_SERVER_EVENT_RESET)
    {

    }

    if (mask & CHAT_SERVER_EVENT_CLOSED)
    {

    }
}


int main(
    int argc,
    char *argv[])
{
    chat_server_init(create_chat_server_thread,
                     chat_server_cback,
                     NULL);
    chat_server_open();


    return 0;
}
