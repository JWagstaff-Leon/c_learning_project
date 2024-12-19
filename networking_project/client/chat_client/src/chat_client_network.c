#include "chat_client.h"
#include "chat_client_internal.h"
#include "chat_client_fsm.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

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
                             (struct sockaddr*)&address,
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

eSTATUS chat_client_network_poll(
    sCHAT_CLIENT_CBLK* master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_CLIENT_MESSAGE new_message;

    assert(NULL != master_cblk_ptr);

    poll(&master_cblk_ptr->server_connection, 1, 0);

    if (master_cblk_ptr->server_connection.revents & POLLOUT &&
        CHAT_CLIENT_SUBFSM_SEND_STATE_IN_PROGRESS == master_cblk_ptr->send_state)
    {
        new_message.type = CHAT_CLIENT_MESSAGE_SEND_CONTINUE;
        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &new_message,
                                   sizeof((new_message)));
        assert(STATUS_SUCCESS == status);
    }

    if (master_cblk_ptr->server_connection.revents & POLLIN)
    {
        status = chat_event_io_read_from_fd(&master_cblk_ptr->incoming_event_reader,
                                            master_cblk_ptr->server_connection.fd);
        if (STATUS_CLOSED == status)
        {
            new_message.type = CHAT_CLIENT_MESSAGE_DISCONNECT;
            status           = message_queue_put(master_cblk_ptr->message_queue,
                                                 &new_message,
                                                 sizeof((new_message)));
            assert(STATUS_SUCCESS == status);
            return STATUS_CLOSED;
        }
        assert(STATUS_SUCCESS == status || STATUS_INCOMPLETE == status);

        if(STATUS_SUCCESS == status)
        {
            new_message.type = CHAT_CLIENT_MESSAGE_INCOMING_EVENT;
            chat_event_io_extract_read_event(&master_cblk_ptr->incoming_event_reader,
                                             &new_message.params.incoming_event.event);

            status = message_queue_put(master_cblk_ptr->message_queue,
                                       &new_message,
                                       sizeof((new_message)));
            assert(STATUS_SUCCESS == status);
        }
    }

    return STATUS_SUCCESS;
}


eSTATUS chat_client_network_disconnect(
    sCHAT_CLIENT_CBLK* master_cblk_ptr)
{
    assert(NULL != master_cblk_ptr);

    poll(&master_cblk_ptr->server_connection, 1, 0);
    if (master_cblk_ptr->server_connection.revents & POLLOUT)
    {
        write(master_cblk_ptr->server_connection.fd,
              &master_cblk_ptr->outgoing_event_writer.event,
              sizeof(master_cblk_ptr->outgoing_event_writer.event));
    }

    close(master_cblk_ptr->server_connection.fd);
    master_cblk_ptr->server_connection.fd = -1;

    return STATUS_SUCCESS;
}
