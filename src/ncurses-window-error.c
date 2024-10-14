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
#include <glib/gprintf.h>

#include "ncurses-common.h"
#include "ncurses-colors.h"
#include "ncurses-window-error.h"

/* ncurses */
static gint _width = 0;
static gint _height = 0;
static gint _x = 0;
static gint _y = 0;

static WINDOW *_win = NULL;
static gchar _msg[ABSOLUTELY_MAX_LINE_LEN] = "";

gboolean ncurses_window_error_init (void)
{
    return TRUE;
}

gboolean ncurses_window_error_resize (gint width, gint height, gint x, gint y)
{
    _width = width;
    _height = height;
    _x = x;
    _y = y;

    if (_win != NULL) ncurses_window_error_delete ();
    _win = newwin (_height, _width, _y, _x);
    if (_win == NULL) goto resize_error;

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_error_delete (void)
{
    if (_win != NULL) delwin (_win);
    _win = NULL;
}

void ncurses_window_error_clear (void)
{
    wclear (_win);
}

void ncurses_window_error_update (void)
{
    /* show only if something to show */
    ncurses_window_error_delete ();
    gint width = (gint)g_utf8_strlen (_msg, -1);
    if (width > 0 && TRUE == ncurses_window_error_resize (width, _height, _x, _y)) {
        ncurses_colors_pair_set (_win, COLOR_PAIR_WHITE_RED);
        mvwprintw (_win, 0, 0, "%-*s", _width, _msg);
        wrefresh (_win);
    }
}

void ncurses_window_error_set (const gchar *str, ...)
{
    if (str != NULL) {
        va_list args;
        va_start(args, str);
        (void)g_vsnprintf (_msg, sizeof(_msg), str, args);
        va_end(args);
    }
}

