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

#include "playlist-m3u.h"
#include "util.h"
#include "inspector.h"
#include "log.h"

#define MAX_LINE_LEN 255

GList *playlist_m3u_load (const gchar *filename)
{
    GList *l = NULL;
    GList *lt;
    GIOChannel *channel;
    GIOStatus status;
    gchar *line = NULL;
    gsize len;
    gchar *curdir = g_get_current_dir ();
    if (curdir == NULL) return NULL;

    channel = g_io_channel_new_file (filename, "r", NULL);
    if (channel == NULL) {
        LOG_ERROR("g_io_channel_new_file() failed for file %s.", filename);
        goto error;
    }

    if (util_chdir_filepath (filename) == FALSE) goto error;

    while ((status = g_io_channel_read_line (channel, &line, &len, NULL, NULL)) == G_IO_STATUS_NORMAL) {
        gboolean ignore = FALSE;
        gchar *stripped = g_strstrip (line); /* strip does not make copy */
        if (len > 0) {
            if (stripped[0] == '#') ignore = TRUE; /* skip */
        }
        if (ignore == FALSE && len > 0) {
            lt = inspector_add_no_check (stripped);
            l = g_list_concat (l, lt);
        }
        g_free (line);
        line = NULL;
    }
error:
    (void)util_chdir (curdir);
    g_free (curdir);
    g_free (line);
    if (channel) {
        g_io_channel_unref (channel);
    }

    return l;
}
