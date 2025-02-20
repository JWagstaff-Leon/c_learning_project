#include "network_watcher.h"
#include "network_watcher_internal.h"
#include "network_watcher_fsm.h"

#include <assert.h>
#include <poll.h>
#include <pthread.h>
#include <string.h>

#include "common_types.h"
#include "message_queue.h"


static void init_cblk(
    sNETWORK_WATCHER_CBLK* master_cblk_ptr)
{
    memset(master_cblk_ptr, 0, sizeof(sNETWORK_WATCHER_CBLK));

    master_cblk_ptr->poll_timeout = NETWORK_WATCHER_DEFAULT_POLL_TIMEOUT;
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

    status = generic_create_thread(network_watcher_thread_entry,
                                   new_network_watcher);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_thread;
    }

    new_network_watcher->user_cback = user_cback;
    new_network_watcher->user_arg   = user_arg;

    *out_new_network_watcher = new_network_watcher;
    return STATUS_SUCCESS;

fail_create_thread:
    message_queue_destroy(new_network_watcher->message_queue);
    
fail_create_message_queue;
    generic_deallocator(new_network_watcher);

fail_alloc_cblk:
    return status;
}


eSTATUS network_watcher_start_watch(
    NETWORK_WATCHER         network_watcher,
    eNETWORK_WATCHER_MODE   mode,
    sNETWORK_WATCHER_WATCH* watches,
    uint32_t                watch_count)
{
    sNETWORK_WATCHER_CBLK*   master_cblk_ptr;
    sNETWORK_WATCHER_MESSAGE message;
    eSTATUS                  status;

    if(NULL == network_watcher)
    {
        return STATUS_INVALID_ARG;
    }

    if (NULL == watches)
    {
        return STATUS_INVALID_ARG;
    }

    if (watch_count <= 0)
    {
        return STATUS_INVALID_ARG;
    }

    switch (mode)
    {
        NETWORK_WATCHER_MODE_READ:
        NETWORK_WATCHER_MODE_WRITE:
        {
            break;
        }
        default:
        {
            return STATUS_INVALID_ARG;
        }
    }

    master_cblk_ptr = (sNETWORK_WATCHER_CBLK*)network_watcher;

    message.type = NETWORK_WATCHER_MESSAGE_NEW_WATCH;
    message.params.new_watch.watches     = watches;
    message.params.new_watch.watch_count = watch_count;
    message.params.new_watch.mode        = mode;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    assert(STATUS_SUCCESS == status);

    return STATUS_SUCCESS;
}


eSTATUS network_watcher_stop_watch(
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

    message.type = NETWORK_WATCHER_MESSAGE_CANCEL;
    status       = message_queue_put(master_cblk_ptr->message_queue,
                                     &message,
                                     sizeof(message));
    assert(STATUS_SUCCESS == status);

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

    message.type = NETWORK_WATCHER_MESSAGE_CLOSE;
    status       = message_queue_put(master_cblk_ptr->message_queue,
                                     &message,
                                     sizeof(message));
    assert(STATUS_SUCCESS == status);

    return STATUS_SUCCESS;
}
