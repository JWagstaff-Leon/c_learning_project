#include "chat_auth.h"
#include "chat_auth_internal.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include "sqlite3.h"

#include "chat_event.h"
#include "chat_user.h"
#include "common_types.h"


static const char sql_init[] =
    "CREATE TABLE IF NOT EXISTS users("                                        "\n"
        "id INT64 NOT NULL primary key,"                                       "\n"
        "username varchar(" mSTRINGIFY(CHAT_USER_MAX_NAME_SIZE) ") NOT NULL,"  "\n"
        "password varchar(" mSTRINGIFY(CHAT_EVENT_MAX_DATA_SIZE) ") NOT NULL"  "\n"
    ") default charset utf-8;";


static const char sql_create_user[] =
    "INSERT INTO users"                  "\n"
        "(id, username, password)"       "\n"
        "VALUES"                         "\n"
        "(:id, :username, :password);";


static const char sql_select_first_user[] =
    "SELECT id, username, password"   "\n"
        "FROM users"                  "\n"
        "WHERE username = :username"  "\n"
        "LIMIT 1;";


eSTATUS chat_auth_sql_init_database(
    sqlite3* database)
{
    int sqlite_status;

    sqlite_status = sqlite3_exec(database, sql_init, NULL, NULL, NULL);
    if (SQLITE_OK != sqlite_status)
    {
        return STATUS_FAILURE;
    }

    return STATUS_SUCCESS;
}


eSTATUS chat_auth_sql_create_user(
    sqlite3*               database,
    sCHAT_USER_CREDENTIALS credentials,
    CHAT_USER_ID           id)
{
    eSTATUS       status;
    int           sqlite_status;
    sqlite3_stmt* sql_statement = NULL;

    sqlite_status = sqlite3_prepare_v2(database,
                                       sql_create_user,
                                       sizeof(sql_create_user),
                                       NULL,
                                       &sql_statement);
    if (SQLITE_OK != sqlite_status || NULL == sql_statement)
    {
        return STATUS_ALLOC_FAILED;
    }

    // Bind id
    sqlite_status = sqlite3_bind_int64(sql_statement,
                                       sqlite3_bind_parameter_index(sql_statement, "id"),
                                       id);
    if (SQLITE_OK != sqlite_status)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    // Bind username
    sqlite_status = sqlite3_bind_text(sql_statement,
                                      sqlite3_bind_parameter_index(sql_statement, "username"),
                                      credentials.username,
                                      credentials.username_size,
                                      SQLITE_STATIC);
    if (SQLITE_OK != sqlite_status)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    // Bind password
    sqlite_status = sqlite3_bind_text(sql_statement,
                                      sqlite3_bind_parameter_index(sql_statement, "password"),
                                      credentials.password,
                                      credentials.password_size,
                                      SQLITE_STATIC);
    if (SQLITE_OK != sqlite_status)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    do
    {
        sqlite_status = sqlite3_step(statement);
        if (SQLITE_BUSY == sqlite_status)
        {
            if (retry_count++ < CHAT_AUTH_SQL_MAX_TRIES)
            {
                assert(0 == usleep(CHAT_AUTH_SQL_RETRY_MS));
            }
        }
    } while(SQLITE_BUSY == sqlite_status && retry_count < CHAT_AUTH_SQL_MAX_TRIES);

    if (retry_count >= CHAT_AUTH_SQL_MAX_TRIES)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    status = STATUS_SUCCESS;

func_exit:
    sqlite_status = sqlite3_finalize(sql_statement);
    assert(SQLITE_OK == sqlite_status);

    return status;
}


eSTATUS chat_auth_sql_auth_user(
    sqlite3*               database,
    sCHAT_USER_CREDENTIALS credentials,
    sCHAT_USER*            out_user)
{
    eSTATUS status;
    int     sqlite_status;
    int     strcmp_status;

    sqlite3_stmt* sql_statement = NULL;

    int retry_count = 0;

    sqlite_status = sqlite3_prepare_v2(database,
                                       sql_select_first_user,
                                       sizeof(sql_select_first_user),
                                       NULL,
                                       &sql_statement);
    if (SQLITE_OK != sqlite_status || NULL == sql_statement)
    {
        return STATUS_ALLOC_FAILED;
    }

    sqlite_status = sqlite3_bind_text(sql_statement,
                                      sqlite3_bind_parameter_index(sql_statement, "username"),
                                      credentials.username,
                                      credentials.username_size,
                                      SQLITE_STATIC);
    if (SQLITE_OK != sqlite_status)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    do
    {
        sqlite_status = sqlite3_step(statement);
        if (SQLITE_BUSY == sqlite_status)
        {
            if (retry_count++ < CHAT_AUTH_SQL_MAX_TRIES)
            {
                assert(0 == usleep(CHAT_AUTH_SQL_RETRY_MS));
            }
        }
    } while(SQLITE_BUSY == sqlite_status && retry_count < CHAT_AUTH_SQL_MAX_TRIES);

    if (retry_count >= CHAT_AUTH_SQL_MAX_TRIES)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    if (SQLITE_ROW == sqlite_status) // User exists in the database; check the password
    {
        if (NULL != credentials.password) // Password given; check it
        {
            strcmp_status = strcmp(credentials,
                                   sqlite3_column_text(sql_statement, 2));
            if (0 == strcmp_status) // Password matches; user is authenticated
            {
                out_user->id = sqlite3_column_int64(sql_statement, 0);
                status       = print_string_to_buffer(out_user->name,
                                                      sqlite3_column_text(sql_statement, 1),
                                                      sizeof(out_user->name),
                                                      NULL);
                assert(STATUS_SUCCESS == status);
                
                status = STATUS_SUCCESS;
            }
            else // 0 != strcmp_status; Provided password does not match
            {
                status = STATUS_INCOMPLETE;
            }
        }
        else // NULL == credentials.password; No password provided
        {
            status = STATUS_EMPTY;
        }
    }
    else // SQLITE_ROW != sqlite_status; User does not exist in the database
    {
        status = STATUS_NOT_FOUND;
    }

func_exit:
    sqlite_status = sqlite3_finalize(sql_statement);
    assert(SQLITE_OK == sqlite_status);

    return status;
}
