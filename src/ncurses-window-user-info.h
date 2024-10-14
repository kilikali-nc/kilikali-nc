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

#ifndef _KK_NCURSES_WINDOW_USER_INFO_H_
#define _KK_NCURSES_WINDOW_USER_INFO_H_

#include <glib.h>

gboolean ncurses_window_user_info_init (void);
gboolean ncurses_window_user_info_resize (gint width, gint height, gint x, gint y);
void ncurses_window_user_info_delete (void);
void ncurses_window_user_info_clear (void);
void ncurses_window_user_info_update (const gchar *info);

void ncurses_window_user_info_set_cmdline_options (GSList *options);

void ncurses_window_user_info_set_cmline_options_index (gint index);

#endif
