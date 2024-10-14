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

#ifndef _KK_NCURSES_SCREEN_H_
#define _KK_NCURSES_SCREEN_H_

#include <glib.h>
#include "ncurses-event.h"

gboolean ncurses_screen_init (void);
void ncurses_screen_free (void);

void ncurses_screen_event (NCursesEvent *e);
void ncurses_screen_update_force (void);
void ncurses_screen_set_user_info (const gchar *userinfo);
void ncurses_screen_format_user_info (const gchar *fmt, ...);

#endif
