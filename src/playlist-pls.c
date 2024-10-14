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

#include "playlist-pls.h"
#include "util.h"
#include "song.h"
#include "inspector.h"

#define MAX_KEY_SIZE 255

static GList *_load_common (GKeyFile *kfile);

GList *playlist_pls_load (const gchar *filename)
{
    gint ret = 0;
    GList *l = NULL;
    GKeyFile *kfile = NULL;
    gboolean success;
    gchar *curdir = g_get_current_dir ();
    gchar *str = NULL;
    if (curdir == NULL) return NULL;

    kfile = g_key_file_new ();
    if (kfile == NULL) goto error;

    ret = util_file_load_to_str (filename, &str);
    if (ret != 0) goto error;
 
    success = g_key_file_load_from_data (kfile, str, -1, G_KEY_FILE_NONE, NULL);
    if (success == FALSE) goto error;

    if (util_chdir_filepath (filename) == FALSE) goto error;

    l = _load_common (kfile);
error:
    g_free (str);
    util_chdir (curdir);
    g_free (curdir);
    g_key_file_free (kfile);
    return l;
}

gboolean playlist_pls_save (GList *playlist, const gchar *filename)
{
    gboolean ret = TRUE;
    GKeyFile *kfile = NULL;
    GList *p = playlist;
    gchar key[MAX_KEY_SIZE];
    gchar *str;
    gsize file_num = 1;
    if (playlist == NULL || filename == NULL) return FALSE;

    kfile = g_key_file_new ();
    if (kfile == NULL) return FALSE;

    while (p != NULL) {
        Song *o = p->data;
        if (o == NULL) break;

        if (o->type == SONG_TYPE_STREAM) str = g_strdup (o->uri);
        else str = g_filename_from_uri (o->uri, NULL, NULL);
        if (str == NULL) break;

        g_snprintf (key, 254, "File%zu", file_num);
        g_key_file_set_string (kfile, "playlist", key, str);
        g_free (str);
        if (o->type == SONG_TYPE_STREAM && o->stream_title != NULL) {
            g_snprintf (key, 254, "Title%zu", file_num);
            g_key_file_set_string (kfile, "playlist", key, o->stream_title);
        }
        p = p->next;
        file_num++;
    }

    if (file_num > 1) {
        gsize len = 0;
        gchar *content = g_key_file_to_data (kfile, &len, NULL);
        if (len > 0) {
            gint retval = util_file_write_data (filename, content, len);
            if (retval != 0) ret = FALSE;
        }
    }
    g_key_file_free (kfile);
    return ret;
}

GList *playlist_pls_parse_raw (const gchar *content)
{
    GList *l = NULL;
    GList *p = NULL;
    GKeyFile *kfile = NULL;
    gboolean success;

    kfile = g_key_file_new ();
    if (kfile == NULL) goto error;

    success = g_key_file_load_from_data (kfile, content, strlen (content), G_KEY_FILE_NONE, NULL);
    if (success == FALSE) goto error;

    p = l = _load_common (kfile);
    if (p != NULL) {
        while (p != NULL) {
            Song *o = (Song *)p->data;
            if (inspector_try_uri (o->uri, o) == FALSE) {
                /* remove unsupported */
                GList *next = p->next;
                song_delete (o);
                l = g_list_remove (l, p);
                p = next;
            } else {
                p = p->next;
            }
        }
    }
error:
    g_key_file_free (kfile);
    return l;
}

static GList *_load_common (GKeyFile *kfile)
{
    GList *l = NULL;
    GList *lt;
    gsize len = 0;
    gsize i;
    gchar key[MAX_KEY_SIZE];
    gchar *val = NULL;
    Song *o;
    gchar **keys = NULL;

    keys = g_key_file_get_keys (kfile, "playlist", &len, NULL);
    if (len < 1) goto common_error;
    for (i = 0; i < len; i++) {
        g_snprintf (key, MAX_KEY_SIZE-1, "File%zu", i + 1); /* C99 */
        val = g_key_file_get_string (kfile, "playlist", key, NULL);
        if (val == NULL) break;
        lt = inspector_add_no_check (val);
        g_free (val);
        if (lt != NULL) {
            o = (Song *)lt->data;
            if (o->type == SONG_TYPE_STREAM) {
                g_snprintf (key, MAX_KEY_SIZE-1, "Title%zu", i + 1);
                val = g_key_file_get_string (kfile, "playlist", key, NULL);
                if (val != NULL) song_set_stream_title (o, val);
                g_free (val);
            }
            l = g_list_concat (l, lt);
        }
    }
    g_strfreev (keys);
common_error:
    return l;
}

