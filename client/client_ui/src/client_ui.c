#include "client_ui.h"
#include "client_ui_internal.h"
#include "client_ui_fsm.h"

#include <assert.h>
#include <ctype.h>
#include <ncurses.h>
#include <stdbool.h>
#include <string.h>

#include "chat_event.h"
#include "common_types.h"
#include "message_queue.h"


eSTATUS client_ui_create(
    CLIENT_UI*       out_new_client_ui,
    fCLIENT_UI_CBACK user_cback,
    void*            user_arg)
{
    eSTATUS          status;
    sCLIENT_UI_CBLK* new_client_ui;

    new_client_ui = generic_allocator(sizeof(sCLIENT_UI_CBLK));
    if (NULL == new_client_ui)
    {
        status = STATUS_ALLOC_FAILED;
        goto fail_alloc_cblk;
    }
    memset(new_client_ui, 0, sizeof(sCLIENT_UI_CBLK));

    new_client_ui->user_cback = user_cback;
    new_client_ui->user_arg   = user_arg;
    new_client_ui->state      = CLIENT_UI_STATE_INIT;

    status = message_queue_create(&new_client_ui->message_queue,
                                  CLIENT_UI_MESSAGE_QUEUE_SIZE,
                                  sizeof(sCLIENT_UI_MESSAGE));
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_message_queue;
    }

    *out_new_client_ui = new_client_ui;
    return STATUS_SUCCESS;

fail_create_message_queue:
    generic_deallocator(new_client_ui);

fail_alloc_cblk:
    return status;
}


eSTATUS client_ui_open(
    CLIENT_UI client_ui)
{
    eSTATUS          status;
    sCLIENT_UI_CBLK* master_cblk_ptr;

    if (NULL == client_ui)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCLIENT_UI_CBLK*)client_ui;
    if (CLIENT_UI_STATE_INIT != master_cblk_ptr->state)
    {
        return STATUS_INVALID_STATE;
    }

    client_ui_init_ncurses(master_cblk_ptr);

    status = generic_create_thread(client_ui_input_thread_entry,
                                   master_cblk_ptr,
                                   &master_cblk_ptr->input_thread);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_input_thread;
    }

    master_cblk_ptr->state = CLIENT_UI_STATE_OPEN;
    status                 = generic_create_thread(client_ui_thread_entry,
                                                   master_cblk_ptr,
                                                   NULL);
    if (STATUS_SUCCESS != status)
    {
        goto fail_create_main_thread;
    }

    return STATUS_SUCCESS;

fail_create_main_thread:
    generic_kill_thread(master_cblk_ptr->input_thread);
    master_cblk_ptr->state = CLIENT_UI_STATE_INIT;

fail_create_input_thread:
    return status;
}


eSTATUS client_ui_close(
    CLIENT_UI client_ui)
{
    eSTATUS            status;
    sCLIENT_UI_CBLK*   master_cblk_ptr;
    sCLIENT_UI_MESSAGE message;

    if (NULL == client_ui)
    {
        return STATUS_INVALID_ARG;
    }
    master_cblk_ptr = (sCLIENT_UI_CBLK*)client_ui;

    message.type = CLIENT_UI_MESSAGE_CLOSE;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    if (STATUS_SUCCESS != status)
    {
        return status;
    }
}


eSTATUS client_ui_destroy(
    CLIENT_UI client_ui)
{
    eSTATUS          status;
    sCLIENT_UI_CBLK* master_cblk_ptr;

    if (NULL == client_ui)
    {
        return STATUS_INVALID_ARG;
    }

    master_cblk_ptr = (sCLIENT_UI_CBLK*)client_ui;
    if (CLIENT_UI_STATE_INIT   != master_cblk_ptr->state &&
        CLIENT_UI_STATE_CLOSED != master_cblk_ptr->state)
    {
        return STATUS_INVALID_STATE;
    }

    status = message_queue_destroy(master_cblk_ptr->message_queue);
    assert(STATUS_SUCCESS == status);
    
    generic_deallocator(master_cblk_ptr);
    return STATUS_SUCCESS;
}


eSTATUS client_ui_post_event(
    CLIENT_UI          client_ui,
    const sCHAT_EVENT* event)
{
    eSTATUS status;

    sCLIENT_UI_CBLK*   master_cblk_ptr;
    sCLIENT_UI_MESSAGE message;

    if (NULL == client_ui)
    {
        return STATUS_INVALID_ARG;
    }
    master_cblk_ptr = (sCLIENT_UI_CBLK*)client_ui;

    message.type = CLIENT_UI_MESSAGE_POST_EVENT;

    status = chat_event_copy(&message.params.post_event.event,
                             event);
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    if (STATUS_SUCCESS != status)
    {
        return status;
    }

    return STATUS_SUCCESS;
}
