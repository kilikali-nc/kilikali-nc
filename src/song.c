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

#include <stdio.h>

#include "song.h"

Song *song_new (const char *uri)
{
    gchar *n0, *n1;
    Song *s = g_malloc0 (sizeof (Song));
    if (s == NULL) return NULL;
    s->uri = g_strdup (uri);
    if (s->uri == NULL) goto error;
    n0 = g_filename_from_uri (s->uri, NULL, NULL);
    if (n0 != NULL) {
        n1 = g_filename_display_basename (n0); 
        if (n1 != NULL) {
           s->basename = n1;
        }
        g_free (n0);
    }
    s->selected = FALSE;
    s->search_hit = -1;
    return s;
error:
    song_delete (s);
    return NULL;
}

Song *song_clone (Song *s)
{
    if (s == NULL) return NULL;
    Song *c = song_new (s->uri);
    if (c == NULL) return NULL;

    (void)song_set_artist (c, s->artist);
    (void)song_set_album(c, s->album);
    (void)song_set_title (c, s->title);
    (void)song_set_stream_title (c, s->stream_title);
    (void)song_set_codec (c, s->codec);
    (void)song_set_copyright (c, s->copyright);
    (void)song_set_type (c, s->type);
    (void)song_set_year (c, s->year);
    (void)song_set_track (c, s->track);
    (void)song_set_duration (c, s->duration);

    c->selected = s->selected;
    c->search_hit = s->search_hit;
    return c;
}

void song_delete (Song *s)
{
    if (s == NULL) return;

    g_free (s->artist);
    g_free (s->album);
    g_free (s->title);
    g_free (s->stream_title);
    g_free (s->uri);
    g_free (s->basename);
    g_free (s->codec);
    g_free (s->copyright);

    g_free(s);
}

int song_set_artist (Song *s, const gchar *artist)
{
    if (s == NULL) return 1;
    if (artist == NULL) return 2;
    g_free (s->artist);
    s->artist = g_strdup (artist);
    if (s->artist == NULL) return 3;
    return 0;
}

int song_set_album (Song *s, const gchar *album)
{
    if (s == NULL) return 1;
    if (album == NULL) return 2;
    g_free (s->album);
    s->album = g_strdup (album);
    if (s->album == NULL) return 3;
    return 0;
}

int song_set_title (Song *s, const gchar *title)
{
    if (s == NULL) return 1;
    if (title == NULL) return 2;
    g_free (s->title);
    s->title = g_strdup (title);
    if (s->title == NULL) return 3;
    return 0;
}

int song_set_stream_title (Song *s, const gchar *title)
{
    if (s == NULL) return 1;
    if (title == NULL) return 2;
    g_free (s->stream_title);
    s->stream_title = g_strdup (title);
    if (s->stream_title == NULL) return 3;
    return 0;
}

int song_set_type (Song *s, SongType type)
{
    if (s == NULL) return 1;
    s->type = type;
    return 0;
}

int song_set_year (Song *s, guint year)
{
    if (s == NULL) return 1;
    s->year = year;
    return 0;
}

int song_set_track (Song *s, guint track)
{
    if (s == NULL) return 1;
    s->track = track;
    return 0;
}

int song_set_duration (Song *s, gint64 duration)
{
    if (s == NULL) return 1;
    s->duration = duration;
    return 0;
}

int song_set_codec (Song *s, const gchar *codec)
{
    if (s == NULL) return 1;
    if (codec == NULL) return 2;
    g_free (s->codec);
    s->codec = g_strdup (codec);
    if (s->codec == NULL) return 3;
    return 0;
}

int song_set_copyright (Song *s, const gchar *copyright)
{
    if (s == NULL) return 1;
    if (copyright == NULL) return 2;
    g_free (s->copyright);
    s->copyright = g_strdup (copyright);
    if (s->copyright == NULL) return 3;
    return 0;
}


int song_tags_copy (Song *target, Song *source)
{
    if (target == NULL) return 1;
    else if (source == NULL) return 2;

    (void)song_set_artist (target, source->artist);
    /* (void)song_set_type (target, source->type); not really a tag */
    (void)song_set_album (target, source->album);
    (void)song_set_title (target, source->title);
    (void)song_set_stream_title (target, source->stream_title);
    (void)song_set_year (target, source->year);
    (void)song_set_track (target, source->track);
    if (source->duration > 0) (void)song_set_duration (target, source->duration);
    (void)song_set_codec (target, source->codec);
    (void)song_set_copyright (target, source->copyright);

    return 0;
}

