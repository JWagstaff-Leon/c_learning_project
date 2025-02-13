#include "chat_connections.h"
#include "chat_connections_internal.h"
#include "chat_connections_fsm.h"

#include <assert.h>
#include <stddef.h>

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


static void open_processing(
    sCHAT_CONNECTIONS_CBLK*          master_cblk_ptr,
    const sCHAT_CONNECTIONS_MESSAGE* message)
{
    assert(NULL != master_cblk_ptr);
    assert(NULL != message);

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_READ_READY:
        {
            for (result_index = 0; result_index < message->params.read_ready.index_count; result_index++)
            {
                connection_index    = message->params.read_ready.connection_indecies[result_index];
                relevant_connection = &master_cblk_ptr->connections.list[connection_index];

                chat_event_io_result = chat_event_io_read_from_fd(relevant_connection->io,
                                                                  relevant_connection->fd);
                switch (chat_event_io_result.event)
                {
                    case CHAT_EVENT_IO_RESULT_READ_FINISHED:
                    {
                        do
                        {
                            chat_event_io_result = chat_event_io_extract_read_event(relevant_connection->io,
                                                                                    &event_buffer);
                            if (CHAT_EVENT_IO_RESULT_EXTRACT_FINISHED & chat_event_io_result)
                            {
                                process_complete_event_from(&master_cblk_ptr->connections,
                                                            connection_index,
                                                            &event_buffer);
                            }
                        } while (CHAT_EVENT_IO_RESULT_EXTRACT_MORE & chat_event_io_result);
                        break;
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

            for (connection_index = 0, fd_index = 0;
                 connection_index < master_cblk_ptr->connections.size && fd_index < sizeof(fds_list) / sizeof(fds_list[0]),
                 connection_index++)
            {
                if (CHAT_SERVER_CONNECTION_STATE_DISCONNECTED != master_cblk_ptr->connections.list[connection_index].state)
                {
                    fds_list[fd_index++] = master_cblk_ptr->connections.list[connection_index].fd;
                }
            }

            generic_deallocator(message->params.read_ready.connection_indecies);
            status = chat_server_network_start_network_watch(master_cblk_ptr);
            assert(STATUS_SUCCESS == status);

            break;
        }
        case CHAT_SERVER_MESSAGE_WRITE_READY:
        {
            for (fd_index = 0; fd_index < message->params.write_ready.index_count; fd_index++)
            {
                connection_index     = message->params.write_ready.connection_indecies[fd_index];
                relevant_connection  = master_cblk_ptr->connections[connection_index].connection;

                chat_event_io_result = chat_event_io_do_operation_on_fd(&relevant_connection->event_writer,
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
