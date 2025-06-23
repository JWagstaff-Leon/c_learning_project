#include "chat_connection.h"
#include "chat_connection_internal.h"
#include "chat_connection_fsm.h"

#include <assert.h>

#include "chat_event.h"
#include "chat_event_io.h"
#include "chat_user.h"
#include "common_types.h"
#include "message_queue.h"
#include "network_watcher.h"


eSTATUS chat_connection_create(
    CHAT_CONNECTION*            out_chat_connection,
    fCHAT_CONNECTION_USER_CBACK user_cback,
    void*                       user_arg)
{
    sCHAT_CONNECTION_CBLK* new_chat_connection;
    eSTATUS                status;

    new_chat_connection = generic_allocator(sizeof(sCHAT_CONNECTION_CBLK));
    if (NULL == new_chat_connection)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_chat_connection;
    }

    new_chat_connection->user_cback = user_cback;
    new_chat_connection->user_arg   = user_arg;
    new_chat_connection->state      = CHAT_CONNECTION_STATE_INIT;

    status = chat_event_io_create(&new_chat_connection->io);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_chat_event_io;
    }

    status = message_queue_create(&new_chat_connection->message_queue,
                                  CHAT_CONNECTION_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_CONNECTION_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_message_queue;
    }

    status = message_queue_create(&new_chat_connection->event_queue,
                                  CHAT_CONNECTION_EVENT_QUEUE_SIZE,
                                  sizeof(sCHAT_EVENT));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_event_queue;
    }

    status = network_watcher_create(&new_chat_connection->network_watcher,
                                    chat_connection_watcher_cback,
                                    new_chat_connection);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_network_watcher;
    }

    *out_chat_connection = (CHAT_CONNECTION)new_chat_connection;
    return STATUS_SUCCESS;

fail_create_network_watcher:
    message_queue_destroy(new_chat_connection->event_queue);

fail_create_event_queue:
    message_queue_destroy(new_chat_connection->message_queue);

fail_create_message_queue:
    chat_event_io_destroy(new_chat_connection->io);

fail_create_chat_event_io:
    generic_deallocator(new_chat_connection);

fail_alloc_chat_connection:
    return status;
}


eSTATUS chat_connection_open(
    CHAT_CONNECTION chat_connection,
    int             fd)
{
    eSTATUS                status;
    sCHAT_CONNECTION_CBLK* master_cblk_ptr;

    if (NULL == chat_connection)
    {
        return STATUS_INVALID_ARG;
    }

    if (CHAT_CONNECTION_STATE_INIT != master_cblk_ptr->state)
    {
        return STATUS_INVALID_STATE;
    }

    master_cblk_ptr->fd = fd;
    master_cblk_ptr->state = CHAT_CONNECTION_STATE_READING;

    status = network_watcher_open(master_cblk_ptr->network_watcher);
    if (STATUS_SUCCESS != status)
    {
        master_cblk_ptr->state = CHAT_CONNECTION_STATE_CLOSED;
        return status;
    }
    
    status = generic_create_thread(chat_connection_thread_entry,
                                   master_cblk_ptr,
                                   NULL);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    return STATUS_SUCCESS;
}


eSTATUS chat_connection_queue_new_event(
    CHAT_CONNECTION  restrict chat_connection,
    eCHAT_EVENT_TYPE          event_type,
    sCHAT_USER                event_origin,
    const char*      restrict event_data)
{
    eSTATUS     status;
    sCHAT_EVENT event;

    status = chat_event_populate(&event,
                                 event_type,
                                 event_origin,
                                 event_data);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    status = chat_connection_queue_event(chat_connection,
                                         &event);

    return status;
}


eSTATUS chat_connection_queue_event(
    CHAT_CONNECTION    restrict chat_connection,
    const sCHAT_EVENT* restrict event)
{
    eSTATUS status;

    sCHAT_CONNECTION_CBLK*   master_cblk_ptr;
    sCHAT_CONNECTION_MESSAGE message;

    if (NULL == chat_connection)
    {
        return STATUS_INVALID_ARG;
    }
    if (NULL == event)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_CONNECTION_CBLK*)chat_connection;

    message.type = CHAT_CONNECTION_MESSAGE_QUEUE_EVENT;

    status = chat_event_copy(&message.params.queue_event.event,
                             event);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    return status;
}


eSTATUS chat_connection_close(
    CHAT_CONNECTION chat_connection)
{
    eSTATUS status;

    sCHAT_CONNECTION_CBLK*   master_cblk_ptr;
    sCHAT_CONNECTION_MESSAGE message;

    if (NULL == chat_connection)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_CONNECTION_CBLK*)chat_connection;

    message.type = CHAT_CONNECTION_MESSAGE_CLOSE;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    return status;
}


eSTATUS chat_connection_destroy(
    CHAT_CONNECTION chat_connection)
{
    eSTATUS                status;
    sCHAT_CONNECTION_CBLK* master_cblk_ptr;

    if (NULL == chat_connection)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCHAT_CONNECTION_CBLK*)chat_connection;
    if (CHAT_CONNECTION_STATE_CLOSED != master_cblk_ptr->state)
    {
        return STATUS_INVALID_STATE;
    }

    status = network_watcher_destroy(master_cblk_ptr->network_watcher);
    assert(STATUS_SUCCESS == status);

    status = message_queue_destroy(master_cblk_ptr->event_queue);
    assert(STATUS_SUCCESS == status);

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);

    status = chat_event_io_destroy(master_cblk_ptr->io);
    assert(STATUS_SUCCESS == status);

    generic_deallocator(master_cblk_ptr);
    return STATUS_SUCCESS;
}
