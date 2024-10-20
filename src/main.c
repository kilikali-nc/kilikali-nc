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

#include <signal.h>
#include <locale.h>
#include <glib.h>
#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libintl.h>

#include "ncurses-event.h"
/* working dir */
#include "util.h"
/* playlist */
#include "playlist.h"
#include "playlist-pls.h"
#include "paths.h"
#include "ncurses-screen.h"
/* other */
#include "config.h"
#include "command.h"
#include "log.h"
#include "keys.h"
#include "player.h"
#include "net.h"

static GMainLoop *loop = NULL;
static const char *help_str =
    APP_EXE " [OPTIONS] [FILE|DIRECTORY|URL|PLAYLIST ...]\n"
    "Command line options:\n"
    "-h:             Print this help dialogue.\n"
    "-c [FILE_PATH]: Set configuration file path.\n"
    "-g [FILE_PATH]: Generate an example configuration file.\n"
    "-l [FILE_PATH]: Set log file path.\n"
    "-d:             Log debugging information.\n"
    "-k:             Print key values for configuring keybindings.";

static gboolean _add_idle (gpointer data);

static void quit (int signum)
{
    if (loop != NULL) {
        g_main_loop_quit (loop);
    }
}

int main (int argc, char **argv)
{
    int retval = 0;
    struct sigaction sa;

    char add_str[PATH_MAX];
    char config_file_path_str[PATH_MAX];
    char log_file_path_str[PATH_MAX] = {0};
    int c;
    GSList *to_add = NULL;

    /* parse command line options */
    while ((c = getopt(argc, argv, "hc:g:l:dk")) != -1) {
        switch (c) {
        case 'c': {
            char *cf = realpath (optarg, config_file_path_str);
            if (config_file_path != NULL) {
                g_free (config_file_path);
            }
            config_file_path = g_strdup (cf);
            break;
        }
        case 'h': {
            puts(help_str);
            return 0;
        }
        case 'g': {
            if (config_generate_example_file(optarg)) {
                return 1;
            } else {
                return 0;
            }
            break;
        }
        case 'l': {
            size_t optargLen = strlen(optarg);
            if (optargLen >= sizeof(log_file_path_str)) {
                fprintf(stderr, "Log file path '%s' is too long: must be < %zu characters.", optarg,
                    sizeof(log_file_path_str));
                return 2;
            }
            memcpy(log_file_path_str, optarg, optargLen);
            log_file_path_str[optargLen] = 0;
            break;
        }
        case 'd': {
            log_debugging_on = 1;
            break;
        }
        case 'k': {
            return keys_print_out ();
        }
        }
    }

    while (optind < argc) {
        char *a = realpath (argv[optind], add_str);
        if (a == NULL) {
            a = argv[optind];
        }
        to_add = g_slist_prepend (to_add, g_strdup (a));
        optind++;
    }

    if (log_file_path_str[0] != 0) { /* strlen > 0 */
        log_init (log_file_path_str);
    } else {
        gchar *log_path = paths_saved_data_default_log ();
        log_init (log_path);
        g_free (log_path);
    }

    /* set locale to get UTF-8 work*/
    setlocale (LC_ALL, "");
    /* use dot with floar double with strtod, sscanf etc. and nott comma for example */
    setlocale (LC_NUMERIC, "C");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);

    playlist_init ();

    /* player might need preinit */
    player_preinit (&argc, &argv);

    /* signal handler initialization. quit with ctrl-c (SIGINT) */
    sa.sa_handler = quit;
    sigemptyset (&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction (SIGINT, &sa, NULL)) {
        return 1;
    }

    /* curl init for net downloads */
    curl_global_init (CURL_GLOBAL_DEFAULT);

    /* if user has not set config use default one */
    if (config_file_path == NULL) {
        struct stat s;
        int status;
        config_file_path = paths_config_data_default_config ();
        status = stat (config_file_path, &s);
        if (status == -1) config_generate_example_file (config_file_path);
    }

    if (config_init()) {
        LOG_ERROR("config_init() failed.");
        retval = 3;
        goto error;
    }

    if (command_init()) {
        LOG_ERROR("command_init() failed.");
        retval = 4;
        goto error;
    }

    if (config.default_music_directory != NULL) {
        if (util_chdir (config.default_music_directory) == FALSE) {
            LOG_ERROR("Failed to enter default music directory '%s'. Try "
                "changing the path in your config file.",
                config.default_music_directory);
            retval = 5;
            goto error;
        }
    } else {
        if (util_chdir (g_get_home_dir ()) == FALSE) { /* get home dir does not make a copy */
            LOG_ERROR("Failed to enter home directory '%s'.", g_get_home_dir ());
            retval = 6;
            goto error;
        }
    }

    loop = g_main_loop_new (NULL, FALSE);
    if (loop == NULL) {
        LOG_ERROR("g_main_loop_new() failed.");
        retval = 7;
        goto error;
    }

    if (ncurses_screen_init() == FALSE) {
        LOG_ERROR("ncurses_screen_init() failed.");
        retval = 8;
        goto error;
    }

    /* Add default playlist to end of list */
    to_add = g_slist_append (to_add,  paths_saved_data_default_playlist ());

    player_set_volume(100);

    g_timeout_add (1, ncurses_event_idle, NULL); /* read keyboard and mouse */

    if (to_add != NULL) {
        g_timeout_add (100, _add_idle, to_add);
    }
    g_main_loop_run (loop);
    if (config.playlist_save_at_exit == TRUE) {
        gchar *pl = paths_saved_data_default_playlist ();
        if (pl != NULL) {
            playlist_pls_save (playlist_get (), pl);
            g_free (pl);
        }
    }

error:
    if (to_add != NULL) {
        for (gint i = 0; i < g_slist_length (to_add); i++) {
            g_free (g_slist_nth_data (to_add, i));
        }
        g_slist_free (to_add);
    }
    if (config_file_path != NULL) {
        g_free (config_file_path);
    }
    if (loop != NULL) {
        g_main_loop_unref (loop);
    }
    ncurses_screen_free ();
    playlist_free ();
    config_destroy ();
    command_destroy ();
    net_free ();
    curl_global_cleanup ();
    log_destroy();
    return retval;
}

static gboolean _add_idle (gpointer data)
{
    if (data == NULL) {
        return FALSE;
    }

    GSList *to_add = (GSList *)data;
    /* reverse order */
    //clock_t start = clock();
    for (gint i = g_slist_length (to_add) - 1; i > -1; i--) {
        gchar *str = (gchar *)g_slist_nth_data (to_add, i);
        if (str != NULL) {
            (void)playlist_add (str);
        }
    }
    //fprintf (stderr, "time: %ld\n", clock()-start);
    ncurses_screen_update_force ();
    return FALSE;
}
