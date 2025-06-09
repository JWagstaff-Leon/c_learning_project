#include "chat_server.h"
#include "chat_server_internal.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common_types.h"

eSTATUS open_listen_socket(
    int* out_fd)
{
    eSTATUS status;

    int listen_fd;
    int std_lib_status;
    int opt_value = 1;

    struct sockaddr_in address;
    
    assert(NULL != master_cblk_ptr);

    listen_fd = socket(AF_INET,
                       SOCK_STREAM,
                       0);
    if (listen_fd < 0)
    {
        return STATUS_FAILURE;
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

    address.sin_family      = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port        = htons(8080);
    memset(address.sin_zero, 0, sizeof(address.sin_zero));

    std_lib_status = bind(listen_fd,
                          (struct sockaddr *)&address,
                          sizeof(address));
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

    status = STATUS_SUCCESS;
    goto success;

fail_post_open_socket:
    close(listen_socket_fd);

success:
    return status;
}


chat_server_accept_connection(
    sCHAT_SERVER_CBLK* master_cblk_ptr,
    int*               new_connection_fd)
{
    // TODO accept a connection and spit out the new fd
}
