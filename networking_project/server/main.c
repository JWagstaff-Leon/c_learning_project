#include <pthread.h>

#include "chat_server.h"
#include "common_types.h"

eSTATUS create_chat_server_thread(
    fCHAT_SERVER_THREAD_ENTRY thread_entry,
    void *thread_entry_arg
)
{
    pthread_t unused;
    pthread_create(&unused,
                   NULL,
                   thread_entry,
                   thread_entry_arg);
    return STATUS_SUCCESS;
}

int main(
    int argc,
    char *argv[])
{
    chat_server_init(create_chat_server_thread);
    return 0;
}
