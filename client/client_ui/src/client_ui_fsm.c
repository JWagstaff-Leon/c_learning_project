#include "client_ui.h"
#include "client_ui_internal.h"
#include "client_ui_fsm.h"

#include <assert.h>
#include <ncurses.h>
#include <string.h>

#include "common_types.h"
#include "chat_event.h"
#include "message_queue.h"


static void open_processing(
    sCLIENT_UI_CBLK*          master_cblk_ptr,
    const sCLIENT_UI_MESSAGE* message)
{
    eSTATUS               status;
    const sCHAT_EVENT*    incoming_event;
    sCLIENT_UI_CBACK_DATA cback_data;

    int cur_y, cur_x;

    switch (message->type)
    {
        case CLIENT_UI_MESSAGE_POST_EVENT:
        {
            getyx(master_cblk_ptr->input_window, cur_y, cur_x);

            incoming_event = &message->params.post_event.event;
            switch(incoming_event->type)
            {
                case CHAT_EVENT_CHAT_MESSAGE:
                {
                    wprintw(master_cblk_ptr->messages_window,
                            "%s: %s\n",
                            incoming_event->origin.name,
                            incoming_event->data);

                    wrefresh(master_cblk_ptr->messages_window);
                    break;
                }

                case CHAT_EVENT_CONNECTION_FAILED:
                case CHAT_EVENT_USERNAME_REQUEST:
                case CHAT_EVENT_USERNAME_REJECTED:
                case CHAT_EVENT_PASSWORD_REQUEST:
                case CHAT_EVENT_PASSWORD_REJECTED:
                case CHAT_EVENT_AUTHENTICATED:
                case CHAT_EVENT_SERVER_ERROR:
                case CHAT_EVENT_SERVER_SHUTDOWN:
                {
                    wattr_on(master_cblk_ptr->messages_window, A_ITALIC, NULL);
                    wcolor_set(master_cblk_ptr->messages_window, 1, NULL);

                    wprintw(master_cblk_ptr->messages_window,
                            "# %s\n",
                            incoming_event->data);

                    wattr_off(master_cblk_ptr->messages_window, A_ITALIC, NULL);
                    wcolor_set(master_cblk_ptr->messages_window, 0, NULL);

                    wrefresh(master_cblk_ptr->messages_window);
                    break;
                }

                case CHAT_EVENT_USER_JOIN:
                {
                    wattr_on(master_cblk_ptr->messages_window, A_ITALIC, NULL);
                    wcolor_set(master_cblk_ptr->messages_window, 1, NULL);

                    wprintw(master_cblk_ptr->messages_window,
                            "# %s joined\n",
                            incoming_event->data);

                    wattr_off(master_cblk_ptr->messages_window, A_ITALIC, NULL);
                    wcolor_set(master_cblk_ptr->messages_window, 0, NULL);

                    wrefresh(master_cblk_ptr->messages_window);
                    break;
                }

                case CHAT_EVENT_USER_LEAVE:
                {
                    wattr_on(master_cblk_ptr->messages_window, A_ITALIC, NULL);
                    wcolor_set(master_cblk_ptr->messages_window, 1, NULL);

                    wprintw(master_cblk_ptr->messages_window,
                            "# %s left\n",
                            incoming_event->data);

                    wattr_off(master_cblk_ptr->messages_window, A_ITALIC, NULL);
                    wcolor_set(master_cblk_ptr->messages_window, 0, NULL);

                    wrefresh(master_cblk_ptr->messages_window);
                    break;
                }
            }

            wmove(master_cblk_ptr->input_window, cur_y, cur_x);
            wrefresh(master_cblk_ptr->input_window);
            break;
        }
        case CLIENT_UI_MESSAGE_INPUT_CHAR:
        {
            master_cblk_ptr->input_buffer[master_cblk_ptr->input_position++] = message->params.input_char.character;
            wprintw(master_cblk_ptr->input_window, "%c", message->params.input_char.character);
            wrefresh(master_cblk_ptr->input_window);
            break;
        }
        case CLIENT_UI_MESSAGE_INPUT_BACKSPACE:
        {
            master_cblk_ptr->input_buffer[master_cblk_ptr->input_position--] = '\0';
            wprintw(master_cblk_ptr->input_window, "\b \b");
            wrefresh(master_cblk_ptr->input_window);
            break;
        }
        case CLIENT_UI_MESSAGE_INPUT_SEND:
        {
            status = print_string_to_buffer(cback_data.user_input.buffer,
                                            master_cblk_ptr->input_buffer,
                                            sizeof(cback_data.user_input.buffer),
                                            NULL);
            assert(STATUS_SUCCESS == status);

            master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                        CLIENT_UI_EVENT_USER_INPUT,
                                        &cback_data);

            memset(master_cblk_ptr->input_buffer, 0, sizeof(master_cblk_ptr->input_buffer));
            master_cblk_ptr->input_position = 0;

            client_ui_reset_input_window(master_cblk_ptr);
            break;
        }
        case CLIENT_UI_MESSAGE_INPUT_THREAD_CLOSED:
        {
            master_cblk_ptr->state = CLIENT_UI_STATE_CLOSED;
            break;
        }
        case CLIENT_UI_MESSAGE_CLOSE:
        {
            generic_kill_thread(master_cblk_ptr->input_thread);
            master_cblk_ptr->state = CLIENT_UI_STATE_CLOSED;
            break;
        }
    }
}


static void dispatch_message(
    sCLIENT_UI_CBLK*          master_cblk_ptr,
    const sCLIENT_UI_MESSAGE* message)
{
    switch (master_cblk_ptr->state)
    {
        case CLIENT_UI_STATE_OPEN:
        {
            open_processing(master_cblk_ptr, message);
            break;
        }
        case CLIENT_UI_STATE_INIT:
        case CLIENT_UI_STATE_CLOSED:
        default:
        {
            // Should never get here
            assert(0);
            break;
        }
    }
}


void* client_ui_thread_entry(
    void* arg)
{
    eSTATUS            status;
    sCLIENT_UI_CBLK*   master_cblk_ptr;
    sCLIENT_UI_MESSAGE message;

    master_cblk_ptr = (sCLIENT_UI_CBLK*)arg;

    while (CLIENT_UI_STATE_CLOSED != master_cblk_ptr->state)
    {
        status = message_queue_get(master_cblk_ptr->message_queue,
                                   &message,
                                   sizeof(message));
        assert(STATUS_SUCCESS == status);

        dispatch_message(master_cblk_ptr, &message);
    }

    client_ui_close_ncurses(master_cblk_ptr);
    master_cblk_ptr->user_cback(master_cblk_ptr->user_arg,
                                CLIENT_UI_EVENT_CLOSED,
                                NULL);
}
