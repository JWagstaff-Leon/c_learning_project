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


// REVIEW does this function need to interact with multithreading?
static eSTATUS realloc_clients(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr,
    uint32_t            new_max_clients)
{
    eSTATUS status;

    uint32_t       client_index;
    sCHAT_CLIENT** new_client_ptr_list;

    new_client_ptr_list = generic_allocator(sizeof(sCHAT_CLIENT*) * new_max_clients);
    if (NULL == new_client_ptr_list)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_client_list;
    }

    // Close any connections in excess of new_max_connections
    for (client_index = new_max_clients;
         client_index < master_cblk_ptr->connection_count;
         client_index++)
    {
        chat_clients_client_close(master_cblk_ptr->client_ptr_list[client_index]);
    }

    // Copy any connections overlapping
    for (client_index = 0;
         client_index < master_cblk_ptr->max_connections && client_index < new_max_clients;
         client_index++)
    {
        new_client_ptr_list[client_index] = master_cblk_ptr->client_ptr_list[client_index];
    }

    // Initialize any new connections
    for (client_index = master_cblk_ptr->max_connections;
         client_index < new_max_clients;
         client_index++)
    {
        new_client_ptr_list[client_index] = NULL;
    }

    generic_deallocator(master_cblk_ptr->client_ptr_list);

    master_cblk_ptr->client_ptr_list = new_client_ptr_list;
    master_cblk_ptr->max_clients     = new_max_clients;

    return STATUS_SUCCESS;

fail_alloc_client_list:
    return status;
}


static void check_new_connections(
    sCHAT_CLIENTS_CBLK* master_cblk_ptr)
{
    eSTATUS         status;
    uint32_t        connection_index;
    CHAT_CONNECTION relevant_connection;
    int             new_connection_fd;

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

                status = chat_connection_create()
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
        case CHAT_CLIENTS_MESSAGE_OPEN_CLIENT:
        {
            if (master_cblk_ptr->client_count >= master_cblk_ptr->max_clients)
            {
                status = realloc_clients(master_cblk_ptr,
                                         master_cblk_ptr->max_clients + 10); // REVIEW make this 10 configurable?
                if (STATUS_SUCCESS != status)
                {
                    // TODO handle failure
                }

                relevant_client = NULL;
                for (client_index = 1;
                     client_index < master_cblk_ptr->max_clients;
                     client_index++)
                {
                    if (NULL == master_cblk_ptr->client_ptr_list[client_index])
                    {
                        relevant_client = generic_allocator(sizeof(sCHAT_CLIENT));
                        master_cblk_ptr->client_ptr_list[client_index] = relevant_client;
                        break;
                    }
                }

                if(NULL == relevant_client)
                {
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_CLIENTS_EVENT_CLIENT_OPEN_FAILED,
                                                NULL);
                    break;
                }

                status = chat_clients_client_init(master_cblk_ptr,
                                                  relevant_client,
                                                  message->params.open_client.fd);
                if (STATUS_SUCCESS != status)
                {
                    close(new_connection_fd);
                    generic_deallocator(relevant_client);

                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CHAT_CLIENTS_EVENT_CLIENT_OPEN_FAILED,
                                                NULL);
                }
            }
            break;
        }
        case CHAT_CLIENTS_MESSAGE_AUTH_RESULT:
        {
            // TODO respond to auth results
            break;
        }
        case CHAT_CLIENTS_MESSAGE_INCOMING_EVENT:
        {
            chat_clients_process_event(master_cblk_ptr,
                                       message->params.incoming_event.client_ptr,
                                       &message->params.incoming_event.event);
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED:
        {
            // TODO close the connection
            break;
        }
        case CHAT_CLIENTS_MESSAGE_AUTH_RESULT:
        {
            // TODO act on the auth result
            break;
        }
        case CHAT_CLIENTS_MESSAGE_CLOSE:
        {
            // TODO start closing
            break;
        }
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
        case CHAT_CLIENTS_MESSAGE_CLIENT_CONNECTION_CLOSED:
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
