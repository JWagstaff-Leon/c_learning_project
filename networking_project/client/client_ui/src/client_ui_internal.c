#include "client_ui.h"
#include "client_ui_internal.h"

#include <ncurses.h>


void client_ui_reset_input_window(
    sCLIENT_UI_CBLK* master_cblk_ptr)
{
    wclear(master_cblk_ptr->input_window);

    wborder(master_cblk_ptr->input_window,
            ' ',  // Left side
            ' ',  // Right side
            '-',  // Top side
            ' ',  // Bottom side
            '-',  // Top left corner
            '-',  // Top right corner
            ' ',  // Bottom left corner
            ' '); // Bottom right corner

    wmove(master_cblk_ptr->input_window, 1, 0);
    wprintw(master_cblk_ptr->input_window, "%c ", '>');
    wrefresh(master_cblk_ptr->input_window);
}


void client_ui_init_ncurses(
    sCLIENT_UI_CBLK* master_cblk_ptr)
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

    master_cblk_ptr->input_window    = newwin(2, 0, max_y - 2, 0);
    master_cblk_ptr->messages_window = newwin(max_y - 3, 0, 0, 0);

    client_ui_reset_input_window(master_cblk_ptr);

    scrollok(master_cblk_ptr->messages_window, true);
}


void client_ui_close_ncurses(
    sCLIENT_UI_CBLK* master_cblk_ptr)
{
    delwin(master_cblk_ptr->messages_window);
    delwin(master_cblk_ptr->input_window);

    endwin();
}
