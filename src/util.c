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

#include <stdlib.h>
#include <string.h>
#include <unistd.h> /* chdir */
#include <limits.h>
#include <fcntl.h> /* open */
#include <ctype.h>
#include <magic.h>
#include <sys/file.h>

#include "util.h"
#include "log.h"

gboolean util_chdir_filepath (const gchar *filepath) {
    gint ret = -1;
    gchar *dirpath;
    char str[PATH_MAX];
    char *rpath = realpath (filepath, str);
    if (rpath == NULL) return FALSE;

    dirpath = g_path_get_dirname (rpath);
    if (dirpath == NULL) goto chdir_filepath_error;

    ret = chdir (dirpath);
chdir_filepath_error:
    g_free (dirpath);
    return (ret==0?TRUE:FALSE);
}

gboolean util_chdir (const gchar *dirpath) {
    gint ret = -1;
    gchar expanded[PATH_MAX];
    if (FALSE == util_expand_tilde (dirpath, expanded)) {
        return FALSE;
    }
    ret = chdir (expanded);
/*chdir_filepath_error:*/
    return (ret==0?TRUE:FALSE);
}


char _pwdpath[PATH_MAX] = "";
const gchar *util_pwd (void)
{
    char *path = getcwd(_pwdpath, PATH_MAX);
    if (path == NULL) {
       LOG_ERROR("Failed with getwd. errno:%d", errno);
    }
    return _pwdpath;
}

gboolean util_expand_tilde (const gchar *path, gchar buf[PATH_MAX])
{
    if (path[0] != '~') {
        size_t path_len = strlen(path);
        if (path_len > PATH_MAX - 1) {
            return FALSE;
        }
        memcpy(buf, path, path_len + 1);
        return TRUE;
    }
    const gchar *home = g_get_home_dir ();
    if (!home) {
        return FALSE;
    }
    path = path + 1;
    size_t home_len = strlen(home);
    size_t path_len = strlen(path);
    if (home_len + 1 + path_len > PATH_MAX - 1) {
        return FALSE;
    }
    memcpy(buf, home, home_len);
    buf[home_len] = G_DIR_SEPARATOR;
    memcpy(buf + home_len + 1, path, path_len);
    buf[home_len + 1 + path_len] = 0;
    return TRUE;
}

gint util_file_write_data (const gchar *filepath, const gchar *data, ssize_t len)
{
    int fd;
    ssize_t wrote_size;
    gchar expanded[PATH_MAX] = "";
    if (FALSE == util_expand_tilde (filepath, expanded)) return FALSE;
    else if (strlen(expanded) == 0) return FALSE;
    fd = open (expanded, O_CREAT|O_TRUNC|O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH); /* user read/write */
    if (fd < 1) return -1;
    int lockret = flock (fd, LOCK_EX);
    if (lockret != 0) {
       close (fd);
       return -1;
    }
    wrote_size = write (fd, data, len);
    lockret = flock (fd, LOCK_UN);
    close (fd);

    if (wrote_size != len) return -1;
    return 0;
}

gint util_file_load_to_str (const char *filepath, gchar **str)
{
    int ret = 0;
    int lockret = -1;
    if (filepath == NULL || strlen (filepath) == 0) {
        LOG_ERROR("Failed to read file: No filepath empty.");
        return -1;
    }
    gchar expanded[PATH_MAX] = "";
    if (FALSE == util_expand_tilde (filepath, expanded)) {
        LOG_ERROR("Failed to expand file path (%s): expand tilde.", filepath);
        return -1;
    } else if (strlen(expanded) == 0) {
        LOG_ERROR("Failed to expand file path (%s): expanded path len zero.", filepath);
        return -1;
    }
    int fd = open (expanded, O_RDONLY);
    if (fd == -1) {
        LOG_ERROR("Failed to read file (%s): Could not open file.", filepath);
        return -1;
    }
    lockret = flock (fd, LOCK_SH);
    if (lockret == -1) {
        ret = -1;
        LOG_ERROR("Failed to read file (%s): Could not lock file.", filepath);
        goto load_error;
    }
    off_t file_size = lseek (fd, 0, SEEK_END);
    if (file_size <= 0) {
        LOG_ERROR("Failed to read file (%s): empty file.", filepath);
        ret = -1;
        goto load_error;
    }
    off_t start = lseek (fd, 0, SEEK_SET);
    if (start != 0) {
        LOG_ERROR("Failed to read file (%s): seek error.", filepath);
        ret = -1;
        goto load_error;
    }
    *str = g_malloc0 (file_size + 1);
    ssize_t size = read (fd, (void *)*str, (size_t)file_size);
    if (size != (ssize_t)file_size) {
        LOG_ERROR("Failed to read file (%s): read returned a different size from the size of the file.", filepath);
        g_free (*str);
        ret = -1;
    }
 load_error:
    if (lockret == 0) {
        if (flock (fd, LOCK_UN) != 0) {
            LOG_ERROR("Failed to read file (%s): could not unlock file", filepath);
            ret = -1;
        }
    }
    close (fd);
    return ret;
}


gboolean util_string_to_bool (const gchar *value)
{
    if (value == NULL) return FALSE;
    else if (strlen (value) < 1) return FALSE;
    else if (value[0] == 't') return TRUE;
    else if (value[0] == 'y') return TRUE;
    else if (value[0] == '1') return TRUE;
    return FALSE;
}

gboolean util_time_to_string (gchar *str, gint max_size, gint64 ms)
{
    gint h = ms / (1000*60*60);
    gint m = (ms / (1000*60)) - (h*60);
    gint s = (ms / 1000) - (m*60) - (h*60*60);

    if (h > 0) g_snprintf (str, max_size, "%d:%02d:%02d", h, m ,s);
    else g_snprintf (str, max_size, "%02d:%02d", m ,s);
    return TRUE;
}

void util_ms_to_hour_min_sec(gint64 ms, gint64 *hour, gint64 *min, gint64 *sec)
{
    *hour = ms / (1000 * 60 * 60);
    *min = (ms / (1000 * 60)) - (*hour * 60);
    *sec = (ms / 1000) - (*min * 60) - (*hour * 60 * 60);
}

const gchar util_progress (void)
{
    static gint i = -1;
    static const gchar *c = "-\\|/";
    ++i;
    if (i > 3) i = 0;
    return c[i];
}

/* from glib. To get lower version requirements to glib */
guint util_g_string_replace (GString *string, const gchar *find, const gchar *replace, guint limit)
{
    gsize f_len, r_len, pos;
    gchar *cur, *next;
    guint n = 0;

    g_return_val_if_fail (string != NULL, 0);
    g_return_val_if_fail (find != NULL, 0);
    g_return_val_if_fail (replace != NULL, 0);

    f_len = strlen (find);
    r_len = strlen (replace);
    cur = string->str;

    while ((next = strstr (cur, find)) != NULL)
    {
        pos = next - string->str;
        g_string_erase (string, pos, f_len);
        g_string_insert (string, pos, replace);
        cur = string->str + pos + r_len;
        n++;
        /* Only match the empty string once at any given position, to
         * avoid infinite loops */
        if (f_len == 0)
        {
            if (cur[0] == '\0')
                break;
            else
                cur++;
        }
        if (n == limit)
            break;
    }

    return n;
}
#include <stdio.h>
char *util_case_insensitive_strstr(const char *a, const char *b)
{
    gunichar uc;
    gunichar utmp;
    gchar *next;
    if (b[0] == 0) {
        return NULL;
    }
    const char *start = a;
    const char *tmp = b;
    for (const char *c = start;;) {
        if (!*tmp) {
            return (char*)start;
        }
        if (!*c) {
            break;
        }
        uc = g_utf8_get_char (c);
        utmp = g_utf8_get_char (tmp);
        next = g_utf8_find_next_char (c, NULL);
        if (g_unichar_tolower(uc) != g_unichar_tolower(utmp)) {
            start = next;
            c = start;
            tmp = b;
        } else {
            c = next;
            tmp = g_utf8_find_next_char (tmp, NULL);
        }
    }
    return NULL;
}


gboolean util_is_possibly_supported_file (const char *path)
{
    gboolean possibly_supported = FALSE;
#ifdef MAGIC_MIME_TYPE
    magic_t magic_cookie = magic_open (MAGIC_ERROR | MAGIC_MIME_TYPE);
#else
    magic_t magic_cookie = magic_open (MAGIC_ERROR | MAGIC_MIME);
#endif
    const char *mime = NULL;

    /* possible to init magic ??? */
    if (magic_cookie == NULL) {
        return TRUE; /* test anyway */
    }

    /* load default db */
    if (magic_load (magic_cookie, NULL) != 0) {
        magic_close (magic_cookie);
        return TRUE; /* test anyway */
    }

    int fd = open (path, O_RDONLY);
    if (fd < 0) {
        goto error;
    }

    mime = magic_descriptor (magic_cookie, fd);
    if (mime == NULL) {
        goto error;
    }

    if (strncmp ("audio", mime, 5) == 0) {
        possibly_supported = TRUE;
    } else if (strncmp ("text", mime, 4) == 0) {
        possibly_supported = TRUE;
    } else if (strncmp ("application/octet-stream", mime, 25) == 0) {
        char buf[4];
        ssize_t len = read (fd, buf, 4);
        if (len == 4) {
            if (strncmp ("PSID", buf, 4) == 0) {
                possibly_supported = TRUE;
            } else if (strncmp ("RSID", buf, 4) == 0) {
                possibly_supported = TRUE;
            } else if (strncmp ("ID3", buf, 3) == 0) {
                possibly_supported = TRUE;
            }
        }
    }

error:
    //fprintf (stderr, "%s: %s %s\n", __FUNCTION__, path, possibly_supported==TRUE?"TRUE":"FALSE"); 
    if (fd > 0) close (fd);
    magic_close (magic_cookie);
    return possibly_supported;
}

gboolean util_has_upper (const gchar *str)
{
    const gchar *current = str;
    gboolean ret = FALSE;
    gunichar c;
    if (str == NULL) return FALSE;

    do {
        if (current[0] == '\0') break;
        c = g_utf8_get_char (current);
        if (g_unichar_isupper (c)) {
            ret = TRUE;
            break;
        }
        current = g_utf8_find_next_char (current, NULL);
    } while (1);

    return ret;
}
