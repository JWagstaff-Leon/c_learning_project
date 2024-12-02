#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_connections.h"

#include <arpa/inet.h>
#include <assert.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common_types.h"


sCHAT_EVENT k_event_buffer;


eSTATUS chat_server_network_open(
    sCHAT_SERVER_CONNECTION* connection)
{
    int status;

    int listen_socket;
    int opt_value = 1;

    struct sockaddr_in address;

    assert(NULL != connection);

    listen_socket = socket(AF_INET,
                           SOCK_STREAM,
                           0);
    if (listen_socket < 0)
    {
        return STATUS_FAILURE;
    }

    status = setsockopt(listen_socket,
                        SOL_SOCKET,
                        SO_REUSEADDR | SO_REUSEPORT,
                        &opt_value,
                        sizeof(opt_value));
    if (0 != status)
    {
        close(listen_socket);
        return STATUS_FAILURE;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);
    memset(address.sin_zero, 0, sizeof(address.sin_zero));

    status = bind(listen_socket,
                  (struct sockaddr *)&address,
                  sizeof(address));
    if (status < 0)
    {
        close(listen_socket);
        return STATUS_FAILURE;
    }

    status = listen(listen_socket,
                    16);
    if(status < 0)
    {
        close(listen_socket);
        return STATUS_FAILURE;
    }

    connection->pollfd.fd = listen_socket;
    return STATUS_SUCCESS;
}


eSTATUS chat_server_network_poll(
    sCHAT_SERVER_CONNECTIONS* connections)
{
    assert(NULL != connections);

    struct pollfd *fds = calloc(connections->size,
                                sizeof(struct pollfd));
    if (NULL == fds)
    {
        return STATUS_ALLOC_FAILED;
    }

    for (int connection = 0; connection < connections->size; connection++)
    {
        fds[connection] = connections->list[connection].pollfd;
    }

    poll(fds, connections->size, 0);

    for (int connection = 0; connection < connections->size; connection++)
    {
        connections->list[connection].pollfd = fds[connection];
    }

    free(fds);
    return STATUS_SUCCESS;
}


static eSTATUS read_from_fd(
    int                                fd,
    sCHAT_SERVER_CONNECTION_READ_INFO* read_info)
{
    ssize_t read_bytes;
    size_t  remaining_bytes = read_info->expected_bytes - read_info->read_bytes;

    switch (read_info->state)
    {
        case CHAT_SERVER_CONNECTION_READ_STATE_NEW:
        {
            read_info->expected_bytes = sizeof(read_info->buffer);
            read_info->read_bytes     = 0;
            memset(&read_info->buffer,
                   0,
                   sizeof(read_info->buffer));

            read_bytes = read(fd,
                              (void*)(&read_info->buffer),
                              sizeof(read_info->buffer));
            if (read_bytes < 0)
            {
                return STATUS_FAILURE;
            }

            read_info->read_bytes = read_bytes;
            if (read_info->read_bytes >= CHAT_EVENT_HEADER_SIZE)
            {
                read_info->expected_bytes = CHAT_EVENT_HEADER_SIZE + read_info->buffer.event_length;
                assert(read_info->read_bytes <= read_info->expected_bytes);

                if(read_info->expected_bytes > sizeof(read_info->buffer))
                {
                    read_info->state = CHAT_SERVER_CONNECTION_READ_STATE_FLUSHING;
                    return STATUS_NO_SPACE;
                }
                
                if (read_info->read_bytes >= read_info->expected_bytes)
                {
                    read_info->state = CHAT_SERVER_CONNECTION_READ_STATE_DONE;
                }
                else
                {
                    read_info->state = CHAT_SERVER_CONNECTION_READ_STATE_DATA;
                }
            }
            else
            {
                read_info->state = CHAT_SERVER_CONNECTION_READ_STATE_HEADER;
            }

            break;
        }
        case CHAT_SERVER_CONNECTION_READ_STATE_HEADER:
        {
            read_bytes = read(fd,
                              (void*)((size_t)&read_info->buffer + read_info->read_bytes),
                              remaining_bytes);
            if (read_bytes < 0)
            {
                return STATUS_FAILURE;
            }

            read_info->read_bytes += read_bytes;
            if (read_info->read_bytes >= CHAT_EVENT_HEADER_SIZE)
            {
                read_info->expected_bytes = CHAT_EVENT_HEADER_SIZE + read_info->buffer.event_length;
                assert(read_info->read_bytes <= read_info->expected_bytes);

                if(read_info->expected_bytes > sizeof(read_info->buffer))
                {
                    read_info->state = CHAT_SERVER_CONNECTION_READ_STATE_FLUSHING;
                    return STATUS_NO_SPACE;
                }

                if (read_info->read_bytes >= read_info->expected_bytes)
                {
                    read_info->state = CHAT_SERVER_CONNECTION_READ_STATE_DONE;
                }
                else
                {
                    read_info->state = CHAT_SERVER_CONNECTION_READ_STATE_DATA;
                }
            }

            break;
        }
        case CHAT_SERVER_CONNECTION_READ_STATE_DATA:
        {
            read_bytes = read(fd,
                              (void*)((size_t)&read_info->buffer + read_info->read_bytes),
                              remaining_bytes);
            if (read_bytes < 0)
            {
                return STATUS_FAILURE;
            }

            read_info->read_bytes += read_bytes;
            assert(read_info->read_bytes <= read_info->expected_bytes);

            if (read_info->read_bytes >= read_info->expected_bytes)
            {
                read_info->state = CHAT_SERVER_CONNECTION_READ_STATE_DONE;
            }

            break;
        }
        case CHAT_SERVER_CONNECTION_READ_STATE_DONE:
        {
            break;
        }
        case CHAT_SERVER_CONNECTION_READ_STATE_FLUSHING:
        {
            if (remaining_bytes > sizeof(read_info->buffer))
            {
                remaining_bytes = sizeof(read_info->buffer);
            }

            read_bytes = read(fd,
                              &read_info->buffer,
                              remaining_bytes);
            if (read_bytes > 0)
            {
                read_info->read_bytes += read_bytes;
            }

            if (read_info->read_bytes >= read_info->expected_bytes)
            {
                read_info->state = CHAT_SERVER_CONNECTION_READ_STATE_FLUSHED;
                return STATUS_SUCCESS;
            }
            return STATUS_FAILURE;
        }
    }

    return STATUS_SUCCESS;
}


static void add_connection(
    sCHAT_SERVER_CONNECTIONS* connections,
    int                       new_connection_fd)
{
    assert(NULL != connections);

    if (connections->count >= connections->size)
    {
        k_event_buffer.event_type   = CHAT_EVENT_CONNECTION_FAILED;
        k_event_buffer.event_origin = CHAT_EVENT_ORIGIN_SERVER;
        snprintf(&k_event_buffer.event_data, CHAT_EVENT_MAX_LENGTH, "%s", &k_server_full_message[0]);
        k_event_buffer.event_length = sizeof(k_server_full_message);

        send(new_connection_fd,
             &k_event_buffer,
             CHAT_EVENT_HEADER_SIZE + k_event_buffer.event_length,
             0);
        close(new_connection_fd);
        return;
    }
}


eSTATUS chat_server_process_connections_events(
    sCHAT_SERVER_CONNECTIONS* connections)
{
    eSTATUS status;

    uint32_t                 connection_index;
    sCHAT_SERVER_CONNECTION* current_connection;

    uint32_t send_connection_index;

    int                new_connection_fd;
    struct sockaddr_in new_address;
    int                new_address_length;

    assert(NULL != connections);

    for (connection_index = 1;
         connection_index < connections->size;
         connection_index++)
    {
        current_connection = &connections->list[connection_index];

        if (CHAT_SERVER_CONNECTION_STATE_CONNECTED == current_connection->state && current_connection->pollfd.revents & POLLOUT)
        {
            k_event_buffer.event_origin = CHAT_EVENT_ORIGIN_SERVER;
            k_event_buffer.event_type   = CHAT_EVENT_USERNAME_REQUEST;
            snprintf(&k_event_buffer.event_data, CHAT_EVENT_MAX_LENGTH, "%s", k_server_event_canned_messages[CHAT_EVENT_USERNAME_REQUEST]);
            k_event_buffer.event_length = strnlen(k_server_event_canned_messages[CHAT_EVENT_USERNAME_REQUEST], CHAT_EVENT_MAX_LENGTH - 1);

            send(current_connection->pollfd.fd,
                 &k_event_buffer,
                 CHAT_EVENT_HEADER_SIZE + k_event_buffer.event_length,
                 0);

            current_connection->state = CHAT_SERVER_CONNECTION_STATE_IN_SETUP;
            continue;
        }

        if (current_connection->pollfd.revents & POLLIN)
        {
            status = read_from_fd(current_connection->pollfd.fd,
                                  &current_connection->read_info);
            if (STATUS_SUCCESS != status)
            {
                continue;
            }
        }

        if (CHAT_SERVER_CONNECTION_READ_STATE_DONE == current_connection->read_info.state)
        {
            current_connection->read_info.buffer.event_origin = connection_index;

            status = STATUS_SUCCESS;
            switch(current_connection->read_info.buffer.event_type)
            {
                case CHAT_EVENT_CHAT_MESSAGE:
                {
                    if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == current_connection->state)
                    {
                        for (send_connection_index = 1;
                             send_connection_index < connections->size;
                             send_connection_index++)
                        {
                            if (send_connection_index == connection_index)
                            {
                                continue;
                            }

                            if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == connections->list[send_connection_index].state)
                            {
                                send(connections->list[send_connection_index].pollfd.fd,
                                     &current_connection->read_info.buffer,
                                     CHAT_EVENT_HEADER_SIZE + current_connection->read_info.buffer.event_length,
                                     0);
                            }
                        }
                    }
                    break;
                }
                case CHAT_EVENT_USERNAME_REQUEST:
                {
                    if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == current_connection->state)
                    {
                        k_event_buffer.event_origin = CHAT_EVENT_ORIGIN_SERVER;
                        k_event_buffer.event_type   = CHAT_EVENT_USERNAME_SUBMIT;
                        snprintf(&k_event_buffer.event_data, CHAT_EVENT_MAX_LENGTH, "%s", connections->list[0].name);
                        k_event_buffer.event_length = strnlen(&k_event_buffer.event_data[0], CHAT_EVENT_MAX_LENGTH - 1);

                        send(current_connection->pollfd.fd,
                             &k_event_buffer,
                             CHAT_EVENT_HEADER_SIZE + k_event_buffer.event_length,
                             0);
                    }
                    break;
                }
                case CHAT_EVENT_USERNAME_SUBMIT:
                {
                    if (CHAT_SERVER_CONNECTION_STATE_IN_SETUP == current_connection->state)
                    {
                        // Compose message to joining user
                        k_event_buffer.event_origin = CHAT_EVENT_ORIGIN_SERVER;
                        k_event_buffer.event_type   = CHAT_EVENT_USERNAME_ACCEPTED;
                        snprintf(&k_event_buffer.event_data, CHAT_EVENT_MAX_LENGTH, "%s", k_server_event_canned_messages[CHAT_EVENT_USERNAME_ACCEPTED]);
                        k_event_buffer.event_length = strnlen(k_server_event_canned_messages[CHAT_EVENT_USERNAME_ACCEPTED], CHAT_EVENT_MAX_LENGTH - 1);
    
                        send(current_connection->pollfd.fd,
                             &k_event_buffer,
                             CHAT_EVENT_HEADER_SIZE + k_event_buffer.event_length,
                             0);
                            
                        current_connection->state = CHAT_SERVER_CONNECTION_STATE_ACTIVE;
    
                        // Compose message to active users about joining user
                        k_event_buffer.event_origin = CHAT_EVENT_ORIGIN_SERVER;
                        k_event_buffer.event_type   = CHAT_EVENT_USER_JOIN;
                        snprintf(&k_event_buffer.event_data, CHAT_EVENT_MAX_LENGTH, "%s", current_connection->read_info.buffer.event_data);
                        k_event_buffer.event_length = strnlen(&k_event_buffer.event_data[0], CHAT_EVENT_MAX_LENGTH - 1);
    
                        for (send_connection_index = 1;
                        send_connection_index < connections->size;
                        send_connection_index++)
                        {
                            if (send_connection_index == connection_index)
                            {
                                continue;
                            }
    
                            if (CHAT_SERVER_CONNECTION_STATE_ACTIVE == connections->list[send_connection_index].state)
                            {
                                send(connections->list[send_connection_index].pollfd.fd,
                                     &k_event_buffer,
                                     CHAT_EVENT_HEADER_SIZE + k_event_buffer.event_length,
                                     0);
                            }
                        }
                    }
                    break;
                }
                case CHAT_EVENT_USER_LIST:
                {
                    break;
                }
                
                // All these are no-op if sent to the server
                case CHAT_EVENT_UNDEFINED:
                case CHAT_EVENT_CONNECTION_FAILED:
                case CHAT_EVENT_OVERSIZED_CONTENT:
                case CHAT_EVENT_USERNAME_ACCEPTED:
                case CHAT_EVENT_USERNAME_REJECTED:
                case CHAT_EVENT_SERVER_SHUTDOWN:
                case CHAT_EVENT_USER_JOIN:
                case CHAT_EVENT_USER_LEAVE:
                default:
                {
                    break;
                }
            }

            if (CHAT_SERVER_CONNECTION_READ_STATE_FLUSHED == current_connection->read_info.state)
            {
                k_event_buffer.event_origin = CHAT_EVENT_ORIGIN_SERVER;
                k_event_buffer.event_type   = CHAT_EVENT_OVERSIZED_CONTENT;
                snprintf(&k_event_buffer.event_data, CHAT_EVENT_MAX_LENGTH, "%s", k_server_event_canned_messages[CHAT_EVENT_OVERSIZED_CONTENT]);
                k_event_buffer.event_length = strnlen(k_server_event_canned_messages[CHAT_EVENT_OVERSIZED_CONTENT], CHAT_EVENT_MAX_LENGTH - 1);

                send(current_connection->pollfd.fd,
                     &k_event_buffer,
                     CHAT_EVENT_HEADER_SIZE + k_event_buffer.event_length,
                     0);
            }
            
            if (STATUS_SUCCESS == status)
            {
                current_connection->read_info.state = CHAT_SERVER_CONNECTION_READ_STATE_NEW;
            }
        }
    }

    if (connections->list[0].pollfd.revents & POLLIN)
    {
        new_address_length = sizeof(new_address);
        new_connection_fd  = accept(connections->list[0].pollfd.fd,
                                    (struct sockaddr*)&new_address,
                                    (socklen_t*)&new_address_length);
        if (new_connection_fd >= 0)
        {
            add_connection(connections, new_connection_fd);
        }
    }

    return STATUS_SUCCESS;
}


static shutdown_connections(
    sCHAT_SERVER_CONNECTIONS* connections)
{
    assert(NULL != connections);
    // TODO send shutdown message to all clients

    // TODO close each connection individually

    // TODO free connection memory
}


void chat_server_network_reset(
    sCHAT_SERVER_CONNECTIONS *connections)
{
    assert(NULL != connections);

    if (connections->list[0].pollfd.fd >= 0)
    {
        shutdown_connections(connections);
        shutdown(connections->list[0].pollfd.fd, SHUT_RDWR);
        close(connections->list[0].pollfd.fd);
    }

    connections->list[0].pollfd.fd = -1;
    connections->list[0].state     = CHAT_SERVER_CONNECTION_STATE_DISCONNECTED;
}


void chat_server_network_close(
    sCHAT_SERVER_CBLK* master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);
    // TODO write this function
}
