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

#include "paths.h"

#include <fcntl.h> /* mkdir flags */

gchar *paths_home_dir (void)
{
    return g_strdup (g_get_home_dir ());
}

gchar *paths_config_dir (void)
{
    gchar dir[PATH_MAX];
    g_snprintf (dir, PATH_MAX, "%s%c%s", g_get_home_dir (), G_DIR_SEPARATOR, ".config");
    g_mkdir_with_parents (dir, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    return g_strdup (dir);
}

gchar *paths_config_data_dir (void)
{
    gchar dir[PATH_MAX];
    gchar *config_dir = paths_config_dir ();
    if (config_dir == NULL) return NULL;
    g_snprintf (dir, PATH_MAX, "%s%c%s", config_dir, G_DIR_SEPARATOR, APP_EXE);
    g_free (config_dir);
    g_mkdir_with_parents (dir, S_IRWXU|S_IRWXG|S_IROTH|S_IXOTH);
    return g_strdup (dir);
}

gchar *paths_config_data_default_config (void)
{
    gchar path[PATH_MAX + NAME_MAX + 1];
    gchar *data_dir = paths_config_data_dir ();
    if (data_dir == NULL) return NULL;
    g_snprintf (path, PATH_MAX, "%s%c%s", data_dir, G_DIR_SEPARATOR, "default.cfg");
    g_free (data_dir);
    return g_strdup (path);
}

gchar *paths_saved_data_dir (void) {
    gchar path[PATH_MAX];
    g_snprintf (path, PATH_MAX, "%s%c.local%cshare%c" APP_EXE,
        g_get_home_dir (), G_DIR_SEPARATOR, G_DIR_SEPARATOR, G_DIR_SEPARATOR);
    g_mkdir_with_parents (path, S_IRUSR | S_IWUSR | S_IXUSR);
    return g_strdup (path);
}

gchar *paths_saved_data_default_playlist (void)
{
    gchar path[PATH_MAX + NAME_MAX + 1];
    gchar *data_dir = paths_saved_data_dir ();
    if (data_dir == NULL) return NULL;
    g_snprintf (path, PATH_MAX, "%s%c%s", data_dir, G_DIR_SEPARATOR, "default.pls");
    g_free (data_dir);
    return g_strdup (path);
}

gchar *paths_saved_data_default_log (void) {
    gchar *dir = paths_saved_data_dir();
    gchar path[PATH_MAX];
    g_snprintf (path, PATH_MAX, "%s%c" APP_EXE ".log", dir, G_DIR_SEPARATOR);
    g_free (dir);
    return g_strdup (path);
}

gchar *paths_saved_data_stderr_log (void) {
    gchar *dir = paths_saved_data_dir();
    gchar path[PATH_MAX];
    g_snprintf (path, PATH_MAX, "%s%c" "stderr.log", dir, G_DIR_SEPARATOR);
    g_free (dir);
    return g_strdup (path);
}

