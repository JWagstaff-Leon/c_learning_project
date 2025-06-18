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
    const sCHAT_EVENT* incoming_event;

    switch (message->type)
    {
        case CLIENT_UI_MESSAGE_TYPE_POST_EVENT:
        {
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

                case CHAT_EVENT_USERNAME_REQUEST:
                case CHAT_EVENT_USERNAME_REJECTED:
                case CHAT_EVENT_PASSWORD_REQUEST:
                case CHAT_EVENT_PASSWORD_REJECTED:
                case CHAT_EVENT_AUTHENTICATED:
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
            break;
        }
        case CLIENT_UI_MESSAGE_TYPE_INPUT_THREAD_CLOSED:
        {
            master_cblk_ptr->state = CLIENT_UI_STATE_CLOSED;
            break;
        }
        case CLIENT_UI_MESSAGE_TYPE_CLOSE:
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
}
