#include "client_ui.h"
#include "client_ui_internal.h"
#include "client_ui_fsm.h"

#include <assert.h>
#include <ncurses.h>
#include <string.h>

#include "chat_event.h"
#include "message_queue.h"


void* client_ui_input_thread_entry(
    void* arg)
{
    eSTATUS status;

    sCLIENT_UI_CBLK* master_cblk_ptr = (sCLIENT_UI_CBLK*)arg;

    int input_char = 0;
    bool done = false;

    sCLIENT_UI_MESSAGE message;

    status = generic_thread_set_kill_mode(THREAD_KILL_INSTANT);
    if (STATUS_SUCCESS != status)
    {
        message.type = CLIENT_UI_MESSAGE_INPUT_THREAD_CLOSED;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
        return NULL;
    }

    while (!done)
    {
        input_char = wgetch(master_cblk_ptr->input_window);
        switch (input_char)
        {
            case input_case:
            {
                if (master_cblk_ptr->input_position < (sizeof(master_cblk_ptr->input_buffer) - 1))
                {
                    message.type = CLIENT_UI_MESSAGE_INPUT_CHAR;

                    message.params.input_char.character = input_char;

                    status = message_queue_put(master_cblk_ptr->message_queue,
                                               &message,
                                               sizeof(message));
                    assert(STATUS_SUCCESS == status);
                }
                break;
            }
            case '\n':
            {
                if (master_cblk_ptr->input_position > 0)
                {
                    message.type = CLIENT_UI_MESSAGE_INPUT_SEND;

                    status = message_queue_put(master_cblk_ptr->message_queue,
                                               &message,
                                               sizeof(message));
                    assert(STATUS_SUCCESS == status);
                }
                break;
            }
            case backspace_case:
            {
                if (master_cblk_ptr->input_position > 0)
                {
                    message.type = CLIENT_UI_MESSAGE_INPUT_BACKSPACE;

                    status = message_queue_put(master_cblk_ptr->message_queue,
                                               &message,
                                               sizeof(message));
                    assert(STATUS_SUCCESS == status);
                }
                break;
            }
            case ctrl_c_case:
            {
                done = true;
                break;
            }
        }
    }

    message.type = CLIENT_UI_MESSAGE_INPUT_THREAD_CLOSED;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    assert(STATUS_SUCCESS == status);
    return NULL;
}
