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

#include <glib.h>

#include "ncurses-colors.h"

static gboolean _colors = FALSE;

void ncurses_colors_init (void)
{
    _colors = has_colors ()?TRUE:FALSE;
    if (_colors) {
        start_color ();
        init_color (COLOR_WHITE, 1000, 1000, 1000); /* white */
        init_color (COLOR_MAGENTA, 400, 400, 400); /* grey */
        init_color (COLOR_CYAN, 700, 700, 700); /* light grey */

        init_pair (COLOR_PAIR_WHITE_BLACK, COLOR_WHITE, COLOR_BLACK);
        init_pair (COLOR_PAIR_BLACK_WHITE, COLOR_BLACK, COLOR_WHITE);
        init_pair (COLOR_PAIR_LIGHTGREY_BLACK, COLOR_CYAN, COLOR_BLACK);
        init_pair (COLOR_PAIR_BLACK_LIGHTGREY, COLOR_BLACK, COLOR_CYAN);
        init_pair (COLOR_PAIR_GREY_BLACK, COLOR_MAGENTA, COLOR_BLACK);
        init_pair (COLOR_PAIR_BLACK_GREY, COLOR_BLACK, COLOR_MAGENTA);
        init_pair (COLOR_PAIR_BLUE_BLACK, COLOR_BLUE, COLOR_BLACK);
        init_pair (COLOR_PAIR_GREEN_BLACK, COLOR_GREEN, COLOR_BLACK);
        init_pair (COLOR_PAIR_BLACK_YELLOW, COLOR_BLACK, COLOR_YELLOW);
        init_pair (COLOR_PAIR_BLUE_YELLOW, COLOR_BLUE, COLOR_YELLOW);
        init_pair (COLOR_PAIR_RED_BLACK, COLOR_RED, COLOR_BLACK);
        init_pair (COLOR_PAIR_BLACK_RED, COLOR_BLACK, COLOR_RED);
        init_pair (COLOR_PAIR_RED_WHITE, COLOR_RED, COLOR_WHITE);
        init_pair (COLOR_PAIR_WHITE_RED, COLOR_WHITE, COLOR_RED);
    }
}

void ncurses_colors_pair_set (WINDOW *win, NCursesColorPair pair)
{
    if (_colors == FALSE) {
        return;
    }
    wattron (win, COLOR_PAIR(pair));
}
