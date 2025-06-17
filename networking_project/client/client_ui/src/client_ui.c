#include "client_ui.h"
#include "client_ui_internal.h"

#include <ctype.h>
#include <ncurses.h>
#include <stdbool.h>
#include <string.h>

#include "chat_event.h"
#include "common_types.h"


static void init_ncurses(
    WINDOW** out_input_window,
    WINDOW** out_message_window)
{
    int max_x, max_y;

    initscr();
    start_color();
    init_color(8, 650, 650, 650);
    init_pair(1, 8, COLOR_BLACK);
    raw();
    noecho();
    keypad(stdscr, TRUE);
    nl();

    getmaxyx(stdscr, max_y, max_x);

    *out_input_window = newwin(1, 0, max_y - 2, 0);

    *out_message_window = newwin(max_y - 2, 0, 0, 0);
    scrollok(*out_message_window, true);
}


eSTATUS client_ui_create(
    CLIENT_UI*       out_new_client_ui,
    fCLIENT_UI_CBACK user_cback,
    void*            user_arg)
{
    eSTATUS         status;
    sCLIENT_UI_CBLK new_client_ui;

    new_client_ui = generic_allocator(sizeof(sCLIENT_UI_CBLK));
    if (NULL == new_client_ui)
    {
        return STATUS_ALLOC_FAILED;
    }
    memset(new_client_ui, 0, sizeof(sCLIENT_UI_CBLK));

    init_ncurses(&new_client_ui->input_window,
                 &new_client_ui->messages_window);

    status = generic_create_thread(client_ui_input_thread_entry,
                                   new_client_ui,
                                   &new_chat_ui->input_thread);
    if (STATUS_SUCCESS != status)
    {
        generic_deallocator(new_client_ui);
        return status;
    }

    *out_new_client_ui = new_client_ui;
    return STATUS_SUCCESS;
}


eSTATUS client_ui_post_local_event(

)
{

}


eSTATUS client_ui_post_foreign_event(

)
{

}
