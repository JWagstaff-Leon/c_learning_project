#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Includes --------------------------------------------------------------------

#include "chat_auth.h"

#include "sqlite3.h"

#include "message_queue.h"


// Constants -------------------------------------------------------------------

#define CHAT_AUTH_MESSAGE_QUEUE_SIZE 32

#define CHAT_AUTH_SQL_MAX_TRIES   3
#define CHAT_AUTH_SQL_RETRY_MS  125


// Types -----------------------------------------------------------------------

typedef enum
{
    CHAT_AUTH_STATE_NO_DATABASE,
    CHAT_AUTH_STATE_OPEN,
    CHAT_AUTH_STATE_CLOSED
} eCHAT_AUTH_STATE;


typedef struct
{
    eCHAT_AUTH_STATE state;
    MESSAGE_QUEUE    message_queue;

    fCHAT_AUTH_USER_CBACK user_cback;
    void*                 user_arg;

    sqlite3* database;
} sCHAT_AUTH_CBLK;

// Functions -------------------------------------------------------------------

void* chat_auth_thread_entry(
    void* arg);


eSTATUS chat_auth_sql_init_database(
        sqlite3* database);


eSTATUS chat_auth_sql_create_user(
    sqlite3*               database,
    sCHAT_USER_CREDENTIALS credentials,
    CHAT_USER_ID           id);


eSTATUS chat_auth_sql_auth_user(
    sqlite3*               database,
    sCHAT_USER_CREDENTIALS credentials,
    sCHAT_USER*            out_user);


#ifdef __cplusplus
}
#endif
