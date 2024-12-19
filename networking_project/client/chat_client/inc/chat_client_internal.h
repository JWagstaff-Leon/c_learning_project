#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_client.h"

#include <netinet/in.h>
#include <poll.h>

#include "chat_event.h"
#include "chat_event_io.h"
#include "message_queue.h"


// Constants -------------------------------------------------------------------

#define CHAT_CLIENT_MSG_QUEUE_SIZE 32


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_CLIENT_STATE_NOT_CONNECTED,
    CHAT_CLIENT_STATE_INACTIVE,
    CHAT_CLIENT_STATE_ACTIVE,
    CHAT_CLIENT_STATE_CLOSED
} eCHAT_CLIENT_STATE;


typedef enum
{
    CHAT_CLIENT_SUBFSM_SEND_STATE_READY,
    CHAT_CLIENT_SUBFSM_SEND_STATE_IN_PROGRESS
} eCHAT_CLIENT_SUBFSM_SEND_STATE;


typedef struct
{

    MESSAGE_QUEUE           message_queue;
    fCHAT_CLIENT_USER_CBACK user_cback;
    void*                   user_arg;

    fGENERIC_ALLOCATOR   allocator;
    fGENERIC_DEALLOCATOR deallocator;

    sMODULE_PARAMETERS io_params;

    struct pollfd server_connection;

    eCHAT_CLIENT_STATE             state;
    eCHAT_CLIENT_SUBFSM_SEND_STATE send_state;

    CHAT_EVENT_IO event_reader;
    CHAT_EVENT_IO event_writer;
} sCHAT_CLIENT_CBLK;


// Functions -------------------------------------------------------------------

void* chat_client_thread_entry(
    void* arg);


eSTATUS chat_client_network_open(
    sCHAT_CLIENT_CBLK* master_cblk_ptr,
    struct sockaddr_in address);

    
eSTATUS chat_client_network_poll(
    sCHAT_CLIENT_CBLK* master_cblk_ptr);

    
eSTATUS chat_client_network_disconnect(
    sCHAT_CLIENT_CBLK* master_cblk_ptr);


#ifdef __cplusplus
}
#endif
