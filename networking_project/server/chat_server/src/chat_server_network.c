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

#include "chat_event.h"
#include "chat_event_io.h"
#include "common_types.h"


static const char* k_server_event_canned_messages[] = {
    "",                                    // CHAT_EVENT_UNDEFINED
    "",                                    // CHAT_EVENT_CHAT_MESSAGE
    "",                                    // CHAT_EVENT_CONNECTION_FAILED
    "Your message content was too large",  // CHAT_EVENT_OVERSIZED_CONTENT
    "Please enter your username",          // CHAT_EVENT_USERNAME_REQUEST
    "",                                    // CHAT_EVENT_USERNAME_SUBMIT
    "Username accepted",                   // CHAT_EVENT_USERNAME_ACCEPTED
    "",                                    // CHAT_EVENT_USERNAME_REJECTED
    "Server is shutting down",             // CHAT_EVENT_SERVER_SHUTDOWN
    "",                                    // CHAT_EVENT_USER_LIST
    "",                                    // CHAT_EVENT_USER_JOIN
    "",                                    // CHAT_EVENT_USER_LEAVE
};

static const char k_server_full_message[]   = "Server is full";
static const char k_name_too_long_message[] = "Username is too long";


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
                    16); // REVIEW change this to something dynamic?
    if(status < 0)
    {
        close(listen_socket);
        return STATUS_FAILURE;
    }

    connection->pollfd.fd     = listen_socket;
    connection->pollfd.events = POLLIN;

    return STATUS_SUCCESS;
}


eSTATUS chat_server_network_poll(
    sCHAT_SERVER_CONNECTIONS* connections)
{
    assert(NULL != connections);

    // TODO make this static as part of connections struct OR use allocator?
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


static void add_connection(
    sCHAT_SERVER_CONNECTIONS* connections,
    int                       new_connection_fd,
    fGENERIC_ALLOCATOR        allocator)
{
    sCHAT_EVENT              event_buffer;
    sCHAT_SERVER_CONNECTION* new_connection = NULL;
    int                      print_status;

    assert(NULL != connections);

    if (connections->count >= connections->size)
    {
        event_buffer.type   = CHAT_EVENT_CONNECTION_FAILED;
        event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
        snprintf((char*)&event_buffer.data[0], CHAT_EVENT_MAX_DATA_SIZE, "%s", &k_server_full_message[0]);
        event_buffer.length = sizeof(k_server_full_message);

        send(new_connection_fd,
             &event_buffer,
             CHAT_EVENT_HEADER_SIZE + event_buffer.length,
             0);
        close(new_connection_fd);
        return;
    }

    for (size_t connection_slot = 1; connection_slot < connections->size; connection_slot++)
    {
        if (CHAT_SERVER_CONNECTION_STATE_DISCONNECTED == connections->list[connection_slot].state)
        {
            new_connection = &connections->list[connection_slot];
            break;
        }
    }

    new_connection->pollfd.fd     = new_connection_fd;
    new_connection->pollfd.events = POLLIN | POLLOUT | POLLHUP;
    new_connection->state         = CHAT_SERVER_CONNECTION_STATE_CONNECTED;

    connections->count += 1;
}


eSTATUS chat_server_process_connections_events(
    sCHAT_SERVER_CONNECTIONS* connections,
    fGENERIC_ALLOCATOR        allocator)
{
    eSTATUS     status;
    sCHAT_EVENT event_buffer;

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
            event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
            event_buffer.type   = CHAT_EVENT_USERNAME_REQUEST;
            snprintf((char*)&event_buffer.data[0], CHAT_EVENT_MAX_DATA_SIZE, "%s", k_server_event_canned_messages[CHAT_EVENT_USERNAME_REQUEST]);
            event_buffer.length = strnlen(k_server_event_canned_messages[CHAT_EVENT_USERNAME_REQUEST], CHAT_EVENT_MAX_DATA_SIZE - 1);

            send(current_connection->pollfd.fd,
                 &event_buffer,
                 CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                 0);

            current_connection->state = CHAT_SERVER_CONNECTION_STATE_IN_SETUP;
            continue;
        }

        if (current_connection->pollfd.revents & POLLIN)
        {
            status = chat_event_io_read_from_fd(&current_connection->event_reader,
                                                current_connection->pollfd.fd);
            if (STATUS_CLOSED == status)
            {
                // TODO close connection
                continue;
            }
            else if (STATUS_INCOMPLETE == status)
            {
                continue;
            }
            else if (STATUS_SUCCESS != status)
            {
                // REVIEW what to do in case of an error?
                continue;
            }

            if (!current_connection->event_reader.in_progress)
            {
                switch(current_connection->event_reader.event.type)
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
                                        &current_connection->event_reader.event,
                                        CHAT_EVENT_HEADER_SIZE + current_connection->event_reader.event.length,
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
                            event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
                            event_buffer.type   = CHAT_EVENT_USERNAME_SUBMIT;
                            snprintf((char*)&event_buffer.data[0], CHAT_EVENT_MAX_DATA_SIZE, "%s", connections->list[0].name);
                            event_buffer.length = strnlen(&event_buffer.data[0], CHAT_EVENT_MAX_DATA_SIZE - 1);

                            send(current_connection->pollfd.fd,
                                &event_buffer,
                                CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                                0);
                        }
                        break;
                    }
                    case CHAT_EVENT_USERNAME_SUBMIT:
                    {
                        if (CHAT_SERVER_CONNECTION_STATE_IN_SETUP == current_connection->state)
                        {
                            if (current_connection->event_reader.event.length <= CHAT_SERVER_CONNECTION_MAX_NAME_SIZE)
                            {
                                // Compose name accepted message to joining user
                                event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
                                event_buffer.type   = CHAT_EVENT_USERNAME_ACCEPTED;
                                snprintf((char*)&event_buffer.data[0], CHAT_EVENT_MAX_DATA_SIZE, "%s", k_server_event_canned_messages[CHAT_EVENT_USERNAME_ACCEPTED]);
                                event_buffer.length = strnlen(k_server_event_canned_messages[CHAT_EVENT_USERNAME_ACCEPTED], CHAT_EVENT_MAX_DATA_SIZE - 1);

                                send(current_connection->pollfd.fd,
                                    &event_buffer,
                                    CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                                    0);

                                snprintf(current_connection->name,
                                        sizeof(current_connection->name),
                                        "%s",
                                        &current_connection->event_reader.event.data[0]);

                                current_connection->state = CHAT_SERVER_CONNECTION_STATE_ACTIVE;

                                // Compose message to active users about joining user
                                event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
                                event_buffer.type   = CHAT_EVENT_USER_JOIN;
                                // TODO do this to all snprintf's while applicable:
                                // Use snprintf return to determine event length
                                snprintf((char*)&event_buffer.data[0],
                                        CHAT_EVENT_MAX_DATA_SIZE,
                                        "%s",
                                        current_connection->name);
                                event_buffer.length = strnlen(&current_connection->name[0], CHAT_EVENT_MAX_DATA_SIZE - 1);

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
                                            &event_buffer,
                                            CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                                            0);
                                    }
                                }
                            }
                            else
                            {
                                event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
                                event_buffer.type   = CHAT_EVENT_USERNAME_REJECTED;
                                snprintf((char*)&event_buffer.data, CHAT_EVENT_MAX_DATA_SIZE, "%s", &k_name_too_long_message[0]);
                                event_buffer.length = sizeof(k_name_too_long_message);

                                send(current_connection->pollfd.fd,
                                    &event_buffer,
                                    CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                                    0);
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
            }
        }


        {
            current_connection->event_reader.event.origin = connection_index;

            status = STATUS_SUCCESS;


            if (CHAT_EVENT_READER_STATE_FLUSHED == current_connection->event_reader.state)
            {
                event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
                event_buffer.type   = CHAT_EVENT_OVERSIZED_CONTENT;
                snprintf((char*)&event_buffer.data, CHAT_EVENT_MAX_DATA_SIZE, "%s", k_server_event_canned_messages[CHAT_EVENT_OVERSIZED_CONTENT]);
                event_buffer.length = strnlen(k_server_event_canned_messages[CHAT_EVENT_OVERSIZED_CONTENT], CHAT_EVENT_MAX_DATA_SIZE - 1);

                send(current_connection->pollfd.fd,
                     &event_buffer,
                     CHAT_EVENT_HEADER_SIZE + event_buffer.length,
                     0);
            }

            if (STATUS_SUCCESS == status)
            {
                current_connection->event_reader.state = CHAT_EVENT_READER_STATE_NEW;
            }
        }
    }

    if (connections->list[0].pollfd.revents & POLLIN)
    {
        new_address_length = sizeof(new_address);
        new_connection_fd  = accept4(connections->list[0].pollfd.fd,
                                     (struct sockaddr*)&new_address,
                                     (socklen_t*)&new_address_length,
                                     SOCK_NONBLOCK);
        if (new_connection_fd >= 0)
        {
            add_connection(connections, new_connection_fd, allocator);
        }
    }

    return STATUS_SUCCESS;
}


static void shutdown_connection(
    sCHAT_SERVER_CONNECTIONS* connections,
    uint32_t                  connection_index,
    const sCHAT_EVENT*        event_buffer)
{
    struct pollfd pollfd;

    assert(NULL != connections);
    assert(connection_index < connections->size);

    pollfd = connections->list[connection_index].pollfd;

    if (pollfd.revents & POLLOUT && NULL != event_buffer)
    {
        send(pollfd.fd,
             event_buffer,
             CHAT_EVENT_HEADER_SIZE + event_buffer->length,
             0);
    }

    chat_event_io_close(connections->list[connection_index].event_reader);
    chat_event_io_close(connections->list[connection_index].event_writer);

    close(pollfd.fd);

    connections->list[connection_index] = k_blank_user;
    connections->count -= 1;
}


static void shutdown_connections(
    sCHAT_SERVER_CONNECTIONS* connections)
{
    sCHAT_EVENT event_buffer;
    uint32_t    connection_index;

    sCHAT_SERVER_CONNECTION* current_connection;

    assert(NULL != connections);

    event_buffer.type   = CHAT_EVENT_SERVER_SHUTDOWN;
    event_buffer.origin = CHAT_EVENT_ORIGIN_SERVER;
    snprintf(&event_buffer.data[0],
             sizeof(event_buffer.data),
             "%s",
             k_server_event_canned_messages[CHAT_EVENT_SERVER_SHUTDOWN]);
    event_buffer.length = strnlen(k_server_event_canned_messages[CHAT_EVENT_SERVER_SHUTDOWN], CHAT_EVENT_MAX_DATA_SIZE - 1);

    chat_server_network_poll(connections);
    for (connection_index = 1;
         connection_index < connections->size;
         connection_index++)
    {
        current_connection = &connections->list[connection_index];
        if (current_connection->state > CHAT_SERVER_CONNECTION_STATE_CONNECTED)
        {
            shutdown_connection(connections,
                                connection_index,
                                &event_buffer);
        }
    }
}


void chat_server_network_close(
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
