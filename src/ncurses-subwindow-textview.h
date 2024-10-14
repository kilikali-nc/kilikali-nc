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

#ifndef _KK_NCURSES_SUBWINDOW_TEXTVIEW_H_
#define _KK_NCURSES_SUBWINDOW_TEXTVIEW_H_

#include <glib.h>
#include <ncurses.h>
#include "ncurses-scroller.h"

/*
 * Text can be static, allocated outside or Textview can allocate it if needed.
 */
typedef struct ncurses_subwindow_textview {
    gint width;
    gint height;
    WINDOW *win;
    NCursesScroller scroller;

    gchar *text;          /* text */
    gboolean free_text;   /* free text when needed */
    gint total_lines;     /* total lines in help_str fitted to window width */
    gint current_line;    /* current line in help_str fitted to window width */
    gint last_line;       /* total lines - height. help_str fitted to window width */
    const gchar *current; /* current position in help_str*/
    const gchar *running; /* used as tmp to get actual position */
} NCursesSubwindowTextview;

gboolean ncurses_subwindow_textview_init (NCursesSubwindowTextview *t);
gboolean ncurses_subwindow_textview_resize (NCursesSubwindowTextview *t, gint width, gint height, gint x, gint y);
void ncurses_subwindow_textview_delete (NCursesSubwindowTextview *t);
void ncurses_subwindow_textview_clear (NCursesSubwindowTextview *t);
void ncurses_subwindow_textview_update (NCursesSubwindowTextview *t);
void ncurses_subwindow_textview_setup (NCursesSubwindowTextview *t);

void ncurses_subwindow_textview_up (NCursesSubwindowTextview *t);
void ncurses_subwindow_textview_down (NCursesSubwindowTextview *t);
void ncurses_subwindow_textview_down_half_page (NCursesSubwindowTextview *t);
void ncurses_subwindow_textview_up_half_page (NCursesSubwindowTextview *t);
void ncurses_subwindow_textview_down_full_page (NCursesSubwindowTextview *t);
void ncurses_subwindow_textview_up_full_page (NCursesSubwindowTextview *t);

gboolean ncurses_subwindow_textview_text (NCursesSubwindowTextview *t, const gchar *text, gboolean alloc_text, gboolean free_text);

#endif
