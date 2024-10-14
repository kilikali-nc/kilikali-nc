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

#ifndef _KK_NCURSES_WINDOW_FILEBROWSER_H_
#define _KK_NCURSES_WINDOW_FILEBROWSER_H_

#include <glib.h>

typedef enum {
    FILEBROWSER_ENTRY_FLAG_IS_DIRECTORY = (1 << 0),
    FILEBROWSER_ENTRY_FLAG_SEARCH_MATCH = (1 << 1) /* Optimization so don't need to check all hilight bits */
} FileBrowserEntryFlag;

typedef enum {
    FILEBROWSER_CMD_MODE_NONE = 0,
    FILEBROWSER_CMD_MODE_CMD,
    FILEBROWSER_CMD_MODE_SEARCH
} FileBrowserCmdMode;

gboolean ncurses_window_filebrowser_init (void);
gboolean ncurses_window_filebrowser_resize (gint width, gint height, gint x, gint y);
void ncurses_window_filebrowser_delete (void);
void ncurses_window_filebrowser_clear (void);
void ncurses_window_filebrowser_update (void);
void ncurses_window_filebrowser_free (void);

void ncurses_window_filebrowser_cmd_mode_set (FileBrowserCmdMode mode);
FileBrowserCmdMode ncurses_window_filebrowser_cmd_mode (void);
void ncurses_window_filebrowser_up (void);
void ncurses_window_filebrowser_down (void);
void ncurses_window_filebrowser_add (void);
void ncurses_window_filebrowser_up_half_page (void);
void ncurses_window_filebrowser_down_half_page (void);
void ncurses_window_filebrowser_up_full_page (void);
void ncurses_window_filebrowser_down_full_page (void);
void ncurses_window_filebrowser_prev_directory (void);
void ncurses_window_filebrowser_top (void);
void ncurses_window_filebrowser_bottom (void);
void ncurses_window_filebrowser_scroll_up (void);
void ncurses_window_filebrowser_scroll_down (void);
void ncurses_window_filebrowser_center_screen_on_cursor (void);
void ncurses_window_filebrowser_search_next (void);
void ncurses_window_filebrowser_search_prev (void);
void ncurses_window_filebrowser_change_directory_to_selected (void);
void ncurses_window_filebrowser_search (const gchar *line);
void ncurses_window_filebrowser_cancel_search (void);
void ncurses_window_filebrowser_complete_search (void);
void ncurses_window_filebrowser_clear_search (void);
int ncurses_window_filebrowser_fill_entries (void);
int ncurses_window_filebrowser_change_directory (const char *directory_path);
int ncurses_window_filebrowser_open (void);
const char *ncurses_window_filebrowser_get_current_directory (void);
void ncurses_window_filebrowser_next_sort_mode (void);
void ncurses_window_filebrowser_refresh (void);
void ncurses_window_filebrowser_toggle_select (void);
void ncurses_window_filebrowser_set_select_off (void);
gboolean ncurses_window_filebrowser_get_select (void);
void ncurses_window_filebrowser_jump (gint index);

#endif
