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

#include <ncurses.h>
#include <dirent.h>
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <libintl.h>
#define _(String) gettext (String)

#include "ncurses-screen.h"

#include "ncurses-window-title.h"
#include "ncurses-window-info.h"
#include "ncurses-window-time.h"
#include "ncurses-window-volume-and-mode.h"
#include "ncurses-window-playlist.h"
#include "ncurses-window-filebrowser.h"
#include "ncurses-window-help.h"
#include "ncurses-window-lyrics.h"
#include "ncurses-window-command-prompt.h"
#include "ncurses-window-user-info.h"
#include "ncurses-window-error.h"

#include "ncurses-key-sequence.h"

#include "playlist.h"
#include "playlist-pls.h"
#include "song.h"
#include "player.h"
#include "inspector.h"
#include "command.h"
#include "cmdline.h"
#include "paths.h"
#include "util.h"
#include "config.h"
#include "log.h"

#define MAX_USERINFO_LEN 1024

static int _quit_callback (int argc, char **argv);
static int _add_callback (int argc, char **argv);
static int _remove_callback (int argc, char **argv);
static int _write_playlist_callback (int argc, char **argv);
static int _search_callback (int argc, char **argv);
static int _metasearch_callback (int argc, char **argv);
static int _open_filebrowser_callback (int argc, char **argv);
static int _change_directory_callback (int argc, char **argv);
static int _clear_search_callback (int argc, char **argv);
static int _filebrowser_home_callback (int argc, char **argv);
static int _help_callback (int argc, char **argv);
static int _print_working_directory_callback (int argc, char **argv);
static int _seek_callback (int argc, char **argv);
static int _volume_callback (int argc, char **argv);
#include "commands.h"

typedef enum {
    NCURSES_SCREEN_MODE_PLAYLIST,
    NCURSES_SCREEN_MODE_CMD,
    NCURSES_SCREEN_MODE_SEARCH,
    NCURSES_SCREEN_MODE_FILEBROWSER,
    NCURSES_SCREEN_MODE_HELP,
    NCURSES_SCREEN_MODE_LYRICS
} NCursesScreenMode;

static void _del_wins (void);
static gboolean _resize_screen (void);
static gboolean _resize_screen_idle (gpointer data);

static void _change_song_to_index (gint to_index, gboolean call_player_stop);
static void _play (void);
static void _stop (gboolean call_player_stop);

static void _event_mouse (MEVENT *m);
static void _event_ch (int ch, const char *keybind_name, uint32_t num_keybind_repeats,
    gboolean is_num_keybind_repeats_specified);

static gboolean _screen_update_idle (gpointer data);
static gboolean _next_song_idle (gpointer data);
static gboolean _change_tune_idle (gpointer data);
static gboolean _update_time_idle (gpointer data);

static inline void _screen_update_time (void);
static inline void _screen_update_userinfo (void);
static inline void _screen_update_cmd (void);

static gboolean _init_callbacks (void);
static void _remove_songs (GSList *remove_list, Song *original);
static void _playlist_paste (gint selection_start_index, gint selection_end_index, gint selection_min_index, gint selection_max_index);
static void _playlist_paste_before (gint selection_start_index, gint selection_end_index, gint selection_min_index, gint selection_max_index);

static void _execute_cmdline (void);

static gboolean _check_key (Keybind *keybind, const char *keybind_name);

#define MAX_CMDLINE_LEN 512
static char _tmp[ABSOLUTELY_MAX_LINE_LEN] = "--";
static const gchar *_cmdline;
static gint _cursor_pos = -1;
static Song *_current_song = NULL;
static gint _current_index; /* actual playing song index */
static gint _playlist_len = 0;

static NCursesScreenMode _mode;

/* ncurses */
static int _width;
static int _height;
static int _last_ch; /* debug */
static gint _update_time_id = 0;

static gchar _formatted_userinfo[MAX_USERINFO_LEN + 1];
static const gchar *_userinfo;
static void _inspector_status_update_func (void);
static void _player_status_update_func (PlayerMessage m, gpointer data);

static gboolean _command_changed_mode = FALSE;
static gboolean _command_changed_userinfo;

static gint _sid_tune_index = 0;

gboolean ncurses_screen_init (void)
{
    _mode = NCURSES_SCREEN_MODE_PLAYLIST;
    _current_index = -1;

    if (_init_callbacks () == FALSE) goto error;
    if (player_init (_player_status_update_func) == FALSE) goto error;
    if (inspector_init (_inspector_status_update_func) == FALSE) goto error;

    initscr ();
    set_escdelay (0);
    ncurses_colors_init ();
    raw ();
    keypad (stdscr, TRUE);
    noecho ();
    nodelay (stdscr, TRUE);
    mousemask (ALL_MOUSE_EVENTS, NULL); /* listen all mouse events */

    /* init windows with positions */
    ncurses_window_title_init ();
    ncurses_window_info_init ();
    ncurses_window_time_init ();
    ncurses_window_volume_and_mode_init ();
    ncurses_window_playlist_init ();
    ncurses_window_filebrowser_init ();
    ncurses_window_help_init ();
    ncurses_window_lyrics_init (_screen_update_idle);
    ncurses_window_user_info_init ();
    ncurses_window_error_init ();

    _last_ch = 0; /* debug */
    if (_resize_screen () == FALSE) goto error;
    g_idle_add (_screen_update_idle, NULL);

    _cmdline = "";
    cmdline_init (command_commands());
    return TRUE;
error:
    ncurses_screen_free ();
    return FALSE;
}

static gboolean _resize_screen (void)
{
    clear ();
    _del_wins ();

    getmaxyx (stdscr, _height, _width);
    if (_width > ABSOLUTELY_MAX_LINE_LEN-1) goto resize_error;

    ncurses_window_title_resize (_width, 1, 0, 0);
    ncurses_window_info_resize (_width-2*NCURSES_WINDOW_MARGIN, 3, NCURSES_WINDOW_MARGIN, 2);
    ncurses_window_time_resize (_width-2*NCURSES_WINDOW_MARGIN, 1, NCURSES_WINDOW_MARGIN, 5);
    ncurses_window_volume_and_mode_resize (_width-2*NCURSES_WINDOW_MARGIN, 1, NCURSES_WINDOW_MARGIN, 6);
    ncurses_window_playlist_resize (_width-2*NCURSES_WINDOW_MARGIN, _height - 11, NCURSES_WINDOW_MARGIN, 8);
    ncurses_window_filebrowser_resize (_width-2*NCURSES_WINDOW_MARGIN, _height - 11, NCURSES_WINDOW_MARGIN, 8);
    ncurses_window_help_resize (_width-2*NCURSES_WINDOW_MARGIN, _height - 11, NCURSES_WINDOW_MARGIN, 8);
    ncurses_window_lyrics_resize (_width-2*NCURSES_WINDOW_MARGIN, _height - 11, NCURSES_WINDOW_MARGIN, 8);
    ncurses_window_user_info_resize (_width-2*NCURSES_WINDOW_MARGIN, 1, NCURSES_WINDOW_MARGIN, _height - 2);
    ncurses_window_command_prompt_resize (_width, 1, 0, _height - 1);
    ncurses_window_error_resize (0, 1, 1, _height - 1);

    return TRUE;
resize_error:
    return FALSE;
}

static gboolean _resize_screen_idle (gpointer data)
{
    if (_resize_screen () == FALSE) (void)raise (SIGINT); /* quit if could not resize */
    g_timeout_add(100, _screen_update_idle, NULL);
    return FALSE;
}

static void _del_wins (void)
{
    ncurses_window_title_delete ();
    ncurses_window_info_delete ();
    ncurses_window_time_delete ();
    ncurses_window_volume_and_mode_delete ();
    ncurses_window_playlist_delete ();
    ncurses_window_filebrowser_delete ();
    ncurses_window_help_delete ();
    ncurses_window_lyrics_delete ();
    ncurses_window_user_info_delete ();
    ncurses_window_command_prompt_delete ();
    ncurses_window_error_delete ();
}

void ncurses_screen_free (void)
{
    inspector_free ();
    player_free ();
    _del_wins ();
    ncurses_window_filebrowser_free ();
    endwin ();
    cmdline_free ();
}

void ncurses_screen_event (NCursesEvent *e)
{
    if (e == NULL) {
        return;
    }
    const char *keybind_name = NULL;
    uint32_t num_keybind_repeats = 0;
    gboolean is_num_keybind_repeats_specified = FALSE;
    ncurses_window_error_set ("");
    if (_mode == NCURSES_SCREEN_MODE_CMD ||
        _mode == NCURSES_SCREEN_MODE_SEARCH ||
        (_mode == NCURSES_SCREEN_MODE_FILEBROWSER &&
            ncurses_window_filebrowser_cmd_mode () == FILEBROWSER_CMD_MODE_SEARCH) ||
        (_mode == NCURSES_SCREEN_MODE_FILEBROWSER &&
            ncurses_window_filebrowser_cmd_mode () == FILEBROWSER_CMD_MODE_CMD)) {
        _cmdline = cmdline_input (e);
        keybind_name = ncurses_key_sequence_add (e, &num_keybind_repeats, &is_num_keybind_repeats_specified);
    } else if (e->type == NCURSES_EVENT_TYPE_CH || e->type == NCURSES_EVENT_TYPE_UTF8) {
        keybind_name = ncurses_key_sequence_add (e, &num_keybind_repeats, &is_num_keybind_repeats_specified);
    } else {
        ncurses_key_sequence_reset ();
    }

    if (e->type == NCURSES_EVENT_TYPE_CH) {
        _event_ch (e->ch, keybind_name, num_keybind_repeats, is_num_keybind_repeats_specified);
    } else if (e->type == NCURSES_EVENT_TYPE_UTF8) {
        _event_ch (INVALID_NCURSES_CH, keybind_name, num_keybind_repeats, is_num_keybind_repeats_specified);
    } else if (e->type == NCURSES_EVENT_TYPE_MOUSE) {
        _event_mouse (&e->mevent);
    }

    if (_mode == NCURSES_SCREEN_MODE_CMD ||
        (_mode == NCURSES_SCREEN_MODE_FILEBROWSER &&
            ncurses_window_filebrowser_cmd_mode () == FILEBROWSER_CMD_MODE_CMD)) {
        _cursor_pos = cmdline_cursor_pos ();
    } else if (_mode == NCURSES_SCREEN_MODE_SEARCH) {
        int old_pos = _cursor_pos;
        _cursor_pos = cmdline_cursor_pos ();
        if (_cursor_pos != old_pos) {
            gint search_index = ncurses_window_playlist_selection_start() + 1;
            if (search_index >= playlist_length()) {
                search_index = 0;
            }
            if  (strlen (_cmdline) > 0) {
                (void)playlist_search_set (_cmdline, search_index, FALSE, FALSE);
            } else {
                playlist_search_free ();
            }
        }
    } else if (_mode == NCURSES_SCREEN_MODE_FILEBROWSER &&
        ncurses_window_filebrowser_cmd_mode () == FILEBROWSER_CMD_MODE_SEARCH) {
        int old_pos = _cursor_pos;
        _cursor_pos = cmdline_cursor_pos ();
        if (_cursor_pos != old_pos) {
            ncurses_window_filebrowser_search (_cmdline);
        }
    } else {
        _cursor_pos = -1;
    }

    g_idle_add (_screen_update_idle, NULL);
}

static void _event_mouse (MEVENT *m)
{
    if (m == NULL) return;

/*
 *  typedef struct
 *  {
 *      short id;         // device id 
 *      int x, y, z;      // coordinates
 *      mmask_t bstate;   // button state bits
 *  } MEVENT;
 */

    /* TODO: click - seek */
    /* TODO: click - volume ??? */
    /* TODO: click - single selection */
    /* TODO: doubleclick - change song ??? */
    /* TODO: pressed+released - multiselection ??? */
    if (m->bstate & BUTTON1_CLICKED) {
    } else if (m->bstate & BUTTON1_DOUBLE_CLICKED) {
    } else if (m->bstate & BUTTON1_PRESSED) {
    } else if (m->bstate & BUTTON1_RELEASED) {
    }
}

static void _event_ch (int ch, const char *keybind_name, uint32_t num_keybind_repeats,
    gboolean is_num_keybind_repeats_specified)
{
    gint selection_start_index = ncurses_window_playlist_selection_start ();
    gint selection_end_index = ncurses_window_playlist_selection_end ();
    gint selection_min_index = ncurses_window_playlist_selection_min ();
    gint selection_max_index = ncurses_window_playlist_selection_max ();
    gint list_len = playlist_length ();
    _playlist_len = list_len;

    if (ch == -1) {
        return; /* just in case */
    }

    /* common input */
    if (ch == 410) { /* resize */
        g_idle_add (_resize_screen_idle, NULL);
    } else if (_check_key (&config.key_global_volume_up, keybind_name)) {
        uint8_t num_vol_up = num_keybind_repeats > PLAYER_VOLUME_MAX ? PLAYER_VOLUME_MAX : (uint8_t)num_keybind_repeats;
        player_volume_up (num_vol_up);
    } else if (_check_key (&config.key_global_volume_down, keybind_name)) {
        uint8_t num_vol_down = num_keybind_repeats > PLAYER_VOLUME_MAX ? PLAYER_VOLUME_MAX : (uint8_t)num_keybind_repeats;
        player_volume_down (num_vol_down);
    }

    /* playlist input */
    if (_mode == NCURSES_SCREEN_MODE_PLAYLIST && ncurses_window_playlist_mode() == NCURSES_WINDOW_PLAYLIST_MODE_NORMAL) {
        if (_check_key (&config.key_common_abort, keybind_name)) {
            playlist_search_free ();
            ncurses_window_playlist_toggle_select_set (FALSE);
            ncurses_window_playlist_selections_set (selection_end_index, selection_end_index); /* to one line */
        } else if (_check_key (&config.key_seek_backward, keybind_name)) {
            gint64 num_to_seek = -((gint64)num_keybind_repeats * 1000);
            player_seek (num_to_seek);
        } else if (_check_key (&config.key_seek_forward, keybind_name)) {
            gint64 num_to_seek = (gint64)num_keybind_repeats * 1000;
            player_seek (num_to_seek);
        } else if (_check_key (&config.key_playpause_toggle, keybind_name)) { /* space */
            PlayerState state = player_toggle_playpause ();
            if (state == PLAYER_STATE_PLAYING) {
                /* todo */
            } else if (state == PLAYER_STATE_PAUSED) {
                /* todo */
            } else {
                /* todo */
            }
        } else if (_check_key (&config.key_quit, keybind_name)) {
            (void)raise (SIGINT);
        } else if (_check_key (&config.key_playlist_mode, keybind_name)) {
            for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                playlist_mode_next ();
                _current_index = playlist_get_song_index (_current_song);
            }
        } else if (_check_key (&config.key_playlist_loop_toggle, keybind_name)) {
            playlist_loop_toggle ();
        } else if (_check_key (&config.key_playlist_remove_songs, keybind_name)) {
            GSList *remove_list = g_slist_alloc ();
            for (gint i = selection_min_index; i < selection_max_index+1; i++) {
                Song *ol = playlist_get_nth_song_no_set (i);
                if (ol != NULL) {
                    remove_list = g_slist_prepend (remove_list, ol);
                }
            }
            _remove_songs (remove_list, _current_song);
            g_slist_free (remove_list);
            ncurses_window_playlist_toggle_select_set (FALSE);
        } else if (_check_key (&config.key_playlist_select, keybind_name)) {
            _change_song_to_index (selection_end_index, TRUE);
        } else if (_check_key (&config.key_command_mode, keybind_name)) {
            _mode = NCURSES_SCREEN_MODE_CMD;
            cmdline_mode_set (CMDLINE_MODE_CMD);
            cmdline_clear ();
        } else if (_check_key (&config.key_search_mode, keybind_name)) {
            _mode = NCURSES_SCREEN_MODE_SEARCH;
            cmdline_mode_set (CMDLINE_MODE_SEARCH);
            cmdline_clear ();
            ncurses_window_playlist_show_search_hilight ();
        } else if (_check_key (&config.key_edit_mode, keybind_name)) {
            ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_EDIT);
            _mode = NCURSES_SCREEN_MODE_PLAYLIST;
        } else if (_check_key (&config.key_search_next, keybind_name)) {
            for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                ncurses_window_playlist_search_next ();
            }
        } else if (_check_key (&config.key_search_previous, keybind_name)) {
            for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                ncurses_window_playlist_search_prev ();
            }
        } else if (_check_key (&config.key_playlist_copy, keybind_name)) {
            if (playlist_copy_range (selection_start_index, selection_end_index) == FALSE) {
                _userinfo = _("Copy failed.");
            } else {
                if (selection_start_index == selection_end_index) {
                    _userinfo = _("Copied one playlist item.");
                } else {
                    _userinfo = _("Copied multiple playlist items.");
                }
                ncurses_window_playlist_toggle_select_set (FALSE);
                ncurses_window_playlist_selections_set (selection_min_index, selection_min_index); /* to one line */
            }
        } else if (_check_key (&config.key_playlist_cut, keybind_name)) {
            if (playlist_cut_range (selection_start_index, selection_end_index) == FALSE) {
                _userinfo = _("Cut failed.");
            } else {
                gint new_index = selection_min_index;
                gint last_index = playlist_length() - 1;
                if (new_index > last_index) {
                    new_index = last_index;
                }
                if (new_index < 0) {
                    new_index = 0;
                }
                if (selection_start_index == selection_end_index) {
                    _userinfo = _("Cut one playlist item.");
                } else {
                    _userinfo = _("Cut multiple playlist items.");
                }
                ncurses_window_playlist_toggle_select_set (FALSE);
                ncurses_window_playlist_selections_set (new_index, new_index); /* to one line */
            }
        } else if (_check_key (&config.key_tune_next, keybind_name)) {
            for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                if (_current_song != NULL && _current_song->type == SONG_TYPE_SID && _sid_tune_index < _current_song->tunes) {
                    _sid_tune_index++;
                    g_idle_add (_change_tune_idle, NULL);
                }
            }
        } else if (_check_key (&config.key_tune_prev, keybind_name)) {
            for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                if (_current_song != NULL && _current_song->type == SONG_TYPE_SID && _sid_tune_index > 0) {
                    _sid_tune_index--;
                    g_idle_add (_change_tune_idle, NULL);
                }
            }
        } else if (_check_key (&config.key_help, keybind_name)) {
            _mode = NCURSES_SCREEN_MODE_HELP;
        } else if (_check_key (&config.key_open_lyrics, keybind_name)) {
            if (config.lyrics_service != 0) {
                _mode = NCURSES_SCREEN_MODE_LYRICS;
                if (_current_song != NULL || player_state () == PLAYER_STATE_PLAYING) {
                    ncurses_window_lyrics_fetch (_current_song->artist, _current_song->title, (NetLyricsService)config.lyrics_service );
                } else {
                    Song *ol = playlist_get_nth_song_no_set (selection_end_index);
                    ncurses_window_lyrics_fetch (ol->artist, ol->title, (NetLyricsService)config.lyrics_service );
                }
            }
        }
    /* cmd input */
    } else if (_mode == NCURSES_SCREEN_MODE_CMD) {
        if (_check_key (&config.key_common_abort, keybind_name)) {
            _mode = NCURSES_SCREEN_MODE_PLAYLIST;
            ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_NORMAL);
            g_idle_add (_screen_update_idle, NULL);
        } else if (ch == 10) {
            _execute_cmdline ();
            if (!_command_changed_mode) {
                _mode = NCURSES_SCREEN_MODE_PLAYLIST;
                ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_NORMAL);
            } else {
                _command_changed_mode = FALSE;
            }
        }
        ncurses_key_sequence_reset ();
    } else if (_mode == NCURSES_SCREEN_MODE_SEARCH) {
        if (_check_key (&config.key_common_abort, keybind_name)) {
            playlist_search_free ();
            _mode = NCURSES_SCREEN_MODE_PLAYLIST;
            ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_NORMAL);
            g_idle_add (_screen_update_idle, NULL);
        } else if (ch == 10) { /* enter */
            if (_cmdline != NULL && strlen (_cmdline) > 0) {
                g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "s %s", _cmdline);
                CommandError cmd_err = command_parse_and_run_string (_tmp);
                if (cmd_err == COMMAND_ERROR_NO_SUCH_COMMAND) {
                //  FIXME:!!
                } else if (cmd_err == COMMAND_ERROR_COMMAND_FAILED) {
                // FIXME:!!
                }
            }
            _mode = NCURSES_SCREEN_MODE_PLAYLIST;
            ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_NORMAL);
            cmdline_clear ();
            if (playlist_search_string () != NULL) {
                gint found_index = playlist_search_next ();
                if (found_index > -1) {
                    ncurses_window_playlist_selections_set (found_index, found_index);
                }
            }
        }
        ncurses_key_sequence_reset ();
    } else if (_mode == NCURSES_SCREEN_MODE_PLAYLIST && ncurses_window_playlist_mode() == NCURSES_WINDOW_PLAYLIST_MODE_EDIT) {
        if (_check_key (&config.key_common_abort, keybind_name)) {
            _mode = NCURSES_SCREEN_MODE_PLAYLIST;
            ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_NORMAL);
            ncurses_window_playlist_toggle_select_set (FALSE);
            ncurses_window_playlist_selections_set (selection_end_index, selection_end_index);
        } else if (_check_key (&config.key_playlist_select, keybind_name)) {
            playlist_toggle_select_range (selection_start_index, selection_end_index);
            ncurses_window_playlist_toggle_select_set (FALSE);
            ncurses_window_playlist_selections_set (selection_end_index, selection_end_index);
        } else if (_check_key (&config.key_playlist_remove_songs, keybind_name)) {
            GSList *remove_list = g_slist_alloc ();
            for (gint i = list_len-1; i>-1; i--) {
                Song *ol = playlist_get_nth_song_no_set (i);
                if (ol != NULL && ol->selected) {
                    remove_list = g_slist_prepend (remove_list, ol);
                }
            }
            _remove_songs (remove_list, _current_song);
            g_slist_free (remove_list);
        } else if (_check_key (&config.key_playlist_copy, keybind_name)) {
            playlist_copy_selected ();
            ncurses_window_playlist_selections_set (selection_start_index, selection_start_index); /* to one line */
        } else if (_check_key (&config.key_playlist_cut, keybind_name)) {
            playlist_cut_selected ();
            ncurses_window_playlist_selections_set (selection_start_index, selection_start_index); /* to one line */
        }
    } else if (_mode == NCURSES_SCREEN_MODE_FILEBROWSER) {
        switch (ncurses_window_filebrowser_cmd_mode ()) {
        case FILEBROWSER_CMD_MODE_NONE: {
            if (ncurses_window_filebrowser_get_select () &&
                (_check_key (&config.key_common_abort, keybind_name) || _check_key (&config.key_quit, keybind_name))) {
                ncurses_window_filebrowser_set_select_off ();
            } else if (_check_key (&config.key_common_abort, keybind_name) || _check_key (&config.key_quit, keybind_name)) {
                _mode = NCURSES_SCREEN_MODE_PLAYLIST;
                ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_NORMAL);
                g_idle_add (_screen_update_idle, NULL);
            } else if (_check_key (&config.key_command_mode, keybind_name)) {
                cmdline_mode_set (CMDLINE_MODE_FILEBROWSER);
                cmdline_clear ();
                ncurses_window_filebrowser_cmd_mode_set (FILEBROWSER_CMD_MODE_CMD);
            } else if (_check_key (&config.key_search_mode, keybind_name)) {
                cmdline_mode_set (CMDLINE_MODE_FILEBROWSER_SEARCH);
                cmdline_clear ();
                ncurses_window_filebrowser_cmd_mode_set (FILEBROWSER_CMD_MODE_SEARCH);
            } else if (_check_key (&config.key_move_up, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_up ();
                }
            } else if (_check_key (&config.key_move_down, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_down ();
                }
            } else if (_check_key (&config.key_filebrowser_add, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_add ();
                    ncurses_window_filebrowser_set_select_off ();
                }
            } else if ( _check_key (&config.key_filebrowser_change_directory, keybind_name)) {
                ncurses_window_filebrowser_change_directory_to_selected ();
            } else if (_check_key (&config.key_filebrowser_refresh, keybind_name)) {
                ncurses_window_filebrowser_refresh ();
            } else if (_check_key (&config.key_move_half_page_up, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_up_half_page ();
                }
            } else if (_check_key (&config.key_move_half_page_down, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_down_half_page ();
                }
            } else if (_check_key (&config.key_move_full_page_up, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_up_full_page ();
                }
            } else if (_check_key (&config.key_move_full_page_down, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_down_full_page ();
                }
            } else if (_check_key (&config.key_filebrowser_previous_directory, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_prev_directory ();
                }
            } else if (_check_key (&config.key_move_top, keybind_name)) {
                if (is_num_keybind_repeats_specified) {
                    if (num_keybind_repeats > 0) {
                        ncurses_window_filebrowser_jump (num_keybind_repeats - 1);
                    }
                } else {
                    ncurses_window_filebrowser_top ();
                }
            } else if (_check_key (&config.key_move_bottom, keybind_name)) {
                ncurses_window_filebrowser_bottom ();
            } else if (_check_key (&config.key_scroll_up, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_scroll_up ();
                }
            } else if (_check_key (&config.key_scroll_down, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_scroll_down ();
                }
            } else if (_check_key (&config.key_center_screen_on_cursor, keybind_name)) {
                ncurses_window_filebrowser_center_screen_on_cursor ();
            } else if (_check_key (&config.key_search_next, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_search_next ();
                }
            } else if (_check_key (&config.key_search_previous, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_search_prev ();
                }
            } else if (_check_key (&config.key_sort, keybind_name)) {
                for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                    ncurses_window_filebrowser_next_sort_mode ();
                }
            } else if (_check_key (&config.key_playlist_select_toggle, keybind_name)) {
                ncurses_window_filebrowser_toggle_select ();
            }
            break;
        }
        case FILEBROWSER_CMD_MODE_CMD: {
            if (_check_key (&config.key_common_abort, keybind_name)) {
                ncurses_window_filebrowser_cmd_mode_set (FILEBROWSER_CMD_MODE_NONE);
            } else if (ch == 10) { /* enter */
                _execute_cmdline ();
                ncurses_window_filebrowser_cmd_mode_set (FILEBROWSER_CMD_MODE_NONE);
            }
            ncurses_key_sequence_reset ();
            break;
        }
        case FILEBROWSER_CMD_MODE_SEARCH: {
            if (_check_key (&config.key_common_abort, keybind_name)) {
                ncurses_window_filebrowser_cancel_search ();
            } else if (ch == 10) { /* enter */
                ncurses_window_filebrowser_complete_search ();
            }
            ncurses_key_sequence_reset ();
            break;
        }
        }
    } else if (_mode == NCURSES_SCREEN_MODE_HELP) {
        if (_check_key (&config.key_common_abort, keybind_name)) {
            _mode = NCURSES_SCREEN_MODE_PLAYLIST;
            ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_NORMAL);
            g_idle_add (_screen_update_idle, NULL);
        } else if (_check_key (&config.key_move_up, keybind_name)) {
            ncurses_window_help_up ();
        } else if (_check_key (&config.key_move_down, keybind_name)) {
            ncurses_window_help_down ();
        } else if (_check_key (&config.key_move_half_page_down, keybind_name)) {
            ncurses_window_help_down_half_page ();
        } else if (_check_key (&config.key_move_half_page_up, keybind_name)) {
            ncurses_window_help_up_half_page ();
        } else if (_check_key (&config.key_move_full_page_down, keybind_name)) {
            ncurses_window_help_down_full_page ();
        } else if (_check_key (&config.key_move_full_page_up, keybind_name)) {
            ncurses_window_help_up_full_page ();
        }
    } else if (_mode == NCURSES_SCREEN_MODE_LYRICS) {
        if (_check_key (&config.key_common_abort, keybind_name)) {
            _mode = NCURSES_SCREEN_MODE_PLAYLIST;
            ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_NORMAL);
            g_idle_add (_screen_update_idle, NULL);
        } else if (_check_key (&config.key_move_up, keybind_name)) {
            ncurses_window_lyrics_up ();
        } else if (_check_key (&config.key_move_down, keybind_name)) {
            ncurses_window_lyrics_down ();
        } else if (_check_key (&config.key_move_half_page_down, keybind_name)) {
            ncurses_window_lyrics_down_half_page ();
        } else if (_check_key (&config.key_move_half_page_up, keybind_name)) {
            ncurses_window_lyrics_up_half_page ();
        } else if (_check_key (&config.key_move_full_page_down, keybind_name)) {
            ncurses_window_lyrics_down_full_page ();
        } else if (_check_key (&config.key_move_full_page_up, keybind_name)) {
            ncurses_window_lyrics_up_full_page ();
        }
    }

    if (_mode == NCURSES_SCREEN_MODE_PLAYLIST) {
        if (_check_key (&config.key_move_up, keybind_name)) {
            for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                ncurses_window_playlist_up ();
            }
        } else if (_check_key (&config.key_move_down, keybind_name)) {
            for (uint32_t i = 0; i < num_keybind_repeats; ++i) {
                ncurses_window_playlist_down ();
            }
        } else if (_check_key (&config.key_playlist_select_toggle, keybind_name)) {
            gboolean toggle = ncurses_window_playlist_toggle_select ();
            ncurses_window_playlist_toggle_select_set (!toggle);
        } else if (_check_key (&config.key_playlist_select_multiple_up, keybind_name)) {
            ncurses_window_playlist_toggle_select_set (FALSE);
            ncurses_window_playlist_select_multiple_up ();
        } else if (_check_key (&config.key_playlist_select_multiple_down, keybind_name)) {
            ncurses_window_playlist_toggle_select_set (FALSE);
            ncurses_window_playlist_select_multiple_down ();
        } else if (_check_key (&config.key_playlist_select_all, keybind_name)) {
            ncurses_window_playlist_select_all ();
        } else if (_check_key (&config.key_scroll_down, keybind_name)) {
            ncurses_window_playlist_scroll_down ();
        } else if (_check_key (&config.key_scroll_up, keybind_name)) {
            ncurses_window_playlist_scroll_up ();
        } else if (_check_key (&config.key_move_half_page_down, keybind_name)) {
            ncurses_window_playlist_down_half_page ();
        } else if (_check_key (&config.key_move_half_page_up, keybind_name)) {
            ncurses_window_playlist_up_half_page ();
        } else if (_check_key (&config.key_move_full_page_down, keybind_name)) {
            ncurses_window_playlist_down_full_page ();
        } else if (_check_key (&config.key_move_full_page_up, keybind_name)) {
            ncurses_window_playlist_up_full_page ();
        } else if (_check_key (&config.key_move_bottom, keybind_name)) {
            ncurses_window_playlist_bottom ();
        } else if (_check_key (&config.key_move_top, keybind_name)) {
            if (is_num_keybind_repeats_specified) {
                if (num_keybind_repeats > 0) {
                    ncurses_window_playlist_jump (num_keybind_repeats - 1);
                }
            } else {
                ncurses_window_playlist_top ();
            }
        } else if (_check_key (&config.key_center_screen_on_cursor, keybind_name)) {
            ncurses_window_playlist_scroll_center_to_cursor ();
        } else if (_check_key (&config.key_playlist_paste, keybind_name)) {
            _playlist_paste (selection_start_index, selection_end_index, selection_min_index, selection_max_index);
        } else if (_check_key (&config.key_playlist_paste_before, keybind_name)) {
            _playlist_paste_before (selection_start_index, selection_end_index, selection_min_index, selection_max_index);
        } else if (_check_key (&config.key_open_filebrowser, keybind_name)) {
            _mode = NCURSES_SCREEN_MODE_FILEBROWSER;
            ncurses_window_filebrowser_open ();
        }
    }

    _last_ch = ch;
}

static gboolean _screen_update_idle (gpointer data)
{
    ncurses_window_volume_and_mode_clear ();
    ncurses_window_filebrowser_clear();
    ncurses_window_help_clear();
    ncurses_window_lyrics_clear();

    if (_height < SCREEN_MIN_HEIGHT || _width < SCREEN_MIN_WIDTH) return FALSE;

    ncurses_window_title_update ();
    ncurses_window_info_update (_sid_tune_index);
    _screen_update_time ();
    ncurses_window_volume_and_mode_update ();
    if (_mode == NCURSES_SCREEN_MODE_FILEBROWSER) {
        ncurses_window_filebrowser_update ();
    } else if (_mode == NCURSES_SCREEN_MODE_HELP) {
        ncurses_window_help_update ();
    } else if (_mode == NCURSES_SCREEN_MODE_LYRICS) {
        ncurses_window_lyrics_update ();
    } else {
        ncurses_window_playlist_update ();
    }
    _screen_update_userinfo ();
    _screen_update_cmd ();
    ncurses_window_error_update ();
    return FALSE; /* call only once */
}

static inline void _screen_update_time (void)
{
    gint64 ms = player_get_current_time ();
    Song *o = _current_song;
    ncurses_window_time_update (o, _sid_tune_index, ms);

    /* logic to change sid tune */
    if (o != NULL && o->type == SONG_TYPE_SID) {
        if (ms > o->tune_duration[_sid_tune_index]) {
            _sid_tune_index++;
            if (_sid_tune_index > o->tunes-1) {
                g_idle_add (_next_song_idle, NULL);
            } else {
                g_idle_add (_change_tune_idle, NULL);
            }
        }
    }
    _screen_update_cmd ();
}

void _player_status_update_func (PlayerMessage m, gpointer data)
{
    switch (m) {
        case PLAYER_MESSAGE_ERROR: {
            ncurses_window_error_set ((gchar *)data);
            break;
        }
        case PLAYER_MESSAGE_EOS: {
            g_idle_add (_next_song_idle, NULL);
            break;
        }
        case PLAYER_MESSAGE_ABOUT_TO_FINISH: {
            static gboolean call_player_stop = FALSE;
            g_idle_add (_next_song_idle, &call_player_stop);
            break;
        }
        case PLAYER_MESSAGE_TAG: {
            Song *s = (Song *)data; 
            song_tags_copy (_current_song, s);
            g_idle_add (_screen_update_idle, NULL);
            break;
        }
        default:
            break;
    }
}

static void _inspector_status_update_func (void)
{
    _userinfo = inspector_status ();
    _screen_update_userinfo ();
}

static inline void _screen_update_userinfo ()
{
    CmdlineMenuMode menumode = cmdline_menu_mode ();
    if (menumode == CMDLINE_MENU_MODE_LIST) {
        GSList *options = cmdline_found_paths ();
        if (options == NULL) options = cmdline_found_commands ();
        ncurses_window_user_info_set_cmdline_options (options);
        ncurses_window_user_info_set_cmline_options_index (cmdline_selected_index ());
    } else {
        ncurses_window_user_info_set_cmdline_options (NULL);
    }
    ncurses_window_user_info_update (_userinfo);
}

static void _screen_update_cmd (void)
{
    NCursesWindowCommandPromptMode mode = NCURSES_WINDOW_COMMAND_PROMPT_MODE_NONE;
    if (_mode == NCURSES_SCREEN_MODE_CMD ||
        (_mode == NCURSES_SCREEN_MODE_FILEBROWSER && ncurses_window_filebrowser_cmd_mode () == FILEBROWSER_CMD_MODE_CMD)) {
        mode = NCURSES_WINDOW_COMMAND_PROMPT_MODE_COMMAND;
    } else if (_mode == NCURSES_SCREEN_MODE_SEARCH ||
        (_mode == NCURSES_SCREEN_MODE_FILEBROWSER && ncurses_window_filebrowser_cmd_mode () == FILEBROWSER_CMD_MODE_SEARCH)) {
        mode = NCURSES_WINDOW_COMMAND_PROMPT_MODE_SEARCH;
    }
    ncurses_window_command_prompt_update(mode, _cmdline, _cursor_pos);
}

static void _change_song_to_index (gint to_index, gboolean call_player_stop)
{
    Song *o;
    gint last_index = playlist_length () - 1;
    if (to_index < 0) to_index = 0;
    else if (to_index > last_index) to_index = last_index;

    o = playlist_get_nth_song (to_index);
    if (o == NULL) {
        _current_index = -1;
        ncurses_window_playlist_unselect_all ();
        return;
    }
    _stop (call_player_stop);
    _play ();
    _current_index = to_index;
}

static void _stop (gboolean call_player_stop)
{
    if (_update_time_id > 0) {
        g_source_remove (_update_time_id);
        _update_time_id = 0;
    }
    if (call_player_stop == TRUE) player_stop ();
    _current_song = NULL;
    _current_index = -1;
}

static void _play (void)
{
    _current_song = playlist_get_current_song ();
    if (_current_song == NULL) return;
    player_set_song (_current_song);
    if (_current_song->type == SONG_TYPE_SID) {
        _sid_tune_index = player_set_sid_tune (_sid_tune_index);
    }
    player_play ();
    if (player_play () == PLAYER_STATE_PLAYING) {
        _update_time_id = g_timeout_add (1000, _update_time_idle, NULL);
    } else {
        _update_time_id = 0;
    }

    if (config.lyrics_service != 0) {
        if (_current_song != NULL) {
            ncurses_window_lyrics_fetch (_current_song->artist, _current_song->title, (NetLyricsService)config.lyrics_service );
        }
    }
}

static gboolean _update_time_idle (gpointer data)
{
    _screen_update_time ();
    return TRUE;
}

static gboolean _next_song_idle (gpointer data)
{
    gint len = playlist_length ();
    Song *o = playlist_get_next_song ();
    gint next_index = playlist_get_song_index (o);
    gboolean call_player_stop = TRUE;
    _sid_tune_index = 0;
    if (data != NULL) {
        call_player_stop = *((gboolean *)data);
    }
    _stop (call_player_stop);
    if (next_index < len && next_index >= 0) {
        _change_song_to_index (next_index, call_player_stop);
    }
    g_idle_add (_screen_update_idle, NULL);
    return FALSE;
}

static gboolean _change_tune_idle (gpointer data)
{ 
    _stop (TRUE);
    _sid_tune_index = player_set_sid_tune (_sid_tune_index);
    _play ();
    g_idle_add (_screen_update_idle, NULL);

    return FALSE;
}



void ncurses_screen_update_force (void)
{
    g_idle_add (_screen_update_idle, NULL);
}

void ncurses_screen_set_user_info(const gchar *userinfo)
{
    _userinfo = userinfo;
}

void ncurses_screen_format_user_info (const gchar *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    g_vsnprintf (_formatted_userinfo, sizeof(_formatted_userinfo), fmt, args);
    va_end (args);
    _userinfo = _formatted_userinfo;
}

static void _remove_songs (GSList *remove_list, Song *original)
{
    PlayerState state;
    gint min_index = ncurses_window_playlist_selection_min ();
    gint list_last_index;
    Song *current = playlist_remove_list (remove_list);

    if (current != original) {
        state = player_state ();
        _stop (TRUE);
        if (state == PLAYER_STATE_PLAYING) _play ();
    }
    _current_index = playlist_get_song_index (current);
    list_last_index = playlist_length () - 1;

    if (list_last_index < min_index) {
        min_index = list_last_index;
    }
    if (min_index < 0) {
        min_index = 0;
    }
    ncurses_window_playlist_selections_set (min_index, min_index); /* to one line */

    ncurses_screen_update_force ();
}

static void _playlist_paste (gint selection_start_index, gint selection_end_index, gint selection_min_index, gint selection_max_index)
{
    gint num_to_paste = playlist_num_to_paste();
    gint where_to_paste = selection_min_index + 1;
    if (num_to_paste < 1) return;
    if (selection_min_index != selection_max_index) {
        GSList *remove_list = g_slist_alloc ();
        for (gint i = selection_min_index; i <= selection_max_index; i++) {
            Song *ol = playlist_get_nth_song_no_set (i);
            if (ol != NULL) {
                remove_list = g_slist_prepend (remove_list, ol);
            }
        }
        _remove_songs (remove_list, _current_song);
        g_slist_free (remove_list);
        where_to_paste = selection_min_index;
    }
    if (playlist_paste_to (where_to_paste) == FALSE) {
        ncurses_window_error_set(_("Paste after failed."));
    } else {
        if (num_to_paste == 1) {
            _userinfo = _("Pasted one playlist item.");
        } else if (num_to_paste > 1) {
            _userinfo = _("Pasted multiple playlist items.");
        } else {
            _userinfo = NULL; /* FIXME: What do with this ??? */
        }
    }
    ncurses_window_playlist_selections_set (where_to_paste, where_to_paste); /* to one line */
}

static void _playlist_paste_before (gint selection_start_index, gint selection_end_index, gint selection_min_index, gint selection_max_index)
{
    gint num_to_paste = playlist_num_to_paste();
    gint where_to_paste = selection_min_index;
    if (num_to_paste < 1) return;
    if (selection_start_index != selection_end_index) {
        GSList *remove_list = g_slist_alloc ();
        for (gint i = selection_min_index; i <= selection_max_index; i++) {
            Song *ol = playlist_get_nth_song_no_set (i);
            if (ol != NULL) {
                remove_list = g_slist_prepend (remove_list, ol);
            }
        }
        _remove_songs (remove_list, _current_song);
        g_slist_free (remove_list);
    }
    if (playlist_paste_to (where_to_paste) == FALSE) {
        ncurses_window_error_set (_("Paste before failed."));
    } else {
        if (num_to_paste == 1) {
            _userinfo = _("Pasted one playlist item.");
        } else if (num_to_paste > 1) {
            _userinfo = _("Pasted multiple playlist items.");
        } else {
            _userinfo = NULL;
        }
    }
    ncurses_window_playlist_selections_set (selection_min_index-num_to_paste+1, selection_min_index-num_to_paste+1); /* to one line */
}

/* commands */
static gboolean _init_callbacks (void)
{
   if (command_register (&quit_command) != 0) {
       return FALSE;
   }
   if (command_register (&add_command) != 0) {
       return FALSE;
   }
   if (command_register (&remove_command) != 0) {
       return FALSE;
   }
   if (command_register (&write_playlist_command) != 0) {
       return FALSE;
   }
   if (command_register (&search_command) != 0) {
       return FALSE;
   }
   if (command_register (&metasearch_command) != 0) {
       return FALSE;
   }
   if (command_register(&open_filebrowser_command)) {
       return FALSE;
   }
   if (command_register(&change_directory_command)) {
       return FALSE;
   }
   if (command_register(&clear_search_command)) {
       return FALSE;
   }
   if (command_register(&filebrowser_home_command)) {
       return FALSE;
   }
   if (command_register(&help_command)) {
       return FALSE;
   }
   if (command_register(&print_working_directory_command)) {
       return FALSE;
   }
   if (command_register(&seek_command)) {
       return FALSE;
   }
   if (command_register(&volume_command)) {
       return FALSE;
   }
   return TRUE;
}

static int _quit_callback (int argc, char **argv)
{
    if (_mode == NCURSES_SCREEN_MODE_FILEBROWSER) {
        ncurses_window_filebrowser_cmd_mode_set (FILEBROWSER_CMD_MODE_CMD);
        _mode = NCURSES_SCREEN_MODE_PLAYLIST;
        ncurses_window_playlist_mode_set (NCURSES_WINDOW_PLAYLIST_MODE_NORMAL);
    } else {
        (void)raise (SIGINT);
    }
    return 0;
}

static int _add_callback (int argc, char **argv)
{
    int i;
    int ret = 0;
    if (argc < 2) {
        ncurses_window_error_set (_("Error: Add. Wrong number of arguments."));
        return -1;
    }
    for (i = 1; i < argc; i++) {
        gboolean bret = playlist_add (argv[i]);
        if (bret == FALSE) ret = -1;
    }
    if (ret != 0) {
        ncurses_window_error_set (_("Error: Remove. Could not add all items."));
    }
    return ret;
}

static int _remove_callback (int argc, char **argv)
{
    gchar **split;
    gint64 first = 0;
    gint64 last = 0;
    Song *original;
    int i,j;
    if (argc < 2) {
        ncurses_window_error_set (_("Error: Remove. Wrong number of arguments."));
        return -1;
    }
    for (i = 1; i < argc; i++) {
        int ret = 0;
        if (argv[i][0] == '-') {
            ret = -1;
        } else if (argv[i][strlen(argv[i])-1] == '-') {
            ret = -1;
        }
        for (j=0; j<strlen(argv[i]); j++) {
            if (argv[i][j] < '0' && argv[i][j] > '9' && argv[i][j] != '-') {
                ret = -1;
            }
        }
        if (ret != 0) {
            ncurses_window_error_set (_("Error: Remove. Invalid arguments."));
            return ret;
       }
    }
    original = _current_song;
    GSList *remove_list = g_slist_alloc ();
    for (i = 1; i < argc; i++) {
        split = g_strsplit (argv[i], "-", 0);
        first = g_ascii_strtoll (split[0], NULL, 10); // assume g_str_split() returns NULL terminated strings
        if (first <= 0) {
            ncurses_window_error_set (_("Error: Remove. Invalid arguments."));
            return -1;
        }
        if (split[1] == NULL) last = first;
        else last = g_ascii_strtoll (split[1], NULL, 10); // assume g_str_split() returns NULL terminated strings
        if (last <= 0) {
            ncurses_window_error_set (_("Error: Remove. Invalid arguments."));
            return -1;
        }
        for (gint j = first; j <= last; j++) {
            Song *ol = playlist_get_nth_song_no_set (j-1);
            if (ol != NULL) {
                remove_list = g_slist_prepend (remove_list, ol);
            }
        }
        g_strfreev (split);
    }
    _remove_songs (remove_list, original);
    g_slist_free (remove_list);
    return 0;
}

static int _write_playlist_callback (int argc, char **argv)
{
    static char saved_playlist[PATH_MAX] = "";
    gchar *playlistfile = _("default");
    gboolean success = FALSE;
    if (argc > 2) {
        ncurses_window_error_set (_("Error: Write playlist. Wrong number of arguments."));
        return -1;
    }
    if (argc == 1) {
        gchar *pl = paths_saved_data_default_playlist ();
        if (pl != NULL) {
            success = playlist_pls_save (playlist_get (), pl);
            memcpy(saved_playlist, pl, strlen(pl)+1);
            g_free (pl);
        }
    } else {
        if (strlen (argv[1]) < 1 || strlen (argv[1]) >= PATH_MAX) {
            ncurses_window_error_set (_("Error: Write playlist. Invalid arguments."));
            return -1;
        }
        success = playlist_pls_save (playlist_get (), argv[1]);
        playlistfile = argv[1];
        memcpy(saved_playlist, argv[1], strlen(argv[1])+1);
    }
    if (success == FALSE) {
        ncurses_window_error_set (_("Error: Write playlist. Save to '%s' file failed."), playlistfile);
        return -1;
    }
    _command_changed_userinfo = TRUE;
    _userinfo = saved_playlist;
    return 0;
}

static int _search_callback (int argc, char **argv)
{
    if (argc == 1) {
        return 0;
    }
    if (argv[1][0] == '\0') {
        return 0;
    }
    if (_mode != NCURSES_SCREEN_MODE_FILEBROWSER) {
        gint found_index = -1;
        gint search_index = -1;
        gchar str[ABSOLUTELY_MAX_LINE_LEN] = "";
        gchar str1[ABSOLUTELY_MAX_LINE_LEN] = "";
        gint len = playlist_length ();
        gint index = ncurses_window_playlist_selection_start ();
        if (argc < 2) {
            ncurses_window_error_set (_("Error: Search. Wrong number of arguments."));
            return -1;
        }

        for (int i = 1; i < argc; i++) {
            if (i == 1) g_snprintf (str, ABSOLUTELY_MAX_LINE_LEN, "%s", argv[i]);
            else g_snprintf (str, ABSOLUTELY_MAX_LINE_LEN, "%s %s", str1, argv[i]);
            g_snprintf (str1, ABSOLUTELY_MAX_LINE_LEN, "%s", str);
        }

        search_index = index + 1;
        if (search_index >= len) search_index = 0;

        found_index = playlist_search_set (str, search_index, FALSE, FALSE);
        if (found_index > -1) {
            ncurses_window_playlist_selections_set (found_index, found_index); /* to one line */
        } else {
            ncurses_window_error_set (_("Error: Search. Could not find pattern '%s'."), str);
        }
    }
    return 0;
}

static int _metasearch_callback (int argc, char **argv)
{
    if (argc == 1) {
        return 0;
    }
    if (argv[1][0] == '\0') {
        return 0;
    }
    if (_mode != NCURSES_SCREEN_MODE_FILEBROWSER) {
        gint found_index = -1;
        gint search_index = -1;
        gchar str[ABSOLUTELY_MAX_LINE_LEN] = "";
        gchar str1[ABSOLUTELY_MAX_LINE_LEN] = "";
        gint len = playlist_length ();
        gint index = ncurses_window_playlist_selection_start ();
        if (argc < 2) {
            ncurses_window_error_set (_("Error: Metasearch. Wrong number of arguments."));
            return -1;
        }

        for (int i = 1; i < argc; i++) {
            if (i == 1) g_snprintf (str, ABSOLUTELY_MAX_LINE_LEN, "%s", argv[i]);
            else g_snprintf (str, ABSOLUTELY_MAX_LINE_LEN, "%s %s", str1, argv[i]);
            g_snprintf (str1, ABSOLUTELY_MAX_LINE_LEN, "%s", str);
        }

        search_index = index + 1;
        if (search_index >= len) search_index = 0;

        found_index = playlist_search_set (str, search_index, FALSE, TRUE);
        if (found_index > -1) {
            ncurses_window_playlist_selections_set (found_index, found_index); /* to one line */
        } else {
            ncurses_window_error_set (_("Error: Metasearch. Could not find pattern '%s'."), str);
        }
    }
    return 0;
}

static int _open_filebrowser_callback (int argc, char **argv)
{
    if (argc != 1) {
        ncurses_window_error_set (_("Error: Filebrowser. Wrong number of arguments."));
        return -1;
    }
    if (ncurses_window_filebrowser_open () != 0) {
        ncurses_window_error_set (_("Error: Filebrowser. Could not open filebrowser"));
        return -1;
    }
    _command_changed_mode = TRUE;
    _mode = NCURSES_SCREEN_MODE_FILEBROWSER;
    return 0;
}

static int _change_directory_callback (int argc, char **argv)
{
    char *pwd[1] = {"pwd"};
    if (argc == 1) {
        const char *dir = config.default_music_directory;
        if (_mode == NCURSES_SCREEN_MODE_FILEBROWSER) {
            if (ncurses_window_filebrowser_change_directory (dir)) {
                ncurses_window_error_set (_("Error: Cd. Could not change directory to '%s'."),dir);
                return -1;
            }
        }
        if (util_chdir (dir) == FALSE) {
            ncurses_window_error_set (_("Error: Cd. Could not change directory to '%s'."),dir);
            return -1;
        }
        return _print_working_directory_callback (1, pwd);
    }
    if (argc != 2) {
        ncurses_window_error_set (_("Error: Cd. Wrong number of arguments."));
        return -1;
    }
    if (_mode == NCURSES_SCREEN_MODE_FILEBROWSER) {
        if (ncurses_window_filebrowser_change_directory (argv[1])) {
            ncurses_window_error_set (_("Error: Cd. Could not change directory to '%s'."), argv[1]);
            return -1;
        }
    }
    if (util_chdir (argv[1]) == FALSE) {
        ncurses_window_error_set (_("Error: Cd. Could not change directory to '%s'."), argv[1]);
        return -1;
    }
    return _print_working_directory_callback (1, pwd);
}

static int _clear_search_callback (int argc, char **argv)
{
    if (argc != 1) {
        ncurses_window_error_set (_("Error: Clear search. Wrong number of arguments."));
        return -1;
    }
    if (_mode != NCURSES_SCREEN_MODE_FILEBROWSER) {
        ncurses_window_playlist_clear_search ();
    } else {
        ncurses_window_filebrowser_clear_search ();
    }
    return 0;
}

static int _filebrowser_home_callback (int argc, char **argv)
{
    if (argc != 1) {
        ncurses_window_error_set (_("Error: Home. Wrong number of arguments."));
        return -1;
    }
    if (ncurses_window_filebrowser_change_directory (config.default_music_directory)) {
        ncurses_window_error_set (_("Error: Home. Could not change directory to '%s'."), config.default_music_directory);
        return -1;
    }
    return 0;
}

static int _help_callback (int argc, char **argv)
{
    if (argc != 1) {
        ncurses_window_error_set (_("Error: Help. Wrong number of arguments."));
        return -1;
    }
    _command_changed_mode = TRUE;
    _mode = NCURSES_SCREEN_MODE_HELP;
    return 0;
}

static int _print_working_directory_callback (int argc, char **argv)
{
    if (argc != 1) {
        ncurses_window_error_set (_("Error: Pwd. Wrong number of arguments."));
        return -1;
    }
    if (_mode == NCURSES_SCREEN_MODE_FILEBROWSER) {
       _userinfo = ncurses_window_filebrowser_get_current_directory ();
    } else {
        _userinfo = util_pwd ();
    }
    _command_changed_userinfo = TRUE;
    return 0;
}

static int _seek_callback (int argc, char **argv)
{
    if (argc != 2) {
        ncurses_window_error_set (_("Error: Seek. Wrong number of arguments."));
        return -1;
    }
    int colon_count = 0;
    gboolean is_percentage = FALSE;
    for (const gchar *c = argv[1]; *c; c = g_utf8_find_next_char (c, NULL)) {
        int ret = 0;
        if (is_percentage) {
            ret = -1;
        }
        switch (*c) {
        case ':': {
            colon_count++;
            if (colon_count > 2) {
                ret = -1;
            }
            break;
        }
        case '%': {
            if (colon_count > 0) {
                ret = -1;
            }
            is_percentage = TRUE;
            break;
        }
        default: {
            if (!isdigit(*c)) {
                ret = -1;
            }
            break;
        }
        }
        if (ret != 0) {
            ncurses_window_error_set (_("Error: Seek. Invalid arguments."));
            return ret;
        }
    }
    gint64 ms;
    gint64 hour = 0;
    gint64 min = 0;
    gint64 sec = 0;
    gboolean have_hour = FALSE;
    gboolean have_min = FALSE;
    gboolean have_sec = FALSE;
    double percentage;
    if (is_percentage) {
        if (sscanf(argv[1], "%lf%%", &percentage) != 1) {
            return -1;
        }
    } else if (sscanf(argv[1], "%" PRIu64 ":%" PRIu64 ":%" PRIu64, &hour, &min, &sec) == 3) {
        have_hour = TRUE;
        have_min = TRUE;
        have_sec = TRUE;
    } else if (sscanf(argv[1], "%" PRIu64 ":%" PRIu64, &min, &sec) == 2) {
        have_min = TRUE;
        have_sec = TRUE;
    } else if (sscanf(argv[1], "%" PRIu64, &sec) == 1) {
        have_sec = TRUE;
    } else {
        ncurses_window_error_set (_("Error: Seek. Invalid arguments."));
        return -1;
    }
    gint64 duration = 0;
    if (!player_get_duration (&duration)) {
        ncurses_window_error_set (_("Error: Seek. Could not get duration."));
        return -1;
    }
    if (have_sec) {
        if (sec < 0) {
            ncurses_window_error_set (_("Error: Seek. Invalid arguments."));
            return -1;
        }
        ms = sec * 1000;
        if (have_min) {
            if (min < 0) {
                ncurses_window_error_set (_("Error: Seek. Invalid arguments."));
                return -1;
            }
            ms += min * 60 * 1000;
            if (have_hour) {
                if (hour < 0) {
                    ncurses_window_error_set (_("Error: Seek. Invalid arguments."));
                    return -1;
                }
                ms += hour * 60 * 60 * 1000;
            }
        }
        if (ms > duration) {
            ncurses_window_error_set (_("Error: Seek. Invalid seek position."));
            return -1;
        }
    } else { /* Percentage */
        if (percentage < 0.0 || percentage > 100.0) {
            ncurses_window_error_set (_("Error: Seek. Invalid percentage."));
            return -1;
        }
        ms = (gint64)(percentage / 100.0 * (double)duration);
    }
    if (!player_seek_to(ms)) {
        ncurses_window_error_set (_("Error: Seek. Could not seek"));
        return -1;
    }
    return 0;
}

static int _volume_callback (int argc, char **argv)
{
    if (argc != 2) {
        ncurses_window_error_set (_("Error: Volume. Wrong number of arguments."));
        return -1;
    }
    gint64 volume = g_ascii_strtoll (argv[1], NULL, 10);
    if (volume > PLAYER_VOLUME_MAX || volume < PLAYER_VOLUME_MIN) {
        ncurses_window_error_set (_("Error: Volume. Out of range."));
        return -1;
    }
    (void)player_set_volume ((guint)volume);
    return 0;
}

static void _execute_cmdline (void)
{
    _command_changed_userinfo = FALSE;
    CommandError cmd_err = command_parse_and_run_string (_cmdline);
    if (cmd_err == COMMAND_ERROR_NO_SUCH_COMMAND) {
       ncurses_window_error_set (_("No such command."));
    } else if (cmd_err == COMMAND_ERROR_COMMAND_FAILED) {
       //ncurses_window_error_set (_("Command failed. Possibly invalid arguments."));
    } else {
        if (!_command_changed_userinfo) {
            _userinfo = NULL;
        }
    }
    cmdline_clear ();
}

static gboolean _check_key (Keybind *keybind, const char *keybind_name)
{
    if (keybind_name != NULL /* Only digits in buffer. */ ) {
        int num_keybind_keys = keybind->num_keys;
        for (int i = 0; i < num_keybind_keys; ++i) {
            int match = !strcmp (keybind->keys[i], keybind_name);
            if (match != 0) {
                ncurses_key_sequence_reset ();
                return TRUE;
            }
        }
    }
    return FALSE;
}
