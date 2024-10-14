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

#ifndef _KK_UTIL_
#define _KK_UTIL_

#include <stddef.h>
#include <glib.h>

#define MAX_UTF8_CHAR_SIZE (7) /* includes \0 at the end */
#define MAX_TIME_STR_LEN (20)

/* Change working dir to same directory using files path
 *
 * filepath NULL terminated string
 * return: TRUE on success */
gboolean util_chdir_filepath (const gchar *filepath);

/* Change working dir
 *
 * dirpath NULL terminated string
 * return: TRUE on success
 */
gboolean util_chdir (const gchar *dirpath);

/* current working dir
 *
 * return: pwd string on success, Empty string on fail
 */
const gchar *util_pwd (void);

/* Expands tilde to home dir in path (if first letter)
 * 
 * path NULL terminated string
 * buf PATH__MAX size buffer to copy expanded path
 * return TRUE o nsuccess. User must free return value */
gboolean util_expand_tilde (const gchar *path, gchar buf[PATH_MAX]);

/* write data to file. user:rw all others no access */
gint util_file_write_data (const gchar *filepath, const gchar *data, ssize_t len);

/* read string from file. str is allocated and must be freed if return value is 0 */
gint util_file_load_to_str (const char *filepath, gchar **str);

/* converts string to gboolean. true, yes and 1 to TRUE, else FALSE */
gboolean util_string_to_bool (const gchar *value);

/* converts time (ms) to string */
gboolean util_time_to_string (gchar *str, gint max_size, gint64 ms);

void util_ms_to_hour_min_sec(gint64 ms, gint64 *hour, gint64 *min, gint64 *sec);

const gchar util_progress (void);

guint util_g_string_replace (GString *string, const gchar *find, const gchar *replace, guint limit);

char *util_case_insensitive_strstr(const char *a, const char *b);

gboolean util_is_possibly_supported_file (const char *path);

gboolean util_has_upper (const gchar *str);

#endif
