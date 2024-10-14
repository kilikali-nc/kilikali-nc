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

#include <libintl.h>
#define _(String) gettext (String)

#include "playlist-line.h"
#include "config.h"
#include "util.h"

static gsize _copy_string (gchar *line_pos, gsize free_space, gchar *str, const gchar *place_holder);

gboolean playlist_line_create (gchar *line, gsize max_line_len, Song *o)
{
    gsize free_space = max_line_len;
    gsize used_space = 0;
    gsize used_space0 = 0;
    gchar *p = line;
    const gchar *template = config.playlist_line;
    const gchar *pt = template;
    gchar *str = NULL;
    gchar timestr[MAX_TIME_STR_LEN] = "";

    if (line == NULL) return FALSE;
    else if (o == NULL) return FALSE;

    if (template == NULL || strlen (template) == 0) {
        if (o->type == SONG_TYPE_STREAM) str = o->stream_title;
        else str = o->title;
        if (str == NULL) str = o->basename;
        used_space = _copy_string (p, free_space, str, _("Unknown title"));
        return TRUE;
    }

    if (o->type == SONG_TYPE_STREAM) {
        used_space = _copy_string (p, free_space, o->stream_title, o->uri);
        if (used_space < 0) return FALSE;
    } else {
        while (*pt != '\0') {
            if (*pt != '%') {
                gchar *next = g_utf8_find_next_char (pt, NULL);
                used_space0 = next - pt;
                if (free_space - used_space0 < 0) return FALSE;
                strncpy (p, pt, used_space0);
                used_space = used_space0;
            } else {
                gchar *next = g_utf8_find_next_char (pt, NULL);
                used_space0 = next - pt;
                if (free_space - used_space0 < 0) return FALSE;
                pt = next;
                if (*pt == 't') {
                    str = o->title;
                    if (str == NULL) str = o->basename;
                    used_space = _copy_string (p, free_space, str, _("Unknown title"));
                } else if (*pt == 'a') {
                    used_space = _copy_string (p, free_space, o->artist, _("Unknown artist"));
                } else if (*pt == 'A') {
                    used_space = _copy_string (p, free_space, o->album, _("Unknown album"));
                } else if (*pt == 'f') {
                    used_space = _copy_string (p, free_space, o->basename, "");
                } else if (*pt == 'l') {
                    util_time_to_string (timestr, MAX_TIME_STR_LEN, o->duration);
                    used_space = _copy_string (p, free_space, timestr, "");
                } else {
                    used_space = 0;
                }
                if (used_space < 0) return FALSE;
            }
            free_space = free_space - used_space;
            p = p + used_space;
            pt = pt + used_space0;
        }
    }
    return TRUE;
}

static gsize _copy_string (gchar *line_pos, gsize free_space, gchar *str, const gchar *place_holder)
{
    size_t len = 0;
    const gchar *s = NULL;
    if (str != NULL) {
        len = strlen (str);
        s = str;
    } else {
        len = strlen (place_holder);
        s = place_holder;
    }
    if (len > free_space) return -1;
    (void)memcpy (line_pos, s, len);
    return len;
}

