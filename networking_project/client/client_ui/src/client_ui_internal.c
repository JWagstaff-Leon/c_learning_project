#include <ncurses.h>


void client_ui_init_ncurses(
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

    *out_input_window   = newwin(1, 0, max_y - 2, 0);
    *out_message_window = newwin(max_y - 2, 0, 0, 0);

    scrollok(*out_message_window, true);
}


void client_ui_close_ncurses(
    void)
{
    endwin();
}
