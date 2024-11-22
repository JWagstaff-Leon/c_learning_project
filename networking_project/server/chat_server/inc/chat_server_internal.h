#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_server.h"

#include <pthread.h>
#include <stdint.h>

#include "chat_server_connections.h"
#include "common_types.h"


// Constants -------------------------------------------------------------------

#define CHAT_SERVER_SERVER_ORIGIN 0

char* k_server_event_canned_messages[] = {
    "",                                    // CHAT_EVENT_UNDEFINED
    "",                                    // CHAT_EVENT_CHAT_MESSAGE
    "",                                    // CHAT_EVENT_CONNECTION_FAILED
    "Your message content was too large",  // CHAT_EVENT_OVERSIZED_CONTENT
    "Please enter your username",          // CHAT_EVENT_USERNAME_REQUEST
    "",                                    // CHAT_EVENT_USERNAME_SUBMIT
    "Username accepted",                   // CHAT_EVENT_USERNAME_ACCEPTED
    "Username rejected",                   // CHAT_EVENT_USERNAME_REJECTED
    "Server is shutting down",             // CHAT_EVENT_SERVER_SHUTDOWN
    "",                                    // CHAT_EVENT_USER_LIST
    "",                                    // CHAT_EVENT_USER_JOIN
    "",                                    // CHAT_EVENT_USER_LEAVE
};


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_SERVER_STATE_INIT = 0,
    CHAT_SERVER_STATE_OPEN,
    CHAT_SERVER_STATE_CLOSED
} eCHAT_SERVER_STATE;


typedef struct
{
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    eCHAT_SERVER_STATE                  state;
    void /* TODO Implementation TBD */ *message_queue;

    sCHAT_SERVER_CONNECTIONS connections;
} sCHAT_SERVER_CBLK;


// Functions -------------------------------------------------------------------

void* chat_server_process_thread_entry(
    void *arg);


eSTATUS chat_server_do_open(
    sCHAT_SERVER_CBLK* master_cblk_ptr);


eSTATUS chat_server_do_poll(
    sCHAT_SERVER_CONNECTIONS* connections);


eSTATUS chat_server_process_connections_events(
    sCHAT_SERVER_CONNECTIONS* connections);


void chat_server_do_reset(
    sCHAT_SERVER_CONNECTIONS* connections);


void chat_server_do_close(
    sCHAT_SERVER_CBLK* master_cblk_ptr);

#ifdef __cplusplus
}
#endif
