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

#include <gst/gst.h>

#include "common.h"

void gst_common_parse_tags (GstMessage *msg, Song *o)
{
    GstTagList *tags = NULL;
    gchar *val = NULL;
    guint uival = 0;
    GDate *date = NULL;

    if (o == NULL || msg == NULL) return;

    gst_message_parse_tag (msg, &tags);
    if (gst_tag_list_get_string_index (tags, GST_TAG_ARTIST, 0, &val) == TRUE) {
        (void)song_set_artist (o, val);
        g_free (val);
    }
    if (gst_tag_list_get_string_index (tags, GST_TAG_TITLE, 0, &val) == TRUE) {
        (void)song_set_title (o, val);
        g_free (val);
    }
    if (gst_tag_list_get_string_index (tags, GST_TAG_ALBUM, 0, &val) == TRUE) {
        (void)song_set_album (o, val);
        g_free (val);
    }
    if (gst_tag_list_get_uint_index (tags, GST_TAG_TRACK_NUMBER, 0, &uival) == TRUE) {
        (void)song_set_track (o, uival);
    }
    if (gst_tag_list_get_string_index (tags, GST_TAG_AUDIO_CODEC, 0, &val) == TRUE) {
        (void)song_set_codec (o, val);
        g_free (val);
    }
    if (gst_tag_list_get_date_index (tags, GST_TAG_DATE, 0, &date) == TRUE) {
        (void)song_set_year (o, date->year);
        g_date_free (date);
    }
    if (gst_tag_list_get_string_index (tags, GST_TAG_COPYRIGHT, 0, &val) == TRUE) {
        (void)song_set_copyright (o, val);
        g_free (val);
    }
    gst_tag_list_unref (tags);
}
