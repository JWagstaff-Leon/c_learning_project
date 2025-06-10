#include "chat_server.h"
#include "chat_server_internal.h"
#include "chat_server_fsm.h"

#include <assert.h>
#include <unistd.h>

#include "chat_auth.h"
#include "chat_clients.h"
#include "chat_event.h"
#include "common_types.h"
#include "message_queue.h"


static void open_processing(
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
{
    eSTATUS              status;
    sCHAT_SERVER_MESSAGE new_message;

    sCHAT_USER              user_info;
    int                     new_connection_fd;
    eCHAT_CLIENTS_AUTH_STEP auth_step;

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_CLOSE:
        {
            status = chat_clients_close(master_cblk_ptr->clients);
            assert(STATUS_SUCCESS == status);

            status = network_watcher_close(master_cblk_ptr->connection_listener);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSING;
            break;
        }
        case CHAT_SERVER_MESSAGE_START_AUTH_TRANSACTION:
        {
            status = chat_auth_submit_credentials(master_cblk_ptr->auth,
                                                  message->params.clients.start_auth_transaction.credentials,
                                                  message->params.clients.start_auth_transaction.client_ptr,
                                                  message->params.clients.start_auth_transaction.auth_transaction_container);
            if (STATUS_SUCCESS != status)
            {
                status = chat_clients_auth_event(master_cblk_ptr->clients,
                                                 CHAT_CLIENTS_AUTH_STEP_CLOSED,
                                                 user_info,
                                                 message->params.clients.start_auth_transaction.client_ptr);
                assert(STATUS_SUCCESS == status);
            }
            break;
        }
        case CHAT_SERVER_MESSAGE_FINISH_AUTH_TRANSACTION:
        {
            status = chat_auth_finish_transaction(message->params.clients.finish_auth_transaction.auth_transaction);
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CHAT_SERVER_MESSAGE_CLIENTS_CLOSED:
        {
            master_cblk_ptr->clients = NULL;
            
            status = chat_auth_close(master_cblk_ptr->auth);
            assert(STATUS_SUCCESS == status);

            status = network_watcher_close(master_cblk_ptr->connection_listener);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSING;
            break;
        }
        case CHAT_SERVER_MESSAGE_INCOMING_CONNECTION:
        {
            status = chat_server_accept_connection(master_cblk_ptr,
                                                   &new_connection_fd);
            break;
        }
        case CHAT_SERVER_MESSAGE_LISTENER_ERROR:
        {
            status = chat_clients_close(master_cblk_ptr->clients);
            assert(STATUS_SUCCESS == status);

            status = network_watcher_close(master_cblk_ptr->connection_listener);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSING;
            break;
        }
        case CHAT_SERVER_MESSAGE_LISTENER_CLOSED:
        {
            master_cblk_ptr->connection_listener = NULL;

            status = chat_clients_close(master_cblk_ptr->clients);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSING;
            break;
        }
        case CHAT_SERVER_MESSAGE_AUTH_RESULT:
        {
            switch (message->params.auth.auth_result.result)
            {
                case CHAT_AUTH_RESULT_USERNAME_REQUIRED:
                {
                    auth_step = CHAT_CLIENTS_AUTH_STEP_USERNAME_REQUIRED;
                    break;
                }
                case CHAT_AUTH_RESULT_USERNAME_REJECTED:
                {
                    auth_step = CHAT_CLIENTS_AUTH_STEP_USERNAME_REJECTED;
                    break;
                }
                case CHAT_AUTH_RESULT_PASSWORD_CREATION:
                {
                    auth_step = CHAT_CLIENTS_AUTH_STEP_PASSWORD_CREATION;
                    break;
                }
                case CHAT_AUTH_RESULT_PASSWORD_REQUIRED:
                {
                    auth_step = CHAT_CLIENTS_AUTH_STEP_PASSWORD_REQUIRED;
                    break;
                }
                case CHAT_AUTH_RESULT_PASSWORD_REJECTED:
                {
                    auth_step = CHAT_CLIENTS_AUTH_STEP_PASSWORD_REJECTED;
                    break;
                }
                case CHAT_AUTH_RESULT_AUTHENTICATED:
                {
                    auth_step = CHAT_CLIENTS_AUTH_STEP_AUTHENTICATED;
                    break;
                }
                case CHAT_AUTH_RESULT_FAILURE:
                {
                    auth_step = CHAT_CLIENTS_AUTH_STEP_CLOSED;
                    break;
                }
            }
            status = chat_clients_auth_event(master_cblk_ptr->clients,
                                             auth_step,
                                             message->params.auth.auth_result.user_info,
                                             message->params.auth.auth_result.client_ptr);
            assert(STATUS_SUCCESS == status);
            break;
        }
        case CHAT_SERVER_MESSAGE_AUTH_CLOSED:
        {
            master_cblk_ptr->auth = NULL;

            status = chat_clients_close(master_cblk_ptr->clients);
            assert(STATUS_SUCCESS == status);

            status = network_watcher_close(master_cblk_ptr->connection_listener);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSING;
            break;
        }
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}


static void closing_processing(
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
{
    eSTATUS status;

    switch (message->type)
    {
        case CHAT_SERVER_MESSAGE_CLIENTS_CLOSED:
        {
            master_cblk_ptr->clients = NULL;
            if (NULL != master_cblk_ptr->auth)
            {
                status = chat_auth_close(master_cblk_ptr->auth);
                assert(STATUS_SUCCESS == status);
            }

            if (NULL == master_cblk_ptr->auth && master_cblk_ptr->connection_listener)
            {
                master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSED;
            }
            break;
        }
        case CHAT_SERVER_MESSAGE_AUTH_CLOSED:
        {
            master_cblk_ptr->auth = NULL;

            if (NULL == master_cblk_ptr->clients && master_cblk_ptr->connection_listener)
            {
                master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSED;
            }
            break;
        }
        case CHAT_SERVER_MESSAGE_LISTENER_CLOSED:
        {
            master_cblk_ptr->connection_listener = NULL;

            if (NULL == master_cblk_ptr->clients && master_cblk_ptr->auth)
            {
                master_cblk_ptr->state = CHAT_SERVER_STATE_CLOSED;
            }
            break;
        }
        case CHAT_SERVER_MESSAGE_START_AUTH_TRANSACTION:
        case CHAT_SERVER_MESSAGE_FINISH_AUTH_TRANSACTION:
        case CHAT_SERVER_MESSAGE_INCOMING_CONNECTION:
        case CHAT_SERVER_MESSAGE_LISTENER_CANCELLED:
        case CHAT_SERVER_MESSAGE_LISTENER_ERROR:
        case CHAT_SERVER_MESSAGE_AUTH_RESULT:
        {
            // Ignore any of these messages
            break;
        }
        default:
        {
            // Should never get here
            assert(0);
        }
    }
}


static void dispatch_message(
    const sCHAT_SERVER_MESSAGE* message,
    sCHAT_SERVER_CBLK*          master_cblk_ptr)
{
    assert(NULL != message);
    assert(NULL != master_cblk_ptr);

    switch (master_cblk_ptr->state)
    {
        case CHAT_SERVER_STATE_OPEN:
        {
            open_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_SERVER_STATE_CLOSING:
        {
            closing_processing(message, master_cblk_ptr);
            break;
        }
        case CHAT_SERVER_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
            break;
        }
    }
}


static void fsm_cblk_init(
    sCHAT_SERVER_CBLK *master_cblk_ptr)
{
    eSTATUS status;

    status = open_listen_socket(&master_cblk_ptr->listen_fd);
    assert(STATUS_SUCCESS == status);
    
    status = network_watcher_start_watch(master_cblk_ptr->connection_listener,
                                         NETWORK_WATCHER_MODE_READ,
                                         master_cblk_ptr->listen_fd);
    assert(STATUS_SUCCESS == status);
}


static void fsm_cblk_close(
    sCHAT_SERVER_CBLK* master_cblk_ptr)
{
    eSTATUS                 status;
    fCHAT_SERVER_USER_CBACK user_cback;
    void*                   user_arg;

    assert(NULL != master_cblk_ptr);

    close(master_cblk_ptr->listen_fd);

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);

    user_cback = master_cblk_ptr->user_cback;
    user_arg   = master_cblk_ptr->user_arg;

    generic_deallocator(master_cblk_ptr);

    user_cback(user_arg,
               CHAT_SERVER_EVENT_CLOSED,
               NULL);
}


void* chat_server_thread_entry(
    void* arg)
{
    sCHAT_SERVER_CBLK *master_cblk_ptr;
    sCHAT_SERVER_MESSAGE message;
    eSTATUS status;

    assert(NULL != arg);
    master_cblk_ptr = (sCHAT_SERVER_CBLK*)arg;

    fsm_cblk_init(master_cblk_ptr);
    while (CHAT_SERVER_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
        dispatch_message(&message, master_cblk_ptr);
    }

    fsm_cblk_close(master_cblk_ptr);
    return NULL;
}
