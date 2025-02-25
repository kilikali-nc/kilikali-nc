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

#ifndef _KK_NET_
#define _KK_NET_

#include <glib.h>

typedef enum {
    NET_LYRICS_SERVICE_NONE = 0,
    NET_LYRICS_SERVICE_FIRST = 1,
    NET_LYRICS_SERVICE_CHARTLYRICS = 1,
    NET_LYRICS_SERVICE_END
} NetLyricsService;

void net_free (void);

gchar *net_get (const gchar *url);

gint net_get_lyrics (const gchar *artist, const gchar *title, NetLyricsService service, GSourceFunc cb);
void net_join_lyrics_thread (void);

#endif
