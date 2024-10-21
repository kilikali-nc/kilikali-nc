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

#ifndef _KK_SONG_H_
#define _KK_SONG_H_

#include <glib.h>

#define SONG_MAX_TUNES 101 /* gst siddec plugin has */

typedef enum
{
    SONG_TYPE_UNKNOWN,
    SONG_TYPE_FILE,
    SONG_TYPE_STREAM,
    SONG_TYPE_SID,
    SONG_TYPE_MOD
} SongType;

typedef struct
{
    gchar *uri;
    gchar *basename;
    SongType type;
    gchar *artist;
    gchar *album;
    gchar *title;
    gchar *stream_title;
    gchar *codec;
    gchar *copyright;
    guint year;
    guint track;
    gint64 duration; /* ms */
    gboolean selected; /* to selection mode */
    gint search_hit; /* 0 or higher == search hit */
    /* multitune for sids */
    gint tunes;
    gint64 tune_duration[SONG_MAX_TUNES];
} Song;

Song *song_new (const gchar *uri);
Song *song_clone (Song *s);
void song_delete (Song *s);


int song_set_artist (Song *s, const gchar *artist);
int song_set_type (Song *s, SongType type);
int song_set_album (Song *s, const gchar *album);
int song_set_title (Song *s, const gchar *title);
int song_set_stream_title (Song *s, const gchar *title);
int song_set_year (Song *s, guint year);
int song_set_track (Song *s, guint track);
int song_set_duration (Song *s, gint64 duration);
int song_set_codec (Song *s, const gchar *codec);
int song_set_copyright (Song *s, const gchar *copyright);

int song_tags_copy (Song *target, Song *source); 
#endif
