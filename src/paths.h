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

#ifndef _KK_PATHS_
#define _KK_PATHS_

#include <glib.h>

/* assumes home dir is already there */
gchar *paths_home_dir (void);
/* creates path(s) if not already there */
gchar *paths_config_dir (void);

/* By default ~/.config/kilikali-nc */
gchar *paths_config_data_dir (void);
gchar *paths_config_data_default_config (void);

/* By default ~/.local/shared/kilikali-nc */
gchar *paths_saved_data_dir (void); /* Creates path if not there */
gchar *paths_saved_data_default_playlist (void);
gchar *paths_saved_data_default_log (void);
gchar *paths_saved_data_stderr_log (void);

#endif
