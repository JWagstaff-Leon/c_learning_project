#include "chat_clients.h"
#include "chat_clients_internal.h"
#include "chat_clients_fsm.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "message_queue.h"


static eSTATUS fsm_cblk_init(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    // TODO this
    return STATUS_SUCCESS;
}


static void fsm_cblk_close(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    fCHAT_CLIENTS_USER_CBACK user_cback;
    void*                        user_arg;

    assert(NULL != master_cblk_ptr);

    // TODO close the cblk

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    // TODO free the cblk

    user_cback(user_arg,
               CHAT_CLIENTS_EVENT_CLOSED,
               NULL);
}


static void chat_connection_close(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    uint32_t            connection_index)
{
    eSTATUS           status;
    int               close_status;
    sCHAT_CONNECTION* connection;

    if (connection_index > master_cblk_ptr->connection_count)
    {
        return;
    }

    connection = &master_cblk_ptr->connections[connection_index];

    status = chat_event_io_close(connection->io);
    assert(STATUS_SUCCESS == status);
    connection->io = NULL;

    status = message_queue_destroy(connection->event_queue);
    assert(STATUS_SUCCESS == status);
    connection->event_queue = NULL;

    if (connection->fd >= 0)
    {
        close_status = close(connection->fd);
        assert(0 == close_status);
    }

    connection->fd      = -1;
    connection->state   = CHAT_CONNECTION_STATE_DISCONNECTED;
    connection->user.id = CHAT_USER_INVALID_ID;

    memset(connection->user.name,
           0,
           sizeof(connection->user.name));

    master_cblk_ptr->read_watches[connection_index].active = false;
    master_cblk_ptr->write_watches[connection_index].active = false;
}


static void check_closed_connections(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    uint32_t connection_index;

    sNETWORK_WATCHER_WATCH* relevant_watch;

    for (connection_index = 1; connection_index < master_cblk_ptr->connection_count; connection_index++)
    {
        if (master_cblk_ptr->read_watches[connection_index].closed)
        {
            chat_connection_close(master_cblk_ptr, connection_index);
        }
    }
}


static void check_read_ready_connections(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    uint32_t connection_index;

    sNETWORK_WATCHER_WATCH* relevant_watch;
    sCHAT_CONNECTION*       relevant_connection;

    bCHAT_EVENT_IO_RESULT chat_event_io_result;
    sCHAT_EVENT           event_buffer;

    uCHAT_CLIENTS_CBACK_DATA callback_data;

    for (connection_index = 1; connection_index < master_cblk_ptr->connection_count; connection_index++)
    {
        relevant_watch      = &master_cblk_ptr->read_watches[connection_index];
        relevant_connection = relevant_watch->fd_ptr - offsetof(sCHAT_CONNECTION, fd); // REVIEW should this just be the index in connections?

        if (relevant_watch->active && relevant_watch->ready)
        {
            chat_event_io_result = chat_event_io_read_from_fd(relevant_connection->io,
                                                              relevant_connection->fd);
            switch (chat_event_io_result)
            {
                case CHAT_EVENT_IO_RESULT_READ_FINISHED:
                {
                    do
                    {
                        chat_event_io_result = chat_event_io_extract_read_event(relevant_connection->io,
                                                                                &event_buffer);
                        if (CHAT_EVENT_IO_RESULT_EXTRACT_FINISHED & chat_event_io_result)
                        {
                            callback_data.incoming_event.user         = &relevant_connection->user;
                            callback_data.incoming_event.event.type   = event_buffer.type;
                            callback_data.incoming_event.event.origin = relevant_connection->user.id;
                            callback_data.incoming_event.event.length = event_buffer.length;
                            memcpy(callback_data.incoming_event.event.data,
                                   event_buffer.data,
                                   sizeof(callback_data.incoming_event.event.data));

                            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                        CHAT_CLIENTS_EVENT_INCOMING_EVENT,
                                                        &callback_data);
                        }
                    } while (CHAT_EVENT_IO_RESULT_EXTRACT_MORE & chat_event_io_result);

                    break;
                }
                case CHAT_EVENT_IO_RESULT_FD_CLOSED:
                {
                    chat_connection_close(master_cblk_ptr, connection_index);
                    break;
                }
            }
        }
    }
}


static void check_write_ready_connections(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    eSTATUS status;

    uint32_t connection_index;

    sNETWORK_WATCHER_WATCH* relevant_watch;
    sCHAT_CONNECTION*       relevant_connection;

    bCHAT_EVENT_IO_RESULT chat_event_io_result;
    sCHAT_EVENT           event_buffer;

    for (connection_index = 1; connection_index < master_cblk_ptr->connection_count; connection_index++)
    {
        relevant_watch      = &master_cblk_ptr->write_watches[connection_index];
        relevant_connection = relevant_watch->fd_ptr - offsetof(sCHAT_CONNECTION, fd); // REVIEW should this just be the index in connections?

        while (relevant_watch->active && relevant_watch->ready)
        {
            chat_event_io_result = chat_event_io_write_to_fd(relevant_connection->io,
                                                             relevant_connection->fd);
            switch (chat_event_io_result)
            {
                case CHAT_EVENT_IO_RESULT_WRITE_FINISHED:
                {
                    if (message_queue_get_count(relevant_connection->event_queue) > 0)
                    {
                        status = message_queue_get(relevant_connection->event_queue,
                                                   &event_buffer,
                                                   sizeof(event_buffer));
                        assert(STATUS_SUCCESS == status);

                        chat_event_io_result = chat_event_io_populate_writer(relevant_connection->io,
                                                                             &event_buffer);
                        assert(CHAT_EVENT_IO_RESULT_POPULATE_SUCCESS == chat_event_io_result);
                    }
                    else
                    {
                        relevant_watch->active = false;
                    }

                    break;
                }
                case CHAT_EVENT_IO_RESULT_INCOMPLETE:
                {
                    relevant_watch->ready = false;
                }
                case CHAT_EVENT_IO_RESULT_FD_CLOSED:
                {
                    chat_connection_close(master_cblk_ptr, connection_index);
                    relevant_watch->active = false;
                    break;
                }
            }
        }
    }
}


static eSTATUS realloc_connections(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    uint32_t                new_max_connections)
{
    eSTATUS status;

    uint32_t          connection_index;
    sCHAT_CONNECTION* new_connections;

    sNETWORK_WATCHER_WATCH* new_read_watches;
    sNETWORK_WATCHER_WATCH* new_write_watches;

    new_connections = generic_allocator(sizeof(sCHAT_CONNECTION) * new_max_connections);
    if (NULL == new_connections)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_connections;
    }

    new_read_watches = generic_allocator(sizeof(sNETWORK_WATCHER_WATCH) * new_max_connections);
    if (NULL == new_read_watches)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_read_watches;
    }

    new_write_watches = generic_allocator(sizeof(sNETWORK_WATCHER_WATCH) * new_max_connections);
    if (NULL == new_write_watches)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_write_watches;
    }

    // Close any connections in excess of new_max_connections
    for (connection_index = new_connections;
         connection_index < master_cblk_ptr->connection_count;
         connection_index++)
    {
        chat_connection_close(master_cblk_ptr, connection_index);
    }

    // Copy any connections overlapping
    for (connection_index = 0;
         connection_index < master_cblk_ptr->max_connections && connection_index < new_max_connections;
         connection_index++)
    {
        new_connections[connection_index].fd          = master_cblk_ptr->connections[connection_index].fd;
        new_connections[connection_index].io          = master_cblk_ptr->connections[connection_index].io;
        new_connections[connection_index].state       = master_cblk_ptr->connections[connection_index].state;
        new_connections[connection_index].event_queue = master_cblk_ptr->connections[connection_index].event_queue;

        // FIXME replace snprintf's with print_string_to_buffer's
        snprintf(new_connections[connection_index].name,
                sizeof(new_connections[connection_index].name),
                "%s",
                master_cblk_ptr->connections[connection_index].name);

        new_read_watches[connection_index].fd_ptr = &new_connections[connection_index].fd;
        new_read_watches[connection_index].active = master_cblk_ptr->read_watches.active;

        new_write_watches[connection_index].fd_ptr = &new_connections[connection_index].fd;
        new_write_watches[connection_index].active = master_cblk_ptr->write_watches.active;
    }

    // Initialize any new connections
    for (connection_index = master_cblk_ptr->max_connections;
         connection_index < new_max_connections;
         connection_index++)
    {
        memcpy(new_connections[connection_index],
               k_blank_connection,
               sizeof(new_connections[connection_index]));

        new_write_watches[connection_index].fd_ptr = &new_connections[connection_index].fd;
        new_write_watches[connection_index].active = false;
    }

    generic_deallocator(master_cblk_ptr->connections);
    generic_deallocator(master_cblk_ptr->read_watches);
    generic_deallocator(master_cblk_ptr->write_watches);

    master_cblk_ptr->connections     = new_connections;
    master_cblk_ptr->max_connections = new_max_connections;
    master_cblk_ptr->read_watches    = new_read_watches;
    master_cblk_ptr->write_watches   = new_write_watches;

    return STATUS_SUCCESS;

fail_alloc_write_watches:
    generic_deallocator(new_read_watches);

fail_alloc_read_watches:
    generic_deallocator(new_connections);

fail_alloc_connections:
    return status;
}


static void check_new_connections(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    eSTATUS           status;
    uint32_t          connection_index;
    sCHAT_CONNECTION* relevant_connection;
    int               new_connection_fd;

    if (master_cblk_ptr->read_watches[0].ready)
    {
        if (master_cblk_ptr->connection_count >= master_cblk_ptr->max_connections)
        {
            realloc_connections(master_cblk_ptr,
                                master_cblk_ptr->connection_count + 10); // REVIEW make this 10 configurable?
        }

        relevant_connection = NULL;
        for (connection_index = 1;
            relevant_connection == NULL && connection_index < master_cblk_ptr->max_connections;
            connection_index++)
        {
            if (CHAT_CONNECTION_STATE_DISCONNECTED == master_cblk_ptr->connections[connection_index].state)
            {
                relevant_connection = &master_cblk_ptr->connections[connection_index];
            }
        }

        if (NULL != relevant_connection)
        {
            status = chat_clients_accept_new_connection(master_cblk_ptr->connections[0].fd,
                                                            &new_connection_fd);
            if (STATUS_SUCCESS == status)
            {
                status = chat_connection_open(relevant_connection,
                                            new_connection_fd);
                if (STATUS_SUCCESS != status)
                {
                    close(new_connection_fd);
                }
            }
        }
    }
}


static void open_processing(
    sCHAT_CLIENTS_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENTS_MESSAGE* message)
{
    eSTATUS status;
    int     new_connection_fd;

    uint32_t      client_index;
    sCHAT_CLIENT* relevant_client;

    switch (message->type)
    {
        case CHAT_CLIENTS_MESSAGE_INCOMING_EVENT:
        {
            chat_clients_process_event(master_cblk_ptr,
                                       message->params.incoming_event.client_ptr,
                                       &message->params.incoming_event.event);
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLIENT_CLOSED:
        {
            // TODO close the client
            break;
        }
        case CHAT_CLIENTS_MESSAGE_NEW_CONNECTION:
        {
            if (master_cblk_ptr->connection_count >= master_cblk_ptr->max_connections)
            {
                status = realloc_connections(master_cblk_ptr,
                                             master_cblk_ptr->connection_count + 10); // REVIEW make this 10 configurable?
                if (STATUS_SUCCESS != status)
                {
                    // TODO handle failure
                }

                relevant_client = NULL;
                for (client_index = 1;
                    relevant_client == NULL && client_index < master_cblk_ptr->max_clients;
                    client_index++)
                {
                    if (NULL == master_cblk_ptr->client_list[client_index])
                    {
                        relevant_client = &master_cblk_ptr->client_list[client_index];
                    }
                }

                status = chat_clients_accept_new_connection(master_cblk_ptr->connections[0].fd,
                                                            &new_connection_fd);
                if (STATUS_SUCCESS == status)
                {
                    status = chat_clients_client_open(master_cblk_ptr,
                                                      relevant_client,
                                                      new_connection_fd);
                    if (STATUS_SUCCESS != status)
                    {
                        close(new_connection_fd);
                    }
                }
            }
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLOSE:
        {
            // TODO start closing
            break;
        }
        case CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_ERROR:
        case CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_CANCELLED:
        case CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_CLOSED:
        default:
        {
            // Should not get here
            assert(0);
            break;
        }
    }
}


static void closing_processing(
    sCHAT_CLIENTS_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENTS_MESSAGE* message)
{
    eSTATUS status;

    switch (message->type)
    {
        case CHAT_CLIENTS_MESSAGE_INCOMING_EVENT:
        {
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLIENT_CLOSED:
        {
            break;
        }
        case CHAT_CLIENTS_MESSAGE_NEW_CONNECTION:
        {
            break;
        }
        case CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_CANCELLED:
        {
            break;
        }
        case CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_ERROR:
        {
            break;
        }
        case CHAT_CLIENTS_MESSAGE_NEW_CONNECTION_WATCH_CLOSED:
        {
            break;
        }
    }
}


void dispatch_message(
    sCHAT_CLIENTS_CBLK*          master_cblk_ptr,
    const sCHAT_CLIENTS_MESSAGE* message)
{
    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (master_cblk_ptr->state)
    {
        case CHAT_CLIENTS_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CLIENTS_STATE_CLOSING:
        {
            closing_processing(master_cblk_ptr, message);
            break;
        }
        case CHAT_CLIENTS_STATE_CLOSED:
        {
            // Should never get here
            assert(0);
        }
    }
}


void* chat_clients_thread_entry(
    void* arg)
{
    sCHAT_CLIENTS_CBLK* master_cblk_ptr;
    eSTATUS                 status;

    sCHAT_CLIENTS_MESSAGE message;

    assert(NULL != arg);
    master_cblk_ptr = arg;

    status = fsm_cblk_init(master_cblk_ptr);
    assert(STATUS_SUCCESS == status);

    while (CHAT_CLIENTS_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
        dispatch_message(master_cblk_ptr, &message);
    }

    fsm_close_cblk(master_cblk_ptr);
    return NULL;
}
