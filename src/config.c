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

#include "config.h"
#include "util.h"
#include "log.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ncurses.h>

#include <sys/file.h>

#define _DEFAULT_MUSIC_DIRECTORY "~/Music"
#define _DEFAULT_C64_SID_SONGLENGTHS_FILE "~/C64Music/DOCUMENTS/Songlengths.md5"
#define _NUM_OPTIONS ((sizeof (_options) / sizeof (_options[0])))

typedef struct {
    ConfigOption *opts;
    int num_opts;
    GString *value;
} ParseStringWithOptionsContext;

gchar *config_file_path = NULL;
Config config = {0,};

/* keep invalid at last one */
int _option_key_reserved[] = {410 /* resize */, INVALID_NCURSES_CH };

static ConfigOption _options[] = {
    {
        .name = "output",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.output,
        .default_value.string = "default",
        .have = 0,
        .comment = "output. Options: default, alsa or pulse"
    },
    {
        .name = "alsa_device",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.alsa_device,
        .default_value.string = "default",
        .have = 0,
        .comment = "alsa_device. Default option: default."
    },
    {
        .name = "default_music_directory",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.default_music_directory,
        .default_value.string = _DEFAULT_MUSIC_DIRECTORY,
        .have = 0,
        .comment = "default_music_directory. The relative starting point when browsing for music files to add. Other paths are also relative from here."
    },
    {
        .name = "stream_error_action",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.stream_error_action,
        .default_value.string = "default",
        .have = 0,
        .comment = "stream_error_action. What to do when an audio stream unexpectedly stops, "
            "for example due to a disconnect. options: default, next or pause."
    },
    {
        .name = "playlist_line",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.playlist_line,
        .default_value.string = "%t (%l)",
        .have = 0,
        .comment = "playlist_line. Formatting of the playlist line. "
            "Switches: \%t = title, \%a = artist, \%A = album, %f = file name \%l = length"
    },
    {
        .name = "playlist_save_at_exit",
        .type = CONFIG_OPTION_TYPE_BOOLEAN,
        .required = 0,
        .value.boolean = &config.playlist_save_at_exit,
        .default_value.boolean = FALSE,
        .have = 0,
        .comment = "playlist_save_at_exit. Options: true, false, yes, no, 0 or 1."
    },
    {
        .name = "wild",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.wild,
        .default_value.string = "none",
        .have = 0,
        .comment = "wild mode aka completion menu. Options: list, none. list shows options in user information bar. none cycles option on command line"
    },
    {
        .name = "search_case_sensivity",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.search_case_sensivity,
        .default_value.string = "smart",
        .have = 0,
        .comment = "search_case_sensivity. Options: smart, yes, no. smart is case sensitive when upper chars appear in search, yes is and no is not case sensitive"
    },
    {
        .name = "sid_kernal_file",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.sid_kernal_file,
        .default_value.string = "kernal.bin",
        .have = 0,
        .comment = "sid_kernal_file. File path to C64 ROM file for SID decoder."
    },
    {
        .name = "sid_basic_file",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.sid_basic_file,
        .default_value.string = "basic.bin",
        .have = 0
    },
    {
        .name = "sid_chargen_file",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.sid_chargen_file,
        .default_value.string = "chargen.bin",
        .have = 0
    },
    {
        .name = "sid_songlengths_file",
        .type = CONFIG_OPTION_TYPE_STRING,
        .required = 0,
        .value.string = &config.sid_songlengths_file,
        .default_value.string = _DEFAULT_C64_SID_SONGLENGTHS_FILE,
        .have = 0,
        .comment = "sid_songlengths_file. Path to HVSIDs Songlengths.md5 file (new version)."
    },
    {
        .name = "sid_default_songlength",
        .type = CONFIG_OPTION_TYPE_INTEGER,
        .required = 0,
        .value.integer = &config.sid_default_songlength,
        .default_value.integer = 180,
        .have = 0,
        .comment = "sid_default_songlength. Default C64 SID playtime if time is not known."
    },
    {
        .name = "sid_filter",
        .type = CONFIG_OPTION_TYPE_BOOLEAN,
        .required = 0,
        .value.boolean = &config.sid_filter,
        .default_value.boolean = FALSE,
        .have = 0
    },
    {
        .name = "sid_sid_model",
        .type = CONFIG_OPTION_TYPE_INTEGER,
        .required = 0,
        .value.integer = &config.sid_sid_model,
        .default_value.integer = 0,
        .have = 0
    },
    {
        .name = "sid_force_sid_model",
        .type = CONFIG_OPTION_TYPE_BOOLEAN,
        .required = 0,
        .value.boolean = &config.sid_force_sid_model,
        .default_value.boolean = FALSE,
        .have = 0
    },
    {
        .name = "sid_c64_model",
        .type = CONFIG_OPTION_TYPE_INTEGER,
        .required = 0,
        .value.integer = &config.sid_c64_model,
        .default_value.integer = 0,
        .have = 0
    },
    {
        .name = "sid_force_c64_model",
        .type = CONFIG_OPTION_TYPE_BOOLEAN,
        .required = 0,
        .value.boolean = &config.sid_force_c64_model,
        .default_value.boolean = FALSE,
        .have = 0
    },
    {
        .name = "sid_cia_model",
        .type = CONFIG_OPTION_TYPE_INTEGER,
        .required = 0,
        .value.integer = &config.sid_cia_model,
        .default_value.integer = 0,
        .have = 0
    },
    {
        .name = "sid_digiboost",
        .type = CONFIG_OPTION_TYPE_BOOLEAN,
        .required = 0,
        .value.boolean = &config.sid_digiboost,
        .default_value.boolean = FALSE,
        .have = 0
    },
    {
        .name = "sid_sampling_method",
        .type = CONFIG_OPTION_TYPE_INTEGER,
        .required = 0,
        .value.integer = &config.sid_sampling_method,
        .default_value.integer = 0,
        .have = 0
    },
    {
        .name = "sid_filter_bias",
        .type = CONFIG_OPTION_TYPE_DOUBLE,
        .required = 0,
        .value.d = &config.sid_filter_bias,
        .default_value.d = 0.5,
        .have = 0
    },
    {
        .name = "sid_filter_curve_6581",
        .type = CONFIG_OPTION_TYPE_DOUBLE,
        .required = 0,
        .value.d = &config.sid_filter_curve_6581,
        .default_value.d = 0.5,
        .have = 0
    },
    {
        .name = "sid_filter_curve_8580",
        .type = CONFIG_OPTION_TYPE_DOUBLE,
        .required = 0,
        .value.d = &config.sid_filter_curve_8580,
        .default_value.d = 0.5,
        .have = 0
    },
    {
        .name = "max_filebrowser_entries",
        .type = CONFIG_OPTION_TYPE_INTEGER,
        .required = 0,
        .value.integer = &config.max_filebrowser_entries,
        .default_value.integer = 10000,
        .have = 0,
        .comment = "max_filebrowser_entries. The maximum number of files in a directory the filebrowser can show."
    },
    {
        .name = "lyrics_service",
        .type = CONFIG_OPTION_TYPE_UNSIGNED_INTEGER,
        .required = 0,
        .value.unsigned_integer = &config.lyrics_service,
        .default_value.unsigned_integer = 0,
        .have = 0,
        .comment = "lyrics service to use. 0 = none, 1 = Chart Lyrics"
    },
    {
        .name = "key_common_abort",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_common_abort,
        .default_value.keybind = {.keys = {"^[", "^C"}, .num_keys = 2},
        .have = 0
    },
    {
        .name = "key_playlist_select",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_select,
        .default_value.keybind = {.keys = {"^J"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_mode",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_mode,
        .default_value.keybind = {.keys = {"m"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_move_up",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_move_up,
        .default_value.keybind = {
            .keys = {"k", "KEY_UP"},
            .num_keys = 2
        },
        .have = 0
    },
    {
        .name = "key_move_down",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_move_down,
        .default_value.keybind = {
            .keys = {"j", "KEY_DOWN"},
            .num_keys = 2
        },
        .have = 0
    },
    {
        .name = "key_move_half_page_up",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_move_half_page_up,
        .default_value.keybind = {.keys = {"^U"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_move_half_page_down",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_move_half_page_down,
        .default_value.keybind = {.keys = {"^D"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_move_full_page_down",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_move_full_page_down,
        .default_value.keybind = {.keys = {"^F", "KEY_NPAGE"}, .num_keys = 2},
        .have = 0
    },
    {
        .name = "key_move_full_page_up",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_move_full_page_up,
        .default_value.keybind = {.keys = {"^B", "KEY_PPAGE"}, .num_keys = 2},
        .have = 0
    },
    {
        .name = "key_move_top",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_move_top,
        .default_value.keybind = {.keys = {"gg"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_move_bottom",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_move_bottom,
        .default_value.keybind = {.keys = {"G"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_scroll_up",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_scroll_up,
        .default_value.keybind = {.keys = {"^Y"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_scroll_down",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_scroll_down,
        .default_value.keybind = {.keys = {"^E"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_center_screen_on_cursor",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_center_screen_on_cursor,
        .default_value.keybind = {.keys = {"zz"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_select_multiple_up",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_select_multiple_up,
        .default_value.keybind = {.keys = {"KEY_SR"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_select_multiple_down",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_select_multiple_down,
        .default_value.keybind = {.keys = {"KEY_SF"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_select_toggle",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_select_toggle,
        .default_value.keybind = {.keys = {"V"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_select_all",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_select_all,
        .default_value.keybind = {.keys = {"^A"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_loop_toggle",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_loop_toggle,
        .default_value.keybind = {.keys = {"l"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_remove_songs",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_remove_songs,
        .default_value.keybind = {.keys = {"KEY_DC"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_copy",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_copy,
        .default_value.keybind = {.keys = {"y"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_cut",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_cut,
        .default_value.keybind = {.keys = {"d", "x"}, .num_keys = 2},
        .have = 0
    },
    {
        .name = "key_playlist_paste",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_paste,
        .default_value.keybind = {.keys = {"p"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playlist_paste_before",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playlist_paste_before,
        .default_value.keybind = {.keys = {"P"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_seek_forward",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_seek_forward,
        .default_value.keybind = {.keys = {"KEY_RIGHT"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_seek_backward",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_seek_backward,
        .default_value.keybind = {.keys = {"KEY_LEFT"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_playpause_toggle",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_playpause_toggle,
        .default_value.keybind = {.keys = {" "}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_quit",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_quit,
        .default_value.keybind = {.keys = {"q", "ZZ"}, .num_keys = 2},
        .have = 0
    },
    {
        .name = "key_command_mode",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_command_mode,
        .default_value.keybind = {.keys = {":"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_search_mode",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_search_mode,
        .default_value.keybind = {.keys = {"/"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_search_next",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_search_next,
        .default_value.keybind = {.keys = {"n"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_search_previous",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_search_previous,
        .default_value.keybind = {.keys = {"N"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_edit_mode",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_edit_mode,
        .default_value.keybind = {.keys = {"e"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_global_volume_up",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_global_volume_up,
        .default_value.keybind = {.keys = {"^K"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_global_volume_down",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_global_volume_down,
        .default_value.keybind = {.keys = {"^L"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_tune_next",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_tune_next,
        .default_value.keybind = {.keys = {">"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_tune_prev",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_tune_prev,
        .default_value.keybind = {.keys = {"<"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_open_filebrowser",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_open_filebrowser,
        .default_value.keybind = {.keys = {"a"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_filebrowser_add",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_filebrowser_add,
        .default_value.keybind = {.keys = {" "}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_filebrowser_change_directory",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_filebrowser_change_directory,
        .default_value.keybind = {.keys = {"^J"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_filebrowser_refresh",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_filebrowser_refresh,
        .default_value.keybind = {.keys = {"u"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_filebrowser_previous_directory",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_filebrowser_previous_directory,
        .default_value.keybind = {.keys = {"-"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_help",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_help,
        .default_value.keybind = {.keys = {"h"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_open_lyrics",
        .type = CONFIG_OPTION_TYPE_KEYBIND,
        .required = 0,
        .value.keybind = &config.key_open_lyrics,
        .default_value.keybind = {.keys = {"L"}, .num_keys = 1},
        .have = 0
    },
    {
        .name = "key_sort",
        .type = CONFIG_OPTION_TYPE_INTEGER,
        .required = 0,
        .value.keybind = &config.key_sort,
        .default_value.keybind = {.keys = {"s"}, .num_keys = 1},
        .have = 0
    }
};

static int _contains_reserved_str (const char *val, size_t len);

static const char *_find_next_non_whitespace_on_line (const char *s)
{
    for (const char *c = s; *c && *c != '\n';) {
        const char *next = g_utf8_find_next_char (c, NULL);
        size_t len = (size_t)(next - c);
        if (len != 1) {
            return c;
        }
        if (!isspace(*c)) {
            return c;
        }
        c = next;
    }
    return 0;
}

static int _process_line (const char *line,
    int (*callback) (void *callback_data,
        const char *opt, int opt_len,
        const char *val, int val_len),
    void *callback_data)
{
    //int have_opt = 0;
    int opt_len = 0;
    int val_len = 0;
    const char *opt = _find_next_non_whitespace_on_line (line);
    const char *val;
    const char *c = opt;
    if (!opt) {
        return 0; /* Empty line */
    }
    if (*c == '#') {
        return 0;
    }
    /* Find left-hand side of the option-value combo (OPTION = value) */
    int have_whitespace = 0;
    int have_equal_sign = 0;
    for (; *c && *(c) != '\n';) {
        const char *next = g_utf8_find_next_char (c, NULL);
        size_t len = (size_t)(next - c);
        if (*c == '=') {
            have_equal_sign = 1;
            break;
        } else if (isspace (*c)) {
            if (have_whitespace) {
                return -1;
            } else {
                have_whitespace = 1;
            }
        } else {
            opt_len += len;
        }
        c = next;
    }
    if (!have_equal_sign) { /* '=' character was not found. */
        return -1;
    }
    if (opt_len < 1) {
        return -1;
    }
    /* Find right-hand side of the option-value combo (option = VALUE) */
    c++; /* Skip = */
    val = _find_next_non_whitespace_on_line (c);
    if (val) {
        while (*c && *c != '\n') {
            const char *next = g_utf8_find_next_char (c, NULL);
            size_t len = (size_t)(next - c);
            val_len += len;
            c = next;
        }
        assert(!*c || *c == '\n');
        while (isspace(*c)) {
            const char *next = g_utf8_find_next_char (c, NULL);
            size_t len = (size_t)(next - c);
            val_len -= len;
            const char *prev = g_utf8_prev_char (c);
            c = prev;
        }
    }
    if (callback (callback_data, opt, opt_len, val, val_len) != 0) {
        return -1;
    }
    return 0;
}

static int _parse_string_with_options_callback (void *callback_data,
    const char *opt, int opt_len, const char *val, int val_len) {
    ParseStringWithOptionsContext *context = callback_data;
    g_string_set_size (context->value, 0);
    ConfigOption *opts = context->opts;
    int num_opts = context->num_opts;
    int found_option = 0;
    for (int i = 0; i < num_opts; ++i) {
        if (strncmp (opts[i].name, opt, opt_len) == 0) {
            if (opts[i].have) {
                LOG_ERROR ("Unique config option '%s' defined more than once.",
                    opts[i].name);
                return -1;
            }
            switch (opts[i].type) {
            case CONFIG_OPTION_TYPE_INTEGER: {
                g_string_append_len (context->value, val, val_len);
                int value;
                if (sscanf (context->value->str, "%d", &value) != 1) {
                    LOG_ERROR ("Bad integer value in config for option '%s'.", opts[i].name);
                    return -1;
                }
                *opts[i].value.integer = value;
                break;
            }
            case CONFIG_OPTION_TYPE_UNSIGNED_INTEGER: {
                if (val[0] == '-') {
                    LOG_ERROR("Bad unsigned integer value in config for option '%s'.", opts[i].name);
                    return -1;
                }
                g_string_append_len(context->value, val, val_len);
                unsigned int value;
                if (sscanf (context->value->str, "%u", &value) != 1) {
                    LOG_ERROR ("Bad unsigned integer value in config for option '%s'.", opts[i].name);
                    return -1;
                }
                *opts[i].value.unsigned_integer = value;
                break;
            }
            case CONFIG_OPTION_TYPE_STRING: {
                *opts[i].value.string = malloc (val_len + 1);
                if (!*opts[i].value.string) {
                    LOG_ERROR ("Failed to allocate memory for the value of option '%s'.", opts[i].name);
                    return -1;
                }
                memcpy (*opts[i].value.string, val, val_len);
                (*opts[i].value.string)[val_len] = 0;
                break;
            }
            case CONFIG_OPTION_TYPE_BOOLEAN: {
                if (strncmp (val, "true", val_len) == 0 ||
                    strncmp (val, "yes", val_len) == 0 ||
                    strncmp (val, "1", val_len) == 0) {
                    *opts[i].value.boolean = TRUE;
                } else if (strncmp (val, "false", val_len) == 0 ||
                    strncmp (val, "no", val_len) == 0 ||
                    strncmp (val, "0", val_len) == 0) {
                    *opts[i].value.boolean = FALSE;
                } else {
                    LOG_ERROR ("Bad bool value in config: must be true, false, yes, no, 1 or 0.");
                    return -1;
                }
                break;
            }
            case CONFIG_OPTION_TYPE_DOUBLE: {
                g_string_append_len (context->value, val, val_len);
                double value;
                if (sscanf (context->value->str, "%lg", &value) != 1) {
                    return -1;
                }
                *opts[i].value.d = value;
                break;
            }
            case CONFIG_OPTION_TYPE_SIZED_STRING: {
                assert (opts[i].value.sized_string.max_len > 0);
                size_t len = val_len + 1;
                if (len > opts[i].value.sized_string.max_len) {
                    LOG_ERROR ("Bad sized string value in config: length must be <= %zu.",
                        opts[i].value.sized_string.max_len ? opts[i].value.sized_string.max_len - 1 : 0);
                    return -1;
                }
                memcpy (opts[i].value.sized_string.data, val, val_len);
                opts[i].value.sized_string.data[val_len] = 0;
                break;
            }
            case CONFIG_OPTION_TYPE_KEYBIND: {
                if (opts[i].value.keybind->num_keys ==
                    CONFIG_MAX_KEYBIND_VARIANTS) {
                    LOG_ERROR ("Keybind %s assigned too many times in config "
                        "(max allowed amount is %d).", val,
                        CONFIG_MAX_KEYBIND_VARIANTS);
                    return -1;
                }
                if (val_len > CONFIG_MAX_KEY_NAME_LEN - 1) {
                    LOG_ERROR ("Keybind value too long in config (max bytes without null terminator: %zu).",
                        (size_t)(CONFIG_MAX_KEY_NAME_LEN - 1));
                    return -1;
                }
                if (_contains_reserved_str (val, val_len)) {
                    LOG_ERROR ("Reserved string value in config: %s.", val);
                    return -1;
                }
                guint8 index = opts[i].value.keybind->num_keys++;
                memcpy (opts[i].value.keybind->keys[index], val, val_len);
                opts[i].value.keybind->keys[index][val_len] = 0;
                break;
            }
            }
            opts[i].have = TRUE;
            found_option = 1;
            break;
        }
    }
    if (!found_option) {
        LOG("Unknown config option '%.*s'.", opt_len, opt);
        return -1;
    }
    return 0;
}

int config_parse_string (const char *str,
    int (*callback) (void *callback_data,
        const char *opt, int opt_len,
        const char *val, int val_len),
    void *callback_data)
{
    const char *c = str;
    const char *line = str;
    for (;; c = g_utf8_find_next_char (c, NULL)) {
        if (*c == '\n' || !*c) {
            if (_process_line (line, callback, callback_data)) {
                return -1;
            }
            if (!*c) {
                break;
            }
            line = c + 1;
        }
    }
    return 0;
}

int config_parse_string_with_options (const char *str,
    ConfigOption *opts, int num_opts) {
    ParseStringWithOptionsContext context = {
        .opts = opts,
        .num_opts = num_opts,
        .value = g_string_new_len (0, 64)
    };
    int ret = config_parse_string ( str, _parse_string_with_options_callback,
        &context);
    g_string_free (context.value, TRUE);
    if (ret != 0) {
        goto out;
    }
    for (int i = 0; i < num_opts; ++i) {
        if (opts[i].required && !opts[i].have) {
            LOG_ERROR ("Missing required config option %s.",
                opts[i].name);
            return -1;
        }
        /* Set default value */
        if (!opts[i].have) {
            switch (opts[i].type) {
            case CONFIG_OPTION_TYPE_INTEGER: {
                *opts[i].value.integer = opts[i].default_value.integer;
                break;
            }
            case CONFIG_OPTION_TYPE_UNSIGNED_INTEGER: {
                *opts[i].value.unsigned_integer = opts[i].default_value.unsigned_integer;
                break;
            }
            case CONFIG_OPTION_TYPE_STRING: {
                *opts[i].value.string = 0;
                if (opts[i].default_value.string) {
                    *opts[i].value.string = g_strdup(opts[i].default_value.string);
                }
                break;
            }
            case CONFIG_OPTION_TYPE_BOOLEAN: {
                *opts[i].value.boolean = opts[i].default_value.boolean;
                break;
            }
            case CONFIG_OPTION_TYPE_DOUBLE: {
                *opts[i].value.d = opts[i].default_value.d;
                break;
            }
            case CONFIG_OPTION_TYPE_SIZED_STRING: {
                if (opts[i].default_value.sized_string) {
                    size_t len = strlen (opts[i].default_value.sized_string);
                    assert (len < opts[i].value.sized_string.max_len);
                    memcpy (opts[i].value.sized_string.data, opts[i].default_value.sized_string, len);
                    opts[i].value.sized_string.data[len] = 0;
                } else {
                    opts[i].value.sized_string.data[0] = 0;
                }
                break;
            }
            case CONFIG_OPTION_TYPE_KEYBIND: {
                for (int i = 0; opts[i].default_value.keybind.num_keys; i++) {
                    size_t len = strlen (opts[i].default_value.keybind.keys[i]);
                    if (_contains_reserved_str (opts[i].default_value.keybind.keys[i], len) == 0) {
                        LOG_ERROR ("Reserved string value in default config: %s.", opts[i].default_value.keybind.keys[i]);
                        ret = -1;
                        goto out;
                    }
                }
                *opts[i].value.keybind = opts[i].default_value.keybind;
                break;
            }
            }
        }
    }
out:
    if (ret != 0) {
        /* Free string variables on failure. */
        for (int i = 0; i < num_opts; ++i) {
            if (opts[i].type == CONFIG_OPTION_TYPE_STRING && opts[i].have) {
                free (*opts[i].value.string);
                *opts[i].value.string = 0;
            }
        }
    }
    return ret;
}

void config_destroy_parsed_options (ConfigOption *opts, int num_opts) {
    for (int i = 0; i < num_opts; ++i) {
        if (opts[i].type == CONFIG_OPTION_TYPE_STRING) {
            free (*opts[i].value.string);
            *opts[i].value.string = 0;
        }
        opts[i].have = 0;
    }
}

int config_init (void)
{
    char *str = NULL;
    int ret = util_file_load_to_str (config_file_path, &str);
    if (ret == 0) {
        if (config_parse_string_with_options (str, _options, _NUM_OPTIONS)) {
            ret = 1;
        }
	g_free (str);
    }
    return ret;
}

void config_destroy (void)
{
    config_destroy_parsed_options (_options, _NUM_OPTIONS);
    memset (&config, 0, sizeof (Config));
}

int config_generate_example_file (const char *file_path)
{
    FILE *file = fopen (file_path, "w");
    if (!file) {
        return -1;
    }
    int ret = 0;
    int lockret = -1;
    int fd = fileno(file);
    if (fd == -1) {
        ret = -1;
        goto out;
    }
    lockret = flock (fd, LOCK_EX);
    if (lockret != 0) {
        ret = -2;
        goto out;
    }
    if (fputs (
        "# Tip for keybindings: You can run the program with the -k flag to find out\n"
        "# what each key's name is by pressing it.\n\n" , file) == EOF) {
        ret = -3;
        goto out;
    }
    for (int i = 0; i < _NUM_OPTIONS; ++i) {
        ConfigOption *option = &_options[i];
        if (option->comment) {
            if (fputs ("# ", file) == EOF ||
            fputs (option->comment, file) == EOF ||
            fputc ('\n', file) == EOF) {
                ret = -4;
                goto out;
            }
        }
        switch (option->type) {
        case CONFIG_OPTION_TYPE_INTEGER: {
            if (fprintf (file, "#%s = %d", option->name, option->default_value.integer) == EOF) {
                ret = -5;
                goto out;
            }
            break;
        }
        case CONFIG_OPTION_TYPE_UNSIGNED_INTEGER: {
            if (fprintf (file, "#%s = %u", option->name, option->default_value.unsigned_integer) == EOF) {
                ret = -5;
                goto out;
            }
            break;
        }
        case CONFIG_OPTION_TYPE_STRING: {
            if (fprintf (file, "#%s = %s", option->name,
                option->default_value.string ? option->default_value.string : "") == EOF) {
                ret = -5;
                goto out;
            }
            break;
        }
        case CONFIG_OPTION_TYPE_BOOLEAN: {
            if (fprintf (file, "#%s = %s", option->name, option->default_value.boolean ? "true" : "false") == EOF) {
                ret = -5;
                goto out;
            }
            break;
        }
        case CONFIG_OPTION_TYPE_DOUBLE: {
            if (fprintf (file, "#%s = %f", option->name, option->default_value.d) == EOF) {
                ret = -5;
                goto out;
            }
            break;
        }
        case CONFIG_OPTION_TYPE_SIZED_STRING: {
            if (fprintf (file, "#%s = %s", option->name,
                option->default_value.sized_string ? option->default_value.sized_string : "") == EOF) {
                ret = -5;
                goto out;
            }
            break;
        }
        case CONFIG_OPTION_TYPE_KEYBIND: {
            int num = option->default_value.keybind.num_keys;
            for (int i = 0; i < num; ++i) {
                if (fprintf (file, "#%s = %s", option->name, option->default_value.keybind.keys[i]) == EOF) {
                    ret = -5;
                    goto out;
                }
                if (i < num-1 && fputc ('\n', file) == EOF) {
                    ret = -5;
                    goto out;
                }
            }
            break;
        }
        }
        if (i != _NUM_OPTIONS - 1) {
            if (fputc ('\n', file) == EOF) {
                ret = -5;
                goto out;
            }
        }
    }
out:
    if (lockret == 0) {
        if (flock (fd, LOCK_UN) != 0) {
            return -6;
        }
    }
    if (fclose (file) == EOF) {
        return -7;
    }
    return ret;
}

gboolean config_option_key_prefix_found(const char *str)
{
    size_t optlen;
    size_t len = strlen (str); // In case the key combination is yet incomplete
    if (len < 1) {
        return FALSE;
    }
    for (int i = 0; i < _NUM_OPTIONS; i++) {
        ConfigOption *option = &_options[i];
        if  (option->type == CONFIG_OPTION_TYPE_KEYBIND) {
            for (int j = 0; j < option->value.keybind->num_keys; j++) {
                optlen = strlen (option->value.keybind->keys[j]);
                if (len > optlen) {
                    continue;
                }
                if (!strncmp (str, option->value.keybind->keys[j], len)) {
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

gboolean config_option_key_in_reserved_list (int ch)
{
    for (int i = 0; i < sizeof (_option_key_reserved)/sizeof (int); i++) {
        if (ch == _option_key_reserved[i]) {
            return TRUE;
        }
    }
    return FALSE;
}

static int _contains_reserved_str (const char *val, size_t len)
{
    int ret = 0;
    for (int i = 0; i < sizeof (_option_key_reserved)/sizeof (int)-1; i++) { /* do not test invalid */
        const char *str = keyname (_option_key_reserved[i]);
        size_t str_len = strlen (str);
        if (str_len > len) {
            break;
        }
        for (int j = 0; j < len - str_len; j++) {
            if (!strncmp (str, &val[j], str_len)) {
                ret = -1;
                break;
            }
        }
    }
    return ret;
}
