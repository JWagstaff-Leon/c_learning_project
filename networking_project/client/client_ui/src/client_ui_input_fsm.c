#include "client_ui.h"
#include "client_ui_internal.h"

#include <ncurses.h>
#include <string.h>

#include "message_queue.h"


void* client_ui_input_thread_entry(
    void* arg)
{
    eSTATUS status;

    sCLIENT_UI_CBLK* master_cblk_ptr = (sCLIENT_UI_CBLK*)arg;

    char input_buffer[CHAT_EVENT_MAX_DATA_SIZE] = { 0 };
    int  input_position                         = 0;
    int  input_char                             = 0;

    bool done = false;

    sCLIENT_UI_MESSAGE message;

    status = generic_thread_set_kill_mode(THREAD_KILL_INSTANT);
    if (STATUS_SUCCESS != status)
    {
        message_queue_put(master_cblk_ptr->message_queue,
                          &message,
                          sizeof(message));
        return NULL;
    }

    while (!done)
    {
        input_char = wgetch(master_cblk_ptr->input_window);
        switch (input_char)
        {
            case input_case:
            {
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
                master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                            CLIENT_UI_EVENT_USER_INPUT,
                                            input_buffer);

                memset(input_buffer, 0, sizeof(input_buffer));
                input_position = 0;

                wclear(master_cblk_ptr->input_window);
                wrefresh(master_cblk_ptr->input_window);

                break;
            }
            case backspace_case:
            {
                if (input_position > 0)
                {
                    input_buffer[input_position--] = '\0';
                    wprintf(master_cblk_ptr->input_window, "\b \b");
                    wrefresh(master_cblk_ptr->input_window);
                }
                break;
            }
            case ctrl_c_case:
            {
                done = true;
            }
        }
    }

    message_queue_put(master_cblk_ptr->message_queue,
                      &message,
                      sizeof(message));
    return NULL;
}
