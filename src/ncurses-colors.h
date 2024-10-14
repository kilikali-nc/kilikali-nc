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

#ifndef _KK_NCURSES_COLORS_
#define _KK_NCURSES_COLORS_

#include <ncurses.h>

typedef enum { /* Foreground, background */
    COLOR_PAIR_WHITE_BLACK = 1, /* selected line */
    COLOR_PAIR_BLACK_WHITE,
    COLOR_PAIR_LIGHTGREY_BLACK, /* basic */
    COLOR_PAIR_BLACK_LIGHTGREY,
    COLOR_PAIR_GREY_BLACK, /* song number */
    COLOR_PAIR_BLACK_GREY,
    COLOR_PAIR_BLUE_BLACK,
    COLOR_PAIR_GREEN_BLACK,
    COLOR_PAIR_BLACK_YELLOW,
    COLOR_PAIR_BLUE_YELLOW,
    COLOR_PAIR_RED_BLACK,
    COLOR_PAIR_BLACK_RED,
    COLOR_PAIR_RED_WHITE,
    COLOR_PAIR_WHITE_RED
} NCursesColorPair;

void ncurses_colors_init (void);
void ncurses_colors_pair_set (WINDOW *win, NCursesColorPair pair);

#endif
