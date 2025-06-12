#include "chat_server.h"
#include "chat_server_internal.h"

#include <arpa/inet.h>
#include <assert.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common_types.h"


eSTATUS open_listen_socket(
    int* out_fd)
{
    eSTATUS status;

    int std_lib_status;
    int listen_fd;
    int opt_value = 1;

    struct addrinfo hints, *address;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;   // Either IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags    = AI_PASSIVE;  // Use whatever IP

    std_lib_status = getaddrinfo(NULL, "8080", &hints, &address);
    if (0 != std_lib_status)
    {
        return STATUS_FAILURE;
    }

    listen_fd = socket(address->ai_family,
                       address->ai_socktype,
                       address->ai_protocol);
    if (listen_fd < 0)
    {
        status = STATUS_FAILURE;
        goto fail_post_getaddrinfo;
    }

    std_lib_status = setsockopt(listen_fd,
                                SOL_SOCKET,
                                SO_REUSEADDR | SO_REUSEPORT,
                                &opt_value,
                                sizeof(opt_value));
    if (0 != std_lib_status)
    {
        status = STATUS_FAILURE;
        goto fail_post_open_socket;
    }

    std_lib_status = bind(listen_fd,
                          address->ai_addr,
                          address->ai_addrlen);
    if (std_lib_status < 0)
    {
        status = STATUS_FAILURE;
        goto fail_post_open_socket;
    }

    std_lib_status = listen(listen_fd,
                            16); // REVIEW change this to something dynamic?
    if(std_lib_status < 0)
    {
        status = STATUS_FAILURE;
        goto fail_post_open_socket;
    }

    *out_fd = listen_fd;
    status  = STATUS_SUCCESS;
    goto success;

fail_post_open_socket:
    close(listen_fd);

fail_post_getaddrinfo:
    freeaddrinfo(address);    

success:
    return status;
}


eSTATUS chat_server_accept_connection(
    sCHAT_SERVER_CBLK* master_cblk_ptr,
    int*               new_connection_fd)
{
    int accept_status;

    struct sockaddr addr;
    socklen_t       addrlen = sizeof(struct sockaddr_storage);

    accept_status = accept(master_cblk_ptr->listen_fd,
                           &addr,
                           &addrlen);
    if (accept_status < 0)
    {
        return STATUS_FAILURE;
    }
    
    *new_connection_fd = accept_status;
    return STATUS_SUCCESS;
}
