#include "chat_connections.h"
#include "chat_connections_internal.h"
#include "chat_connections_fsm.h"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "message_queue.h"


static eSTATUS fsm_cblk_init(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr)
{
    // TODO this
    return STAUTS_SUCCESS;
}


static void fsm_cblk_close(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr)
{
    fCHAT_CONNECTIONS_USER_CBACK user_cback;
    void*                        user_arg;

    assert(NULL != master_cblk_ptr);

    // TODO close the cblk

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    // TODO free the cblk

    user_cback(user_arg,
               CHAT_CONNECTIONS_EVENT_CLOSED,
               NULL);
}


static eSTATUS chat_connection_open(
    sCHAT_CONNECTION* connection,
    int               fd)
{
    eSTATUS status;
    eSTATUS fail_status;

    status = message_queue_create(&connection->event_queue,
                                  CHAT_CONNECTION_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCHAT_EVENT));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_message_queue;
    }

    status = chat_event_io_create(&connection->io);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_event_io;
    }

    connection->fd    = fd;
    connection->state = CHAT_CONNECTION_STATE_INIT;
    memset(connection->name,
           0,
           sizeof(connection->name));

    return STATUS_SUCCESS;

fail_message_event_io:
    fail_status = message_queue_destroy(connection->event_queue);
    assert(STATUS_SUCCESS == fail_status);
    connection->event_queue = NULL;

fail_message_queue_create:
    return status;
}


static void chat_connection_close(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr,
    uint32_t                connection_index)
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
    connection->fd = -1;

    connection->state = CHAT_CONNECTION_STATE_DISCONNECTED;
    memset(connection->name,
           0,
           sizeof(connection->name));

    master_cblk_ptr->read_watches[connection_index].active = false;
    master_cblk_ptr->write_watches[connection_index].active = false;
}


static void check_closed_connections(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr)
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
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr)
{
    uint32_t connection_index;

    sNETWORK_WATCHER_WATCH* relevant_watch;
    sCHAT_CONNECTION*       relevant_connection;

    eCHAT_EVENT_IO_RESULT chat_event_io_result;
    sCHAT_EVENT           event_buffer;

    for (connection_index = 1; connection_index < master_cblk_ptr->connection_count; connection_index++)
    {
        relevant_watch      = &master_cblk_ptr->read_watches[connection_index];
        relevant_connection = relevant_watch->fd_ptr - offsetof(sCHAT_CONNECTION, fd); // REVIEW should this just be the index in connections?

        if (relevant_watch->acitve && relevant_watch->ready)
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
                            chat_connections_process_event_from(master_cblk_ptr,
                                                                relevant_connection,
                                                                &event_buffer);
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


static eSTATUS realloc_connections(
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr,
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
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr)
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
            status = chat_connections_accept_new_connection(master_cblk_ptr->connections[0].fd,
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
    sCHAT_CONNECTIONS_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTIONS_MESSAGE* message)
{
    eSTATUS status;

    switch (message->type)
    {
        case CHAT_CONNECTIONS_MESSAGE_READ_READY:
        {
            check_closed_connections(master_cblk_ptr);
            check_read_ready_connections(master_cblk_ptr);
            check_new_connections(master_cblk_ptr);

            status = network_watcher_start_watch(master_cblk_ptr->read_network_watcher,
                                                 NETWORK_WATCHER_MODE_READ,
                                                 master_cblk_ptr->read_watches,
                                                 master_cblk_ptr->connection_count);
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_CONNECTIONS_MESSAGE_WRITE_READY:
        {
            check_closed_connections(master_cblk_ptr);
            check_write_ready_connections(master_cblk_ptr);

            // TODO only set active on the write once they have a message queued to them
            for (fd_index = 0; fd_index < message->params.write_ready.index_count; fd_index++)
            {
                connection_index     = message->params.write_ready.connection_indecies[fd_index];
                relevant_connection  = master_cblk_ptr->connections[connection_index].connection;

                chat_event_io_result = chat_event_write_to_fd(&relevant_connection->event_writer,
                                                              relevant_connection->fd,
                                                              &event_buffer);
                switch (chat_event_io_result.event)
                {
                    case CHAT_EVENT_IO_RESULT_WRITE_FINISHED:
                    {
                        // TODO nothing?
                    }
                    case CHAT_EVENT_IO_RESULT_FD_CLOSED:
                    {
                        // TODO respond to a close
                    }
                    case CHAT_EVENT_IO_RESULT_FAILED:
                    {
                        // TODO respond to a failure
                    }
                    // REVIEW should this respond to flushing conditions?
                }
            }

            for (connection_index = 1, fd_index = 0; // Start at 1 for write as there's no reason to write to incoming connection socket
                 connection_index < master_cblk_ptr->connections.size && fd_index < sizeof(fds_list) / sizeof(fds_list[0]),
                 connection_index++)
            {
                if (CHAT_SERVER_CONNECTION_STATE_DISCONNECTED != master_cblk_ptr->connections.list[connection_index].state)
                {
                    fds_list[fd_index++] = master_cblk_ptr->connections.list[connection_index].fd;
                }
            }

            status = network_watcher_start_watch(master_cblk_ptr->read_network_watcher,
                                                 NETWORK_WATCHER_MODE_WRITE,
                                                 &fds_list[0],
                                                 fd_index);
            assert(STATUS_SUCCESS == status);

            break;
        }
    }
}


void dispatch_message(
    sCHAT_CONNECTIONS_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTIONS_MESSAGE* message)
{
    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (master_cblk_ptr->state)
    {
        case CHAT_CONNECTIONS_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
            break;
        }
    }
}


void* chat_connections_thread_entry(
    void* arg)
{
    sCHAT_CONNECTIONS_CBLK* master_cblk_ptr;
    eSTATUS                 status;

    sCHAT_CONNECTIONS_MESSAGE message;

    assert(NULL != arg);
    master_cblk_ptr = arg;

    status = fsm_cblk_init(master_cblk_ptr);
    assert(STATUS_SUCCESS == status);

    while (CHAT_CONNECTIONS_STATE_CLOSED != master_cblk_ptr->state)
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
