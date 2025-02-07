#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Defines ---------------------------------------------------------------------

#define CHAT_SERVER_CONNECTION_MAX_NAME_SIZE      65
#define CHAT_SERVER_CONNECTION_MESSAGE_QUEUE_SIZE 32


// Includes --------------------------------------------------------------------

#include <poll.h>
#include <stddef.h>
#include <stdint.h>

#include "chat_event.h"
#include "chat_event_io.h"
#include "message_queue.h"


// Types -----------------------------------------------------------------------

typedef enum {
    CHAT_SERVER_CONNECTION_STATE_DISCONNECTED,
    CHAT_SERVER_CONNECTION_STATE_INIT,
    CHAT_SERVER_CONNECTION_STATE_SETUP,
    CHAT_SERVER_CONNECTION_STATE_ACTIVE
} eCHAT_SERVER_CONNECTION_STATE;


typedef enum
{
    CHAT_SERVER_CONNECTION_MESSAGE_SEND_EVENT
} eCHAT_SERVER_CONNECTION_MESSAGE_TYPE;


typedef struct
{
    const sCHAT_EVENT* event;
} sCHAT_SERVER_CONNECTION_SEND_EVENT_PARAMS;


typedef union
{
    sCHAT_SERVER_CONNECTION_SEND_EVENT_PARAMS send_event;
} uCHAT_SERVER_CONNECTION_MESSAGE_PARAMS;


typedef struct
{
    eCHAT_SERVER_CONNECTION_MESSAGE_TYPE   type;
    uCHAT_SERVER_CONNECTION_MESSAGE_PARAMS params;
} sCHAT_SERVER_CONNECTION_MESSAGE;


typedef struct
{
    int                           fd;
    CHAT_EVENT_IO                 io;
    eCHAT_SERVER_CONNECTION_STATE state;
    char                          name[CHAT_SERVER_CONNECTION_MAX_NAME_SIZE];
    MESSAGE_QUEUE                 event_buffer;
} sCHAT_SERVER_CONNECTION;


typedef struct
{
    sCHAT_SERVER_CONNECTION* list;
    uint32_t                 count;
    uint32_t                 size;
} sCHAT_SERVER_CONNECTIONS;


// Constants -------------------------------------------------------------------

static const sCHAT_SERVER_CONNECTION k_blank_user = {
    .fd           = -1,
    .io           = NULL,
    .state        = CHAT_SERVER_CONNECTION_STATE_DISCONNECTED,
    .name         = "",
    .event_buffer = NULL
};


// Functions -------------------------------------------------------------------

eSTATUS chat_server_connections_new_connection(
    sCHAT_SERVER_CONNECTIONS* connections,
    int                       connection_fd);


eSTATUS chat_server_connections_get_readable(
    sCHAT_SERVER_CONNECTIONS* connections,
    int*                      out_indecies,
    uint32_t*                 out_indecies_count);

    
eSTATUS chat_server_connections_get_writeable(
    sCHAT_SERVER_CONNECTIONS* connections,
    int*                      out_indecies,
    uint32_t*                 out_indecies_count);


eSTATUS chat_server_connection_queue_event(
    sCHAT_SERVER_CONNECTION* connection,
    const sCHAT_EVENT*       event);


eSTATUS chat_server_connection_do_read(
    sCHAT_SERVER_CONNECTION* connection);

    
eSTATUS chat_server_connection_do_write(
    sCHAT_SERVER_CONNECTION* connection);


#ifdef __cplusplus
}
#endif
