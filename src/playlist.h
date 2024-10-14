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

#ifndef _KK_PLAYLIST_
#define _KK_PLAYLIST_

#include <glib.h>
#include "song.h"
#include "search.h"

typedef enum {
   PLAYLIST_ORDER_PATH = 0,
   PLAYLIST_ORDER_ARTIST
   /* FIXME: Add more */
} PlaylistOrder;

typedef enum {
   PLAYLIST_MODE_STANDARD = 0,
   PLAYLIST_MODE_SUFFLE,
   PLAYLIST_MODE_RANDOM
} PlaylistMode;

void playlist_init (void);

GList *playlist_get (void);
gint playlist_length (void);
void playlist_free (void);

gboolean playlist_add (gchar *path);

/* playlist mode */
gboolean playlist_mode_set (PlaylistMode mode);
gboolean playlist_mode_next (void);
PlaylistMode playlist_mode (void);
const gchar *playlist_mode_to_string (void);

/* loop */
void playlist_loop_set (gboolean loop);
gboolean playlist_loop (void);
gboolean playlist_loop_toggle (void);

/* Possibly changes current song */
Song *playlist_remove_list (GSList *remove_list);

/* Changes current song */
Song *playlist_get_first_song (void);
Song *playlist_get_next_song (void);
Song *playlist_get_nth_song (gint index);
/* Does not change current song */
Song *playlist_get_current_song (void);
Song *playlist_get_nth_song_no_set (gint index);

gint playlist_get_song_index (Song *o);
gint playlist_reorder (PlaylistOrder o);

gint playlist_search_set (const gchar *search, gint start_index, gboolean backwards, gboolean with_tags);
gint playlist_search_next (void);
gint playlist_search_prev (void);
void playlist_search_index_set (gint index);
const gchar *playlist_search_string (void);
gboolean playlist_search_with_tags (void);
gboolean playlist_search_use_case_sensitive (void);
void playlist_search_free (void);
gboolean playlist_search_get_search_match (Song *o, SearchMatchType *sm);

gboolean playlist_toggle_select_range (gint start, gint end);

gboolean playlist_copy_range (gint first_index, gint second_index);
gboolean playlist_cut_range (gint first_index, gint second_index);
gboolean playlist_copy_selected ();
gboolean playlist_cut_selected ();
gboolean playlist_paste_to (gint index);
gint playlist_num_to_paste (void);

/* Global GList reorder */
gint song_sort_by_path (gconstpointer p1, gconstpointer p2);

#endif
