/*
 * Copyright (C) 2024 kilikali-nc team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307,
 * USA.
 */

#include <locale.h>
#include <ncurses.h>
#include <glib.h>
#include "keys.h"
#include "config.h"
#include "util.h"

static void _print_ch_to (int to, int ch, int width)
{
    if (config_option_key_in_reserved_list (ch) == TRUE)
        mvprintw (to, 1, "Reserved. Do not use! str:'%s' int:'%d'%-*s", keyname(ch), ch, width-37-(int)strlen(keyname(ch)), ".");
    else
        mvprintw (to, 1, "str:'%s' int:'%d'%-*s", keyname(ch), ch, width-15-(int)strlen(keyname(ch)), ".");
}

int keys_print_out (void)
{
    int ch = 0;
    int i = 1;
    int height = 0;
    int width = 0;
    char utf8[MAX_UTF8_CHAR_SIZE];
    int pos = 0;

    setlocale (LC_ALL, "");
    setlocale (LC_NUMERIC, "C");

    initscr ();
    raw ();
    keypad (stdscr, TRUE);
    noecho ();
    nodelay (stdscr, TRUE);
    getmaxyx (stdscr, height, width);
    mvprintw (0,0, "Press key or key combination to get key code. Ctrl-c (^C) to quit.");
    wmove (stdscr, i, 1);
    do {
        ch = getch ();
        if (ch >= 0 && ch <= 0xff) { /* FIXME: check is this enough!!! */
            utf8[pos++] = (char)ch;
            if (pos > 6) { /* Should not happen, but */
                pos = 0;
            }
            utf8[pos] = '\0';
            if (g_utf8_validate (utf8, -1, NULL) == TRUE) {
                if (pos > 1) {
                    mvprintw (i++, 1, "str (utf8):'%s'%-*s", utf8, width-12-(int)g_utf8_strlen(utf8, -1), ".");
                } else { // one byte as ch
                    _print_ch_to (i++, ch, width);
                }
                pos = 0;
            } else {
                continue; /* no need for sleep */
            }
        } else if (ch != -1) {
            _print_ch_to (i++, ch, width);
        }
        if (i >= height) i = 1;
        wmove (stdscr, i, 1);
    } while (ch != 3);

    endwin ();
    return 0;
}
