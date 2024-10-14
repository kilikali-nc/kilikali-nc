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
#include "ncurses-window-help.h"
#include "ncurses-subwindow-textview.h"

#include "help.h"

/* ncurses */
static gint _width = 0;
static gint _height = 0;

static WINDOW *_win = NULL;

static NCursesSubwindowTextview _textview;

gboolean ncurses_window_help_init (void)
{
    ncurses_subwindow_textview_init (&_textview);
    ncurses_subwindow_textview_text (&_textview, help_str, FALSE, FALSE);
    return TRUE;
}

gboolean ncurses_window_help_resize (gint width, gint height, gint x, gint y)
{
    _width = width;
    _height = 2;

    ncurses_subwindow_textview_resize (&_textview, width, height - 2, x, y + 2);
    ncurses_subwindow_textview_setup (&_textview);

    if (_win != NULL) ncurses_window_help_delete ();
    _win = newwin (_height, _width, y, x);
    if (_win == NULL) goto resize_error;

    ncurses_colors_pair_set (_win, COLOR_PAIR_BLUE_BLACK);

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_help_delete (void)
{
    ncurses_subwindow_textview_delete (&_textview);
    if (_win != NULL) delwin (_win);
    _win = NULL;
}

void ncurses_window_help_clear (void)
{
    wclear (_win);
    ncurses_subwindow_textview_clear (&_textview);
}

void ncurses_window_help_update (void)
{
    mvwprintw (_win, 0, 2, "Help");
    ncurses_subwindow_textview_update (&_textview);
    wrefresh (_win);
}

void ncurses_window_help_down (void)
{
    ncurses_subwindow_textview_down (&_textview);
}

void ncurses_window_help_up (void)
{
    ncurses_subwindow_textview_up (&_textview);
}

void ncurses_window_help_down_half_page (void)
{
    ncurses_subwindow_textview_down_half_page (&_textview);
}

void ncurses_window_help_up_half_page (void)
{
    ncurses_subwindow_textview_up_half_page (&_textview);
}

void ncurses_window_help_down_full_page (void)
{
    ncurses_subwindow_textview_down_full_page (&_textview);
}

void ncurses_window_help_up_full_page (void)
{
    ncurses_subwindow_textview_up_full_page (&_textview);
}

