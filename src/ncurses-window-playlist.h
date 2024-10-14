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

#ifndef _KK_NCURSES_WINDOW_PLAYLIST_H_
#define _KK_NCURSES_WINDOW_PLAYLIST_H_

#include <glib.h>
#include "ncurses-common.h"
#include "ncurses-colors.h"

typedef enum {
    NCURSES_WINDOW_PLAYLIST_MODE_NORMAL,
    NCURSES_WINDOW_PLAYLIST_MODE_EDIT
} NCursesWindowPlaylistMode;

/* common for all windows */
gboolean ncurses_window_playlist_init (void);
gboolean ncurses_window_playlist_resize (gint width, gint height, gint x, gint y);
void ncurses_window_playlist_delete (void);
void ncurses_window_playlist_clear (void);
void ncurses_window_playlist_update (void);

/* special for this window */
void ncurses_window_playlist_color_set (NCursesColorPair color);
void ncurses_window_playlist_mode_set (NCursesWindowPlaylistMode mode);
NCursesWindowPlaylistMode ncurses_window_playlist_mode (void);
void ncurses_window_playlist_current_index_set (gint index);
void ncurses_window_playlist_unselect_all (void);
void ncurses_window_playlist_selections_set (gint start, gint end);
gint ncurses_window_playlist_selection_start (void);
gint ncurses_window_playlist_selection_end (void);
gint ncurses_window_playlist_selection_min (void);
gint ncurses_window_playlist_selection_max (void);
void ncurses_window_playlist_ensure_page_start_index (void);
void ncurses_window_playlist_up (void);
void ncurses_window_playlist_down (void);
void ncurses_window_playlist_select_multiple_up (void);
void ncurses_window_playlist_select_multiple_down (void);
void ncurses_window_playlist_select_all (void);
void ncurses_window_playlist_down_half_page (void);
void ncurses_window_playlist_up_half_page (void);
void ncurses_window_playlist_down_full_page (void);
void ncurses_window_playlist_up_full_page (void);
void ncurses_window_playlist_top (void);
void ncurses_window_playlist_bottom (void);
void ncurses_window_playlist_scroll_down (void);
void ncurses_window_playlist_scroll_up (void);
void ncurses_window_playlist_scroll_center_to_cursor (void);
void ncurses_window_playlist_toggle_select_set (gboolean value);
gboolean ncurses_window_playlist_toggle_select (void);
void ncurses_window_playlist_jump (gint index);
void ncurses_window_playlist_search_next (void);
void ncurses_window_playlist_search_prev (void);
void ncurses_window_playlist_clear_search (void);
void ncurses_window_playlist_show_search_hilight (void);

#endif
