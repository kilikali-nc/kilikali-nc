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

#ifndef _KK_CONFIG_
#define _KK_CONFIG_

#include <glib.h>

#define CONFIG_MAX_KEY_NAME_LEN 32
#define CONFIG_MAX_KEYBIND_VARIANTS 4
#define INVALID_NCURSES_CH 666666

extern gchar *config_file_path;

typedef enum {
    CONFIG_OPTION_TYPE_INTEGER,
    CONFIG_OPTION_TYPE_UNSIGNED_INTEGER,
    CONFIG_OPTION_TYPE_STRING,
    CONFIG_OPTION_TYPE_BOOLEAN,
    CONFIG_OPTION_TYPE_DOUBLE,
    CONFIG_OPTION_TYPE_SIZED_STRING,
    CONFIG_OPTION_TYPE_KEYBIND
} ConfigOptionType;

typedef struct
{
    char    keys[CONFIG_MAX_KEYBIND_VARIANTS][CONFIG_MAX_KEY_NAME_LEN];
    guint8  num_keys;
} Keybind;

typedef struct {
    gchar *output;
    gchar *alsa_device;
    gchar *default_music_directory;
    gchar *stream_error_action;
    gchar *playlist_line;
    gboolean playlist_save_at_exit;
    gchar *wild;
    gchar *search_case_sensivity;
    /* sid related */
    gchar *sid_songlengths_file;
    gint sid_default_songlength;
    /* for siddecfp */
    gboolean sid_filter;
    gint sid_sid_model;
    gboolean sid_force_sid_model;
    gint sid_c64_model;
    gboolean sid_force_c64_model;
    gint sid_cia_model;
    gboolean sid_digiboost;
    gint sid_sampling_method;
    gdouble sid_filter_bias;
    gdouble sid_filter_curve_6581;
    gdouble sid_filter_curve_8580;
    gchar *sid_kernal_file;
    gchar *sid_basic_file;
    gchar *sid_chargen_file;
    guint lyrics_service;
    gint max_filebrowser_entries;
    /* keybindings */
    Keybind key_global_volume_up;
    Keybind key_global_volume_down;
    Keybind key_common_abort;
    Keybind key_move_up;
    Keybind key_move_down;
    Keybind key_move_half_page_up;
    Keybind key_move_half_page_down;
    Keybind key_move_full_page_down;
    Keybind key_move_full_page_up;
    Keybind key_move_top;
    Keybind key_move_bottom;
    Keybind key_scroll_up;
    Keybind key_scroll_down;
    Keybind key_center_screen_on_cursor;
    Keybind key_playlist_select;
    Keybind key_playlist_mode;
    Keybind key_playlist_select_multiple_up;
    Keybind key_playlist_select_multiple_down;
    Keybind key_playlist_select_toggle;
    Keybind key_playlist_select_all;
    Keybind key_playlist_loop_toggle;
    Keybind key_playlist_remove_songs;
    Keybind key_playlist_copy;
    Keybind key_playlist_cut;
    Keybind key_playlist_paste;
    Keybind key_playlist_paste_before;
    Keybind key_seek_forward;
    Keybind key_seek_backward;
    Keybind key_playpause_toggle;
    Keybind key_quit;
    Keybind key_command_mode;
    Keybind key_search_mode;
    Keybind key_search_next;
    Keybind key_search_previous;
    Keybind key_edit_mode;
    Keybind key_tune_next;
    Keybind key_tune_prev;
    Keybind key_open_filebrowser;
    Keybind key_filebrowser_add;
    Keybind key_filebrowser_change_directory;
    Keybind key_filebrowser_refresh;
    Keybind key_filebrowser_previous_directory;
    Keybind key_help;
    Keybind key_sort;
    Keybind key_open_lyrics;
} Config;

typedef struct {
    const char *name;
    ConfigOptionType type;
    int required;
    union {
        int *integer;
        unsigned int *unsigned_integer;
        char **string;
        gboolean *boolean;
        gdouble *d;
        struct {
            char *data; /* Can be null */
            size_t max_len; /* Includes null terminator */
        } sized_string;
        Keybind *keybind;
    } value;
    union {
        int integer;
        unsigned int unsigned_integer;
        char *string; /* Can be null */
        gboolean boolean;
        gdouble d;
        gchar *sized_string; /* Can be null */
        Keybind keybind;
    } default_value;
    const char *comment; /* For default config generation. Can be null. */
    /* Initialize data below this line to 0. */
    gboolean have; /* Marked as TRUE after parsing if option was defined in file. */
} ConfigOption;

extern Config config;

/* Parse a configuration string.
 * Returns 0 if the string is a valid configuration string.
 * - str: The config string to parse, for example the contents of a config file.
 * - callback: a function called for every "option = value" pair. The strings
 *   passed to the callback are NOT null terminated - instead, string lengths are
 *   passed as arguments.
 * - callback_data: optional pointer to user data passed to the callback. */
int config_parse_string (const char *str,
    int (*callback) (void *callback_data,
        const char *opt, int opt_len,
        const char *val, int val_len),
    void *callback_data);

int config_parse_string_with_options (const char *str,
    ConfigOption *config_options, int num_options);

void config_destroy_parsed_options (ConfigOption *opts, int num_opts);

/* Return non-zero on failure. */
int config_init (void);

void config_destroy (void);

int config_generate_example_file (const char *file_path);

gboolean config_option_key_prefix_found (const char *str);
gboolean config_option_key_in_reserved_list (int ch);
#endif
