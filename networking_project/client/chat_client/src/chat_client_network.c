#include "chat_client.h"
#include "chat_client_internal.h"

#include <arpa/inet.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "common_types.h"

eSTATUS chat_client_network_open(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    struct sockaddr_in address)
{
    int                new_fd;
    int                connect_status;
    
    assert(NULL != master_cblk_ptr);
    
    new_fd = socket(address.sin_family,
                    SOCK_STREAM,
                    0);
    if (new_fd < 0)
    {
        return STATUS_FAILURE;
    }

    master_cblk_ptr->server_connection.fd = new_fd;
    connect_status = connect(master_cblk_ptr->server_connection.fd,
                             address,
                             sizeof(address));
    if (connect_status < 0)
    {
        close(master_cblk_ptr->server_connection.fd);
        master_cblk_ptr->server_connection.fd = -1;
        return STATUS_FAILURE;
    }

    master_cblk_ptr->server_connection.events  = POLLIN | POLLOUT;
    master_cblk_ptr->server_connection.revents = 0;

    return STATUS_SUCCESS;
}


eSTATUS chat_client_network_send_username(
    int               server_fd,
    const char* const username)
{
    sCHAT_EVENT outgoing_event;

    outgoing_event.type   = CHAT_EVENT_USERNAME_SUBMIT;
    outgoing_event.origin = 0;
    outgoing_event.length = strlen(message->params.send_text.text);
    snprintf(&outgoing_event.data[0],
             sizeof(outgoing_event.data),
             "%s",
             message->params.send_text.text);
    
    // FIXME properly buffer this send
    send(server_fd,
         &outgoing_event,
         sizeof(outgoing_event),
         NULL);

    return STATUS_SUCCESS;
}


eSTATUS chat_client_network_send_message(
    int               server_fd,
    const char* const message_text)
{
    sCHAT_EVENT outgoing_event;

    outgoing_event.type   = CHAT_EVENT_CHAT_MESSAGE;
    outgoing_event.origin = 0;
    outgoing_event.length = strlen(message->params.send_text.text);
    snprintf(&outgoing_event.data[0],
             sizeof(outgoing_event.data),
             "%s",
             message->params.send_text.text);
    
    // FIXME properly buffer this send
    send(server_fd,
         &outgoing_event,
         sizeof(outgoing_event),
         NULL);

    return STATUS_SUCCESS;
}
