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

#ifndef _KK_GST_INSPECTOR_
#define _KK_GST_INSPECTOR_

#include "song.h"

typedef void (*ScreenStatusUpdateFunc)(void);

gboolean inspector_init (ScreenStatusUpdateFunc status_update);
void inspector_free (void);

const gchar *inspector_status (void);

GList *inspector_run (gchar *path);
GList *inspector_add_no_check (gchar *uri);

/* Actual test part. Used also in raw/net downloaded playlist */
gboolean inspector_try_uri (gchar *uri, Song *s);

#endif
