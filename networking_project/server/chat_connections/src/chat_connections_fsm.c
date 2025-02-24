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


static eSTATUS chat_connection_init(
    sCHAT_CONNECTION* connection,
    int               fd)
{
    eSTATUS status;

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
    message_queue_destroy(connection->event_queue);
    connection->event_queue = NULL;

fail_message_queue_create:
    return status;
}


static void open_processing(
    sCHAT_CONNECTIONS_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTIONS_MESSAGE* message)
{
    eCHAT_EVENT_IO_RESULT chat_event_io_result;
    sCHAT_EVENT           event_buffer;
    eSTATUS               status;

    sNETWORK_WATCHER_WATCH* relevant_watch;
    sCHAT_CONNECTION*       relevant_connection;

    uint32_t result_index;
    uint32_t connection_index;

    sCHAT_CONNECTION* new_connections;
    uint32_t          new_max_connections;

    int new_connection_fd;

    switch (message->type)
    {
        case CHAT_CONNECTIONS_MESSAGE_READ_READY:
        {
            for (connection_index = 1; connection_index < master_cblk_ptr->connection_count; connection_index++)
            {
                relevant_watch      = &master_cblk_ptr->read_watches[connection_index];
                relevant_connection = relevant_watch->fd_ptr - offsetof(sCHAT_CONNECTION, fd); // REVIEW can this just be the index in connections?

                if (relevant_watch->ready)
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
                            relevant_connection->fd = -1;
                            relevant_watch->active  = false;
                        }
                    }
                }
            }

            if (master_cblk_ptr->read_watches[0].ready)
            {
                if (master_cblk_ptr->connection_count >= master_cblk_ptr->max_connections)
                {
                    new_max_connections = master_cblk_ptr->max_connections + 10;
                    assert(master_cblk_ptr->connection_count < new_max_connections);
                    new_connections = generic_allocator(sizeof(sCHAT_CONNECTION) * new_max_connections);
                    assert(NULL != new_connections);

                    for (connection_index = 0; connection_index < master_cblk_ptr->max_connections; connection_index++)
                    {
                        new_max_connections[connection_index].fd          = master_cblk_ptr->connections[connection_index].fd;
                        new_max_connections[connection_index].io          = master_cblk_ptr->connections[connection_index].io;
                        new_max_connections[connection_index].state       = master_cblk_ptr->connections[connection_index].state;
                        new_max_connections[connection_index].event_queue = master_cblk_ptr->connections[connection_index].event_queue;

                        snprintf(new_connections[connection_index].name,
                                 sizeof(new_connections[connection_index].name),
                                "%s",
                                master_cblk_ptr->connections[connection_index].name);
                    }

                    for (connection_index = master_cblk_ptr->max_connections; connection_index < new_max_connections; connection_index++)
                    {
                        new_max_connections[connection_index] = k_blank_connection;
                    }
                    
                    generic_deallocator(master_cblk_ptr->connections);

                    master_cblk_ptr->connections     = new_connections;
                    master_cblk_ptr->max_connections = new_max_connections;
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
                assert(NULL != relevant_connection);

                status = chat_connections_accept_new_connection(master_cblk_ptr->connections[0].fd,
                                                                &new_connection_fd);
                if (STATUS_SUCCESS == status)
                {
                    status = chat_connection_init(relevant_connection,
                                                  new_connection_fd);
                    if (STATUS_SUCCESS != status)
                    {
                        close(new_connection_fd);
                    }
                }
            }

            status = network_watcher_start_watch(master_cblk_ptr->read_network_watcher,
                                                 NETWORK_WATCHER_MODE_READ,
                                                 master_cblk_ptr->read_watches,
                                                 master_cblk_ptr->connection_count);
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_CONNECTIONS_MESSAGE_WRITE_READY:
        {
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
