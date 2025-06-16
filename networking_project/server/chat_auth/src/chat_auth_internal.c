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
    ");";


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
    sqlite3*                      database,
    const sCHAT_USER_CREDENTIALS* credentials,
    CHAT_USER_ID                  id)
{
    eSTATUS       status;
    int           sqlite_status;
    sqlite3_stmt* sql_statement = NULL;

    sqlite_status = sqlite3_prepare_v2(database,
                                       sql_create_user,
                                       sizeof(sql_create_user),
                                       &sql_statement,
                                       NULL);
    if (SQLITE_OK != sqlite_status || NULL == sql_statement)
    {
        return STATUS_ALLOC_FAILED;
    }

    // Bind id
    sqlite_status = sqlite3_bind_int64(sql_statement,
                                       sqlite3_bind_parameter_index(sql_statement, ":id"),
                                       id);
    if (SQLITE_OK != sqlite_status)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    // Bind username
    sqlite_status = sqlite3_bind_text(sql_statement,
                                      sqlite3_bind_parameter_index(sql_statement, ":username"),
                                      credentials->username,
                                      credentials->username_size,
                                      SQLITE_STATIC);
    if (SQLITE_OK != sqlite_status)
    {
        status = STATUS_FAILURE;
        goto func_exit;
    }

    // Bind password
    sqlite_status = sqlite3_bind_text(sql_statement,
                                      sqlite3_bind_parameter_index(sql_statement, ":password"),
                                      credentials->password,
                                      credentials->password_size,
                                      SQLITE_STATIC);
    if (SQLITE_OK != sqlite_status)
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


eCHAT_AUTH_RESULT chat_auth_sql_auth_user(
    sqlite3*                      database,
    const sCHAT_USER_CREDENTIALS* credentials,
    sCHAT_USER*                   out_user)
{
    eSTATUS           status;
    eCHAT_AUTH_RESULT result;
    int               sqlite_status;
    int               strcmp_status;

    sqlite3_stmt* sql_statement = NULL;

    sqlite_status = sqlite3_prepare_v2(database,
                                       sql_select_first_user,
                                       sizeof(sql_select_first_user),
                                       &sql_statement,
                                       NULL);
    if (SQLITE_OK != sqlite_status || NULL == sql_statement)
    {
        return CHAT_AUTH_RESULT_FAILURE;
    }

    if (NULL == credentials->username)
    {
        result = CHAT_AUTH_RESULT_USERNAME_REQUIRED;
        goto func_exit;
    }

    if (credentials->username_size > CHAT_USER_MAX_NAME_SIZE)
    {
        result = CHAT_AUTH_RESULT_USERNAME_REJECTED;
        goto func_exit;
    }

    sqlite_status = sqlite3_bind_text(sql_statement,
                                      sqlite3_bind_parameter_index(sql_statement, ":username"),
                                      credentials->username,
                                      credentials->username_size,
                                      SQLITE_STATIC);
    if (SQLITE_OK != sqlite_status)
    {
        result = CHAT_AUTH_RESULT_FAILURE;
        goto func_exit;
    }

    sqlite_status = sqlite3_step(sql_statement);
    switch (sqlite_status)
    {
        case SQLITE_ROW: // Username was found in the database
        {
            if (NULL != credentials->password) // Password given; check it
            {
                strcmp_status = strcmp(credentials->password,
                                       sqlite3_column_text(sql_statement, 2));
                if (0 == strcmp_status) // Password matches; user is authenticated
                {
                    out_user->id = sqlite3_column_int64(sql_statement, 0);
                    status       = print_string_to_buffer(out_user->name,
                                                          sqlite3_column_text(sql_statement, 1),
                                                          sizeof(out_user->name),
                                                          NULL);
                    if (STATUS_SUCCESS != status)
                    {
                        result = CHAT_AUTH_RESULT_FAILURE;
                    }
                    else
                    {
                        result = CHAT_AUTH_RESULT_AUTHENTICATED;
                    }

                }
                else // 0 != strcmp_status; Provided password does not match
                {
                    result = CHAT_AUTH_RESULT_PASSWORD_REJECTED;
                }
            }
            else // NULL == credentials->password; No password provided
            {
                result = CHAT_AUTH_RESULT_PASSWORD_REQUIRED;
            }

            break;
        }
        case SQLITE_DONE: // Username was not found in the database
        {
            if (NULL != credentials->password) // Password given; use it as the password for a new account
            {
                // TODO generate id
                // out_user->id = ?;
                status = chat_auth_sql_create_user(database,
                                                   credentials,
                                                   out_user->id);
                if (STATUS_SUCCESS != status)
                {
                    result = CHAT_AUTH_RESULT_FAILURE;
                    break;
                }

                status = print_string_to_buffer(out_user->name,
                                                credentials->username,
                                                sizeof(out_user->name),
                                                NULL);
                if (STATUS_SUCCESS != status)
                {
                    result = CHAT_AUTH_RESULT_FAILURE;
                    break;
                }

                result = CHAT_AUTH_RESULT_AUTHENTICATED;
            }
            else // No password given; prompt for the creation of a password for the new user
            {
                result = CHAT_AUTH_RESULT_PASSWORD_CREATION;
            }
            break;
        }
        default:
        {
            result = CHAT_AUTH_RESULT_FAILURE;
            break;
        }

    }

func_exit:
    sqlite_status = sqlite3_finalize(sql_statement);
    assert(SQLITE_OK == sqlite_status);

    return result;
}
