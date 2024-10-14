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

#ifndef _KK_NCURSES_WINDOW_HELP_H_
#define _KK_NCURSES_WINDOW_HELP_H_

#include <glib.h>

gboolean ncurses_window_help_init (void);
gboolean ncurses_window_help_resize (gint width, gint height, gint x, gint y);
void ncurses_window_help_delete (void);
void ncurses_window_help_clear (void);
void ncurses_window_help_update (void);

void ncurses_window_help_up (void);
void ncurses_window_help_down (void);
void ncurses_window_help_down_half_page (void);
void ncurses_window_help_up_half_page (void);
void ncurses_window_help_down_full_page (void);
void ncurses_window_help_up_full_page (void);

#endif
