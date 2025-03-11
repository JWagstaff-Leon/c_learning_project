#include "network_watcher.h"
#include "network_watcher_internal.h"
#include "network_watcher_fsm.h"

#include <assert.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"
#include "message_queue.h"


static void init_cblk(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    memset(master_cblk_ptr, 0, sizeof(sNETWORK_WATCHER_CBLK));

    master_cblk_ptr->state = NETWORK_WATCHER_STATE_OPEN;

    pthread_mutex_init(&master_cblk_ptr->cancel_mutex);
}


eSTATUS network_watcher_create(
    NETWORK_WATCHER*               out_new_network_watcher,
    fNETWORK_WATCHER_USER_CBACK    user_cback,
    void*                          user_arg)
{
    sNETWORK_WATCHER_CBLK* new_network_watcher;
    eSTATUS                status;
    int                    pipe_status;

    assert(NULL != out_new_network_watcher);
    assert(NULL != user_cback);

    new_network_watcher = (sNETWORK_WATCHER_CBLK*)generic_allocator(sizeof(sNETWORK_WATCHER_CBLK));
    if (NULL == new_network_watcher)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }
    init_cblk(new_network_watcher);

    status = message_queue_create(&new_network_watcher->message_queue,
                                  NETWORK_WATCHER_MESSAGE_QUEUE_SIZE,
                                  sizeof(sNETWORK_WATCHER_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_message_queue;
    }

    pipe_status = pipe2(new_network_watcher->cancel_pipe, O_NONBLOCK);
    if (0 != pipe_status)
    {
        status = STATUS_FAILURE;
        goto fail_create_pipe;
    }

    status = generic_create_thread(network_watcher_thread_entry,
                                   new_network_watcher);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_thread;
    }

    new_network_watcher->user_cback = user_cback;
    new_network_watcher->user_arg   = user_arg;

    new_network_watcher->fds[1].fd     = new_network_watcher->cancel_pipe[PIPE_END_READ];
    new_network_watcher->fds[1].events = POLLIN;

    *out_new_network_watcher = new_network_watcher;
    return STATUS_SUCCESS;

fail_create_thread:
    close(new_network_watcher->cancel_pipe[PIPE_END_WRITE]);
    close(new_network_watcher->cancel_pipe[PIPE_END_READ]);

fail_create_pipe:
    message_queue_destroy(new_network_watcher->message_queue);

fail_create_message_queue;
    generic_deallocator(new_network_watcher);

fail_alloc_cblk:
    return status;
}


eSTATUS network_watcher_start_watch(
    NETWORK_WATCHER       network_watcher,
    bNETWORK_WATCHER_MODE mode,
    int                   fd)
{
    sNETWORK_WATCHER_CBLK*   master_cblk_ptr;
    sNETWORK_WATCHER_MESSAGE message;
    eSTATUS                  status;
    uint8_t                  flush_buffer[128];

    if(NULL == network_watcher)
    {
        return STATUS_INVALID_ARG;
    }

    if (!(mode & (NETWORK_WATCHER_MODE_READ | NETWORK_WATCHER_MODE_WRITE)))
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    message.type = NETWORK_WATCHER_MESSAGE_WATCH;

    message.params.new_watch.mode = mode;
    message.params.new_watch.fd   = fd;

    pthread_mutex_lock(&master_cblk_ptr->cancel_mutex);

    while (read(master_cblk_ptr->control_pipe[PIPE_END_READ],
           flush_buffer,
           sizeof(flush_buffer)) > 0);

    pthread_mutex_unlock(&master_cblk_ptr->cancel_mutex);

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    assert(STATUS_SUCCESS == status);

    return STATUS_SUCCESS;
}


eSTATUS network_watcher_cancel_watch(
    NETWORK_WATCHER network_watcher)
{
    sNETWORK_WATCHER_CBLK* master_cblk_ptr;
    eSTATUS                status;

    if(NULL == network_watcher)
    {
        return STATUS_INVALID_ARG;
    }
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    pthread_mutex_lock(&master_cblk_ptr->cancel_mutex);
    write(master_cblk_ptr->cancel_pipe[PIPE_END_WRITE], " ", 2); // TODO add error checking
    pthread_mutex_unlock(&master_cblk_ptr->cancel_mutex);

    return STATUS_SUCCESS;
}


eSTATUS network_watcher_close(
    NETWORK_WATCHER network_watcher)
{
    sNETWORK_WATCHER_CBLK*   master_cblk_ptr;
    sNETWORK_WATCHER_MESSAGE message;
    eSTATUS                  status;

    if(NULL == network_watcher)
    {
        return STATUS_INVALID_ARG;
    }
    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    status = network_watcher_cancel_watch(network_watcher);
    assert(STATUS_SUCCESS == status);

    message.type = NETWORK_WATCHER_MESSAGE_CLOSE;
    status       = message_queue_put(master_cblk_ptr->message_queue,
                                     &message,
                                     sizeof(message));
    assert(STATUS_SUCCESS == status);

    return STATUS_SUCCESS;
}
