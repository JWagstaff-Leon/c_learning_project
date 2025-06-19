#include "client_ui.h"
#include "client_ui_internal.h"
#include "client_ui_fsm.h"

#include <assert.h>
#include <ncurses.h>
#include <string.h>

#include "chat_event.h"
#include "message_queue.h"


#include <stdio.h>
void* client_ui_input_thread_entry(
    void* arg)
{
    eSTATUS status;

    sCLIENT_UI_CBLK* master_cblk_ptr = (sCLIENT_UI_CBLK*)arg;

    char input_buffer[CHAT_EVENT_MAX_DATA_SIZE] = { 0 };
    int  input_position                         = 0;
    int  input_char                             = 0;

    bool done = false;

    sCLIENT_UI_MESSAGE    message;
    sCLIENT_UI_CBACK_DATA cback_data;

    status = generic_thread_set_kill_mode(THREAD_KILL_INSTANT);
    if (STATUS_SUCCESS != status)
    {
        message.type = CLIENT_UI_MESSAGE_TYPE_INPUT_THREAD_CLOSED;

        status = message_queue_put(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);
        return NULL;
    }

    int proc = 0;
    while (!done)
    {
        input_char = wgetch(master_cblk_ptr->input_window);
        fprintf(stderr, "got char: %d\n", input_char);
        switch (input_char)
        {
            case input_case:
            {
                fprintf(stderr, "%6d - input case.\n", proc++);
                if (input_position < (sizeof(input_buffer) - 1))
                {
                    input_buffer[input_position++] == input_char;
                    wprintw(master_cblk_ptr->input_window, "%c", input_char);
                    wrefresh(master_cblk_ptr->input_window);
                }
                break;
            }
            case '\n':
            {
                if (input_position > 0)
                {
                    fprintf(stderr, "%6d - newline case.\n", proc++);
                    print_string_to_buffer(cback_data.user_input.buffer,
                                        input_buffer,
                                        sizeof(cback_data.user_input.buffer),
                                        NULL);
                    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                                CLIENT_UI_EVENT_USER_INPUT,
                                                &cback_data);

                    memset(input_buffer, 0, sizeof(input_buffer));
                    input_position = 0;

                    wclear(master_cblk_ptr->input_window);
                    wrefresh(master_cblk_ptr->input_window);
                }
                break;
            }
            case backspace_case:
            {
                fprintf(stderr, "%6d - backspace case.\n", proc++);
                if (input_position > 0)
                {
                    input_buffer[input_position--] = '\0';
                    wprintw(master_cblk_ptr->input_window, "\b \b");
                    wrefresh(master_cblk_ptr->input_window);
                }
                break;
            }
            case ctrl_c_case:
            {
                fprintf(stderr, "%6d - ctrl c case.\n", proc++);
                done = true;
                break;
            }
        }
    }

    message.type = CLIENT_UI_MESSAGE_TYPE_INPUT_THREAD_CLOSED;

    status = message_queue_put(master_cblk_ptr->message_queue,
                               &message,
                               sizeof(message));
    assert(STATUS_SUCCESS == status);
    return NULL;
}
