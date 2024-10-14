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
#include <unistd.h>
#include <fcntl.h>

#include "sid.h"
#include "config.h"
#include "util.h"

static gint64 _str_to_duration (gchar *str);

static gint64 _default_duration = 180 * 1000;
static FILE *_file = NULL;

static GByteArray *_kernal = NULL;
static GByteArray *_basic = NULL;
static GByteArray *_chargen = NULL;

void sid_init (void)
{
    _default_duration = config.sid_default_songlength * 1000;
    gchar tmp[PATH_MAX] = "";
    if (FALSE == util_expand_tilde (config.sid_songlengths_file, tmp)) return;
    if (strlen (tmp) > 0) _file = fopen (tmp, "r");
}

void sid_free (void)
{
    if (_file != NULL) fclose (_file);
    _file = NULL;
}

void sid_setup_song (Song *s, guint tunes)
{
    gboolean found = FALSE;
    GChecksum *md5c = NULL;
    gchar *filename = NULL;
    int fd = -1;
    size_t len = 0;
    char *line = NULL;
    gchar **s0 = NULL;
    gchar **s1 = NULL;
    ssize_t size;
    int ret;

    /* set initial values */
    s->tunes = tunes; /* 0 == special case, plays only default one */
    s->duration = _default_duration;
    s->tune_duration[0] = _default_duration;

    for (gint i = 0; i < tunes; i++) {
        s->tune_duration[i] = _default_duration;
    }

    if (_file == NULL) return; /* No songlength.md5 file */

    /* MD5 */
    md5c = g_checksum_new (G_CHECKSUM_MD5);
    if (md5c == NULL) return;

    filename = g_filename_from_uri (s->uri, NULL, NULL);
    if (filename == NULL) goto error_setup_song;

    fd = open (filename, O_RDONLY);
    if (fd < 0) goto error_setup_song;

#define BUF_SIZE 1024
    guchar buf[BUF_SIZE];
    while ((size = read (fd, buf, BUF_SIZE)) > 0) {
        g_checksum_update (md5c, buf, size);
    }

    const gchar *md5 = g_checksum_get_string (md5c); /* no need to free */

    /* Check if in 'db' */
    ret = fseek (_file, 0, SEEK_SET);
    if (ret != 0) goto error_setup_song;

    while ((size = getline (&line, &len, _file)) != -1) {
        if (line[0] == ';' || line[0] == '[') continue;

        if (g_str_has_prefix (line, md5) == TRUE) {
            found = TRUE;
            break;
        }
    }

    if (found == TRUE) {
        s0 = g_strsplit (line, "=", 2);
        if (s0 == NULL || s0[0] == NULL || s0[1] == NULL) {
            goto error_setup_song;
        }
        s1 = g_strsplit (s0[1], " ", -1);
        if (s1 == NULL || s1[0] == NULL) {
            goto error_setup_song;
        }
        s->tunes = 0;
        gint i = 0;
        while (s1[i] != NULL) {
            s->tunes++;
            s->tune_duration[i] = _str_to_duration (s1[i]);
            if (i == 0) s->duration = s->tune_duration[i];
            i++;
            if (i > SONG_MAX_TUNES-1) break;
        }
    }
error_setup_song:
    if (filename != NULL) g_free (filename);
    if (s0 != NULL) g_strfreev (s0);
    if (s1 != NULL) g_strfreev (s1);
    if (line != NULL) free (line);
    if (fd > -1) close (fd);
    if (md5c != NULL) g_checksum_free (md5c);
}

#define KERNAL_SIZE (8*1024)
#define BASIC_SIZE (8*1024)
#define CHARGEN_SIZE (4*1024)

static GByteArray *_load_rom (const gchar *name, gsize rom_size)
{
    GByteArray *a = NULL;
    ssize_t len;
    int fd = -1;
    guint8 *buf = NULL;
    if (name == NULL) goto load_rom_error;
    fd  = open (name, O_RDONLY, 0);
    if (fd < 0) goto load_rom_error;

    buf = g_malloc0 (rom_size);
    if (buf == NULL) goto load_rom_error;

    a = g_byte_array_new ();
    if (a == NULL) goto load_rom_error;

    while ((len = read (fd, buf, rom_size)) > 0) {
        g_byte_array_append (a, buf, len);
    }

    if (a->len != rom_size) {
        g_byte_array_free (a, TRUE);
        a = NULL;
    }
load_rom_error:
    g_free (buf);
    if (fd > -1) close (fd);
    return a;
}

GByteArray *sid_kernal (void)
{
    if (_kernal != NULL) return _kernal;
    gchar name[PATH_MAX];
    if (FALSE == util_expand_tilde (config.sid_kernal_file, name)) return NULL;
    _kernal = _load_rom (name, KERNAL_SIZE);
    return _kernal;
}

GByteArray *sid_basic (void)
{
    if (_basic != NULL) return _basic;
    gchar name[PATH_MAX];
    if (FALSE == util_expand_tilde (config.sid_basic_file, name)) return NULL;
    _basic = _load_rom (name, BASIC_SIZE);
    return _basic;
}

GByteArray *sid_chargen (void)
{
    if (_chargen != NULL) return _chargen;
    gchar name[PATH_MAX];
    if (FALSE == util_expand_tilde (config.sid_chargen_file, name)) return NULL;
    _chargen = _load_rom (name, CHARGEN_SIZE);
    return _chargen;
}

static gint64 _str_to_duration (gchar *str)
{
    gint64 mins = 0;
    gint64 secs = 0;

    gchar **s = g_strsplit (str, ":", 2); /* min:sec.msec */

    if (s == NULL) return 0;
    else if (s[0] == NULL) return 0;
    else if (s[1] == NULL) return 0;

    mins = strtol (s[0], NULL, 10);
    secs = strtol (s[1], NULL, 10); /* do not care msecs */

    g_strfreev (s);

    return (((mins * 60) + secs) * 1000); /* to ms */
}

