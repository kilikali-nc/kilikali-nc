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

#include <ncurses.h>
#include <ctype.h>

#include "ncurses-common.h"
#include "ncurses-colors.h"
#include "ncurses-window-title.h"
#include "playlist.h"
#include "player.h"
#include "util.h"

/* ncurses */
static gint _width = 0;
static gint _height = 0;

static WINDOW *_win = NULL;

gboolean ncurses_window_title_init (void)
{
    return TRUE;
}

gboolean ncurses_window_title_resize (gint width, gint height, gint x, gint y)
{
    _width = width;
    _height = height;

    if (_win != NULL) ncurses_window_title_delete ();
    _win = newwin (_height, _width, y, x);
    if (_win == NULL) goto resize_error;
    ncurses_colors_pair_set (_win, COLOR_PAIR_WHITE_BLACK);

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_title_delete (void)
{
    if (_win != NULL) delwin (_win);
    _win = NULL;
}

void ncurses_window_title_clear (void)
{
    wclear (_win);
}

void ncurses_window_title_update (void)
{
    const char *title = "KILIKALI-NC";
    mvwprintw (_win, 0, _width/2-strlen(title)/2, "%s", title);
    wrefresh (_win);
}
