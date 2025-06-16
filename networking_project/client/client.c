#include <errno.h>
#include <ncurses.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "chat_event.h"
#include "chat_connection.h"
#include "chat_user.h"
#include "common_types.h"
#include "message_queue.h"


void chat_connection_cback(
    void* user_arg,
    bCHAT_CONNECTION_EVENT_TYPE event_mask,
    const sCHAT_CONNECTION_CBACK_DATA* data)
{
    if (event_mask & CHAT_CONNECTION_EVENT_INCOMING_EVENT)
    {
        message_queue_put(user_arg, &data->incoming_event.event, sizeof(data->incoming_event.event));
    }
}


void print_event(
    const sCHAT_EVENT* event)
{
    printf("------------------------------\n"
           "| Event type: %d\n"
           "| Event origin: %lu\n"
           "| Event data: %s\n"
           "------------------------------\n",
           event->type,
           event->origin,
           event->data);
}


int main(int argc, char *argv[])
{
    eSTATUS status;
    sCHAT_EVENT event;
    int stdlib_status;

    int connection_fd;
    CHAT_CONNECTION connection;
    MESSAGE_QUEUE incoming_events;
    char* input_buffer;
    size_t input_size;
    struct pollfd connect_poll;
    socklen_t sockopt_len;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    status = message_queue_create(&incoming_events, 32, sizeof(sCHAT_EVENT));
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "message_queue_create failed\n");
        close(connection_fd);
        return 1;
    }

    status = chat_connection_create(&connection, chat_connection_cback, incoming_events, connection_fd);
    if (STATUS_SUCCESS != status)
    {
        fprintf(stderr, "chat_connection_create failed\n");
        message_queue_destroy(incoming_events);
        close(connection_fd);
        return 1;
    }

    printf("Starting main loop...\n");
    while(1)
    {
        printf("Waiting for new events...\n");
        message_queue_get(incoming_events, &event, sizeof(event));
        print_event(&event);

        input_buffer = NULL;
        input_size = 0;
        printf("Outgoing event type: ");
        getline(&input_buffer, &input_size, stdin);
        sscanf(input_buffer, "%d", (int*)(&event.type));
        free(input_buffer);
        input_buffer = NULL;
        input_size = 0;
        printf("Outgoing event data: ");
        getline(&input_buffer, &input_size, stdin);
        input_buffer[input_size] = '\0';

        chat_event_populate(&event, event.type, 0, input_buffer);
        print_event(&event);
        chat_connection_queue_event(connection, &event);

        free(input_buffer);
        input_buffer = NULL;
        input_size = 0;
    }

    return 0;
}
