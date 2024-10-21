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
#include <glib.h>
#include <libintl.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <errno.h>
#define _(String) gettext (String)

#include "ncurses-window-filebrowser.h"
#include "ncurses-colors.h"
#include "ncurses-key-sequence.h"

#include "playlist.h"
#include "cmdline.h"
#include "util.h"
#include "config.h"
#include "log.h"
#include "ncurses-screen.h"
#include "ncurses-scroller.h"

#define NUM_TOP_FILEBROWSER_ELEMENTS 2
#define NUM_BOTTOM_FILEBROWSER_ELEMENTS 1

typedef enum
{
    SORT_MODE_NAME = 0,
    NUM_SORT_MODES
} SortMode;

typedef struct {
    int highlight_begin;
    int highlight_end;
    uint8_t flags;
    char name[256];
    uint32_t hilight[8]; /* Bit array */
} FileBrowserEntry;

static inline void _draw_filebrowser_line (int y, int x, int max_x,
    FileBrowserEntry *entry, int color, int search_match_color);
static int _compute_page_height (void);
static int _compute_width (void);
static inline uint32_t _get_entry_char_hilight (uint32_t hilight[8],
    unsigned int index);
static inline void _set_entry_char_hilight_on (uint32_t hilight[8],
    unsigned int index);
static void _set_selected_index (int index);
static int _sort_mode_name_callback (const void *a, const void *b);
static void _sort (gboolean print_user_info);
static void _update_last_directory_change_time (void);

static FileBrowserCmdMode _cmd_mode = FILEBROWSER_CMD_MODE_NONE;
static int _width; /* ncurses */
static int _height; /* ncurses */
static WINDOW *_win = NULL;
static SortMode _sort_mode;
static NCursesScroller _scroller;
static gboolean _show_search_hilight;
static struct timespec _last_directory_change_time;

static struct {
    FileBrowserEntry *data;
    int num;
    int max;
} _entries;

static char _current_directory[PATH_MAX];

// Return to these values if search cancelled
static int _page_start_index_before_search;
static int _page_end_index_before_search;
static int _selection_start_index_before_search;
static int _selection_end_index_before_search;

gboolean ncurses_window_filebrowser_init (void)
{
    int max = config.max_filebrowser_entries;
    if (max < 2) { /* Room for . and .. */
        max = 2;
    }
    _entries.data = malloc (max * sizeof (FileBrowserEntry));
    if (!_entries.data) {
        LOG_ERROR ("Failed to allocate file browser entries. "
            "Try reducing the maximum number for "
            "max_filebrowser_entries in config.");
        return FALSE;
    }
    _entries.num = 0;
    _entries.max = max;
    _sort_mode = SORT_MODE_NAME;
    ncurses_scroller_init (&_scroller, _compute_page_height (), 0);
    if (realpath (".", _current_directory) == NULL) {
        LOG_ERROR ("realpath() failed to resolve path for '.': %s.",
            strerror (errno));
        return FALSE;
    }
    if (ncurses_window_filebrowser_fill_entries () != 0) {
        return FALSE;
    }
    ncurses_scroller_top (&_scroller);
    ncurses_scroller_selection_start_and_end (&_scroller, 0, 0);
    ncurses_scroller_selection_toggle (&_scroller, FALSE);
    _show_search_hilight = FALSE;
    struct stat stat_buf;
    if (stat (_current_directory, &stat_buf) == -1) {
        LOG_ERROR ("stat() failed for directory '%s'.: %s.",
            _current_directory, strerror (errno));
        return FALSE;
    }
    _last_directory_change_time = stat_buf.st_mtim;
    return TRUE;
}

gboolean ncurses_window_filebrowser_resize (gint width, gint height, gint x,
    gint y)
{
    _width = width;
    _height = height;
    if (_win != NULL) {
        ncurses_window_filebrowser_delete ();
    }
    _win = newwin (_height, _width, y, x);
    if (_win == NULL) {
        goto resize_error;
    }
    ncurses_scroller_resize(&_scroller, _compute_page_height ());
    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_filebrowser_delete (void)
{
    if (_win != NULL) {
        delwin (_win);
    }
    _win = NULL;
}

void ncurses_window_filebrowser_clear (void)
{
    wclear (_win);
}

void ncurses_window_filebrowser_update (void)
{
    ncurses_scroller_ensure_page_start_index (&_scroller);
    int page_h = _scroller.page_height;
    int first = _scroller.page_start_index;
    int last = _entries.num;
    int y = NUM_TOP_FILEBROWSER_ELEMENTS;
    int max_y = y + page_h;
    int selection_start_index;
    int selection_end_index;
    if (_scroller.selection_start_index <= _scroller.selection_end_index ) {
        selection_start_index = _scroller.selection_start_index;
        selection_end_index = _scroller.selection_end_index;
    } else {
        selection_start_index = _scroller.selection_end_index;
        selection_end_index = _scroller.selection_start_index;
    }
    int num_entries = _entries.num;
    int num_line_number_digits;
    if (num_entries > 999999999) {
        num_line_number_digits = 10;
    } else if (num_entries > 99999999) {
        num_line_number_digits = 9;
    } else if (num_entries > 9999999) {
        num_line_number_digits = 8;
    } else if (num_entries > 999999) {
        num_line_number_digits = 7;
    } else if (num_entries > 99999) {
        num_line_number_digits = 6;
    } else if (num_entries > 9999) {
        num_line_number_digits = 5;
    } else if (num_entries > 999) {
        num_line_number_digits = 4;
    } else if (num_entries > 99) {
        num_line_number_digits = 3;
    } else {
        num_line_number_digits = 2;
    }
    assert(num_line_number_digits <= KEY_SEQUENCE_MAX_DIGITS);
    int text_max_w = _compute_width () - num_line_number_digits - 2;
    if (text_max_w < 0) {
        text_max_w = 0;
    }
    FileBrowserEntry *entries = _entries.data;
    ncurses_colors_pair_set (_win, COLOR_PAIR_WHITE_BLACK);
    mvwprintw (_win, 0, 1, _("Filebrowser"));
    for (int i = first; i < last; ++i) {
        if (y >= max_y) {
            break;
        }
        char line_number_str[12];
        snprintf(line_number_str, sizeof(line_number_str), "%" PRIi32 "",
            i + 1);
        int color;
        int search_match_color;
        if (i < selection_start_index || i > selection_end_index) {
            if (!(entries[i].flags & FILEBROWSER_ENTRY_FLAG_IS_DIRECTORY)) {
                color = COLOR_PAIR_WHITE_BLACK;
                search_match_color = COLOR_PAIR_BLACK_YELLOW;
            } else {
                color = COLOR_PAIR_LIGHTGREY_BLACK;
                search_match_color = COLOR_PAIR_BLACK_YELLOW;
            }
            ncurses_colors_pair_set (_win, COLOR_PAIR_GREY_BLACK);
            mvwprintw (_win, y, 0, "%*.*s ", num_line_number_digits,
                num_line_number_digits, line_number_str);
            _draw_filebrowser_line (y, num_line_number_digits + 1, text_max_w,
                &entries[i], color, search_match_color);
        } else {
            color = COLOR_PAIR_BLACK_WHITE;
            search_match_color = COLOR_PAIR_BLACK_YELLOW;
            ncurses_colors_pair_set (_win, COLOR_PAIR_GREY_BLACK);
            mvwprintw (_win, y, 0, "%*.*s ", num_line_number_digits,
                num_line_number_digits, line_number_str);
            _draw_filebrowser_line (y, num_line_number_digits + 1, text_max_w,
                &entries[i], color, search_match_color);
            /* Pad */
            ncurses_colors_pair_set (_win, COLOR_PAIR_BLACK_WHITE);
            int len = g_utf8_strlen (entries[i].name, -1);
            wmove(_win, y, num_line_number_digits + 1 + len);
            for (int i = len; i <= text_max_w; ++i) {
                waddch (_win, ' ');
            }
        }
        y++;
    }
    ncurses_colors_pair_set (_win, COLOR_PAIR_RED_BLACK);
    int numbers = 1;
    int test = _entries.num;
    while ((test = test / 10) > 10) {
        numbers++;
    }
    if (_scroller.selection_start_index == _scroller.selection_end_index) {
        mvwprintw (_win, _height-1, 0, "%0*d/%0*d%-*s",
            numbers, _scroller.selection_start_index + 1,
            numbers, _entries.num,
            _width-numbers-numbers-2, "");
    } else {
        // Multiple selected
        mvwprintw (_win, _height-1, 0, "%0*d-%0*d/%0*d%-*s",
            numbers, selection_start_index + 1,
            numbers, selection_end_index + 1,
            numbers, _entries.num, _width-numbers-numbers-2, "");
    }
    wrefresh (_win);
}

void ncurses_window_filebrowser_free (void)
{
    ncurses_window_filebrowser_delete ();
    free (_entries.data);
    _entries.data = NULL;
}

FileBrowserCmdMode ncurses_window_filebrowser_cmd_mode (void)
{
    return _cmd_mode;
}

void ncurses_window_filebrowser_cmd_mode_set (FileBrowserCmdMode mode)
{
    if (mode == FILEBROWSER_CMD_MODE_SEARCH) {
        _page_start_index_before_search = _scroller.page_start_index;
        _page_end_index_before_search = _scroller.page_end_index;
        _selection_start_index_before_search = _scroller.selection_start_index;
        _selection_end_index_before_search = _scroller.selection_end_index;
    };
    _cmd_mode = mode;
}

void ncurses_window_filebrowser_clear_search (void)
{
    _show_search_hilight = FALSE;
}

void ncurses_window_filebrowser_up (void)
{
    ncurses_scroller_up (&_scroller);
}

void ncurses_window_filebrowser_down (void)
{
    ncurses_scroller_down (&_scroller);
}

void ncurses_window_filebrowser_add (void)
{
    if (!_entries.num) {
        return;
    }
    int start;
    int end;
    if (_scroller.selection_start_index <= _scroller.selection_end_index) {
        start = _scroller.selection_start_index;
        end = _scroller.selection_end_index;
    } else {
        start = _scroller.selection_end_index;
        end = _scroller.selection_start_index;
    }
    for (int i = start; i <= end; ++i) {
        const char *file_name = _entries.data[i].name;
        if (!strcmp (file_name, ".") || !strcmp (file_name, "..")) {
            return;
        }
        size_t current_directory_len = strlen (_current_directory);
        size_t file_name_len = strlen (file_name);
        size_t total_len = current_directory_len + file_name_len + 1;
        if (total_len > PATH_MAX - 1) {
            LOG_ERROR ("Cannot open file %s with filebrowser: path too long.",
                file_name);
            return;
        }
        char path[PATH_MAX];
        memcpy (path, _current_directory, current_directory_len);
        path[current_directory_len] = '/';
        memcpy (path + current_directory_len + 1, file_name, file_name_len);
        path[total_len] = 0;
        (void)playlist_add (path);
    }
    if (_scroller.selection_start_index != _scroller.selection_end_index) {
        ncurses_scroller_selection_start_and_end (&_scroller, start, start);
    }
}

void ncurses_window_filebrowser_up_half_page (void)
{
    ncurses_scroller_scroll_up_half_page (&_scroller);
}

void ncurses_window_filebrowser_down_half_page (void)
{
    ncurses_scroller_down_half_page (&_scroller);
}

void ncurses_window_filebrowser_up_full_page (void)
{
    ncurses_scroller_up_full_page (&_scroller);
}

void ncurses_window_filebrowser_down_full_page (void)
{
    ncurses_scroller_down_full_page (&_scroller);
}

void ncurses_window_filebrowser_prev_directory (void)
{
    size_t current_path_len = strlen (_current_directory);
    char buf[PATH_MAX];
    if (current_path_len + 3 + 1 > sizeof (buf)) {
        return;
    }
    memcpy (buf, _current_directory, current_path_len);
    strcpy (buf + current_path_len, "/..");
    if (realpath (buf, _current_directory) == NULL) {
        return;
    }
    if (ncurses_window_filebrowser_fill_entries () != 0) {
        memcpy (_current_directory, buf, current_path_len);
        _current_directory[current_path_len] = 0;
    } else {
        ncurses_scroller_top (&_scroller);
    }
}

void ncurses_window_filebrowser_top (void)
{
    ncurses_scroller_top (&_scroller);
}

void ncurses_window_filebrowser_bottom (void)
{
    ncurses_scroller_bottom (&_scroller);
}

void ncurses_window_filebrowser_scroll_up (void)
{
    ncurses_scroller_scroll_up (&_scroller);
}

void ncurses_window_filebrowser_scroll_down (void)
{
    ncurses_scroller_scroll_down (&_scroller);
}

void ncurses_window_filebrowser_center_screen_on_cursor (void)
{
    ncurses_scroller_scroll_center_to_cursor (&_scroller);
}

void ncurses_window_filebrowser_search_next (void)
{
    int num_entries = _entries.num;
    assert (num_entries > 0);
    FileBrowserEntry *entries = _entries.data;
    assert (_scroller.selection_start_index == _scroller.selection_end_index);
    int selected_index = _scroller.selection_start_index;
    for (int i = (selected_index + 1) % num_entries;
        i != selected_index;
        i = (i + 1) % num_entries) {
        if (entries[i].flags & FILEBROWSER_ENTRY_FLAG_SEARCH_MATCH) {
            _set_selected_index (i);
            break;
        }
    }
    _show_search_hilight = TRUE;
}

void ncurses_window_filebrowser_search_prev (void)
{
    int num_entries = _entries.num;
    assert (num_entries > 0);
    FileBrowserEntry *entries = _entries.data;
    assert (_scroller.selection_start_index == _scroller.selection_end_index);
    int selected_index = _scroller.selection_start_index;
    for (int i = selected_index ? selected_index - 1 : num_entries - 1;
        i != selected_index;
        i = i ? i - 1 : num_entries - 1) {
        if (entries[i].flags & FILEBROWSER_ENTRY_FLAG_SEARCH_MATCH) {
            _set_selected_index (i);
            break;
        }
    }
    _show_search_hilight = TRUE;
}

void ncurses_window_filebrowser_change_directory_to_selected (void)
{
    if (_entries.num < 1) {
        return;
    }
    if (_scroller.selection_start_index != _scroller.selection_end_index) {
        return;
    }
    char buf[PATH_MAX];
    const char *filename = _entries.data[_scroller.selection_start_index].name;
    size_t filename_len = strlen (filename);
    size_t current_path_len = strlen (_current_directory);
    size_t total_len = current_path_len + filename_len + 1 /* 1 for / */;
    if (total_len > sizeof (buf) - 1) {
        return;
    }
    memcpy (buf, _current_directory, current_path_len);
    buf[current_path_len] = '/';
    memcpy (buf + current_path_len + 1, filename, filename_len);
    buf[total_len] = 0;
    if (realpath (buf, _current_directory) == NULL) {
        LOG_ERROR ("realpath() failed for '%s': %s.", buf, strerror(errno));
        return;
    }
    if (ncurses_window_filebrowser_fill_entries () != 0) {
        memcpy (_current_directory, buf, current_path_len);
        _current_directory[current_path_len] = 0;
    } else {
        ncurses_scroller_top (&_scroller);
    }
    _update_last_directory_change_time();
    ncurses_screen_set_user_info (_current_directory);
}

int ncurses_window_filebrowser_fill_entries (void)
{
    int num = 0;
    int max = _entries.max;
    DIR *dir = opendir (_current_directory);
    if (dir != NULL) {
        for (;;) {
            struct dirent *data = readdir (dir);
            if (data == NULL) {
                break;
            }
            FileBrowserEntry *entry = &_entries.data[num++];
            strncpy (entry->name, data->d_name, sizeof (entry->name) - 1);
            entry->name[sizeof (entry->name) - 1] = 0;
            entry->flags &= ~FILEBROWSER_ENTRY_FLAG_SEARCH_MATCH;
            if (data->d_type == DT_DIR) {
                entry->flags |= FILEBROWSER_ENTRY_FLAG_IS_DIRECTORY;
            } else {
                entry->flags &= ~FILEBROWSER_ENTRY_FLAG_IS_DIRECTORY;
            }
            if (num == max) {
                break;
            }
        }
    } else {
        LOG_ERROR ("Failed to open directory for browinsg files.");
        return -1;
    }
    closedir (dir);
    _entries.num = num;
    _scroller.page_max_index = num ? num - 1 : 0;
    /* Move . and .. to top. */
    for (int i = 0; i < num; ++i) {
        if (!strcmp (_entries.data[i].name, ".")) {
            FileBrowserEntry tmp = _entries.data[0];
            _entries.data[0] = _entries.data[i];
            _entries.data[i] = tmp;
            break;
        }
    }
    for (int i = 0; i < num; ++i) {
        if (!strcmp (_entries.data[i].name, "..")) {
            int dst_index;
            if (!strcmp (_entries.data[0].name, ".")) {
                dst_index = 1;
            } else {
                dst_index = 0;
            }
            FileBrowserEntry tmp = _entries.data[dst_index];
            _entries.data[dst_index] = _entries.data[i];
            _entries.data[i] = tmp;
        }
    }
    _sort(FALSE);
    return 0;
}

static int _compute_page_height (void)
{
    int height = _height - NUM_TOP_FILEBROWSER_ELEMENTS -
        NUM_BOTTOM_FILEBROWSER_ELEMENTS;
    if (height < 0) {
        return 0;
    }
    return height;
}

static int _compute_width (void)
{
    return _width;
}

void ncurses_window_filebrowser_search (const gchar *line)
{
    const gchar *search_term = line;
    FileBrowserEntry *entries = _entries.data;
    int num_entries = _entries.num;
    size_t keyword_len = strlen (search_term);
    char *(*compare_func)(const char *, const char *) =
        util_case_insensitive_strstr;
    /* If there are upper-case letters, do a case-sensitive search. */
    for (size_t i = 0; i < keyword_len; ++i) {
        if (isupper (search_term[i])) {
            compare_func = strstr;
            break;
        }
    }
    int selected_index = _scroller.selection_start_index;
    int jump_index = selected_index;
    for (int i = 0; i < num_entries; ++i) {
        memset (entries[i].hilight, 0, sizeof (entries[i].hilight));
        gboolean match = FALSE;
        const char *name = entries[i].name;
        for (const char *c = name; *c; ++c) {
            char *substr = compare_func (c, search_term);
            if (substr != NULL) {
                match = TRUE;
                unsigned int index = (unsigned int)(substr - name);
                unsigned int end = index + keyword_len;
                for (unsigned int j = index; j < end; ++j) {
                    _set_entry_char_hilight_on (entries[i].hilight, j);
                }
            }
        }
        if (!match) {
            entries[i].flags &= ~FILEBROWSER_ENTRY_FLAG_SEARCH_MATCH;
        } else {
            entries[i].flags |= FILEBROWSER_ENTRY_FLAG_SEARCH_MATCH;
            if (jump_index == selected_index) {
                jump_index = i;
            } else if (i >= selected_index && jump_index < selected_index) {
                jump_index = i;
            }
        }
    }
    int page_height = _scroller.page_height;
    if (num_entries > page_height && jump_index != selected_index) {
        if (jump_index < _scroller.page_start_index) {
            ncurses_scroller_page_start (&_scroller, jump_index);
        } else if (jump_index > (_scroller.page_start_index + page_height)) {
            ncurses_scroller_page_start (&_scroller,
                jump_index - page_height + 3);
        }
    }
    _show_search_hilight = TRUE;
}

void ncurses_window_filebrowser_cancel_search (void)
{
    ncurses_scroller_move_to (&_scroller, _page_start_index_before_search,
        _page_end_index_before_search, _selection_start_index_before_search,
        _selection_end_index_before_search);
    _cmd_mode = FILEBROWSER_CMD_MODE_NONE;
    int num_entries = _entries.num;
    FileBrowserEntry *entries = _entries.data;
    for (int i = 0; i < num_entries; ++i) {
        entries[i].flags &= ~FILEBROWSER_ENTRY_FLAG_SEARCH_MATCH;
    }
}

void ncurses_window_filebrowser_complete_search (void)
{
    _cmd_mode = FILEBROWSER_CMD_MODE_NONE;
    cmdline_clear ();
    FileBrowserEntry *entries = _entries.data;
    int num_entries = _entries.num;
    assert (_scroller.selection_start_index == _scroller.selection_end_index);
    int selected_index = _scroller.selection_start_index;
    for (int i = (selected_index + 1) % num_entries;
        i != selected_index;
        i = (i + 1) % num_entries) {
        if (entries[i].flags & FILEBROWSER_ENTRY_FLAG_SEARCH_MATCH) {
            ncurses_scroller_selection_start_and_end (&_scroller, i, i);
            break;
        }
    }
}

int ncurses_window_filebrowser_change_directory (const char *directory_path)
{
    char buf[PATH_MAX];
    if (directory_path[0] != '~' && directory_path[0] != '/') { /* Relative */
        size_t current_path_len = strlen (_current_directory);
        size_t directory_path_len = strlen (directory_path);
        size_t new_len = current_path_len + 1 + directory_path_len;
        if (new_len + 1 /* null term */ > sizeof (buf)) {
            ncurses_screen_set_user_info (_("Cannot change directory: relative "
                "path too long."));
            return -1;
        }
        memcpy (buf, _current_directory, current_path_len);
        buf[current_path_len] = '/';
        memcpy (buf + current_path_len + 1, directory_path, directory_path_len);
        buf[new_len] = 0;
    } else if (!util_expand_tilde (directory_path, buf)) {
        return -1;
    }
    char old_path[PATH_MAX];
    assert (sizeof (old_path) == sizeof (_current_directory));
    strcpy (old_path, _current_directory);
    if (realpath (buf, _current_directory) == NULL) {
        LOG_ERROR ("realpath() failed to resolve path for '%s'.", buf);
        strcpy (_current_directory, old_path);
        return -1;
    }
    _current_directory[sizeof (_current_directory) - 1] = 0;
    if (ncurses_window_filebrowser_fill_entries () != 0) {
        strcpy(_current_directory, old_path);
        return -1;
    }
    _update_last_directory_change_time();
    ncurses_scroller_top (&_scroller);
    ncurses_scroller_selection_start_and_end (&_scroller, 0, 0);
    ncurses_screen_set_user_info (_current_directory);
    return 0;
}

int ncurses_window_filebrowser_open (void)
{
    struct stat stat_buf;
    if (stat(_current_directory, &stat_buf) == -1) {
        LOG_ERROR ("stat() failed for directory '%s': %s",
            _current_directory, strerror (errno));
        return 1;
    }
    if (memcmp (&stat_buf.st_mtim, &_last_directory_change_time,
        sizeof(stat_buf.st_mtim)) != 0) {
        if (config.key_filebrowser_refresh.num_keys > 0) {
            ncurses_screen_format_user_info (
                "Directory contents changed (%s to update).",
                config.key_filebrowser_refresh.keys[0]);
        } else {
            ncurses_screen_set_user_info ("Directory contents changed.");
        }
    }
    _last_directory_change_time = stat_buf.st_mtim;
    return 0;
}

const char *ncurses_window_filebrowser_get_current_directory (void)
{
    return _current_directory;
}

void ncurses_window_filebrowser_next_sort_mode (void)
{
    _sort_mode = (_sort_mode + 1) % NUM_SORT_MODES;
    _sort (TRUE);
}

void ncurses_window_filebrowser_refresh (void)
{
    if (ncurses_window_filebrowser_fill_entries () == 0) {
        _update_last_directory_change_time ();
        ncurses_scroller_top (&_scroller);
        ncurses_scroller_selection_start_and_end (&_scroller, 0, 0);
        ncurses_screen_set_user_info (_("Refreshed."));
        ncurses_screen_update_force ();
    }
}

void ncurses_window_filebrowser_toggle_select (void)
{
    ncurses_scroller_selection_toggle (&_scroller, !_scroller.selection_toggle);
}

void ncurses_window_filebrowser_set_select_off (void)
{
    ncurses_scroller_selection_toggle (&_scroller, FALSE);
}

gboolean ncurses_window_filebrowser_get_select (void)
{
    return _scroller.selection_toggle;
}

void ncurses_window_filebrowser_jump (gint index)
{
    ncurses_scroller_jump (&_scroller, index);
}

static inline uint32_t _get_entry_char_hilight (uint32_t hilight[8],
    unsigned int index)
{
    unsigned int bucket_index = index / 32;
    unsigned int bit_index = index % 32;
    return hilight[bucket_index] & (uint32_t)(1 << bit_index);
}

static inline void _set_entry_char_hilight_on (uint32_t hilight[8],
    unsigned int index)
{
    unsigned int bucket_index = index / 32;
    unsigned int bit_index = index % 32;
    hilight[bucket_index] |= (1 << bit_index);
}

static void _set_selected_index (int index)
{
    int num_entries = _entries.num;
    int page_h = _scroller.page_height;
    assert (index < _entries.num);
    ncurses_scroller_selection_start_and_end (&_scroller, index, index);
    if (num_entries <= page_h) {
        return;
    }
    int page_bottom = _scroller.page_start_index + page_h;
    if (page_bottom < index || _scroller.page_start_index > index) {
        ncurses_window_filebrowser_center_screen_on_cursor ();
    }
}

static inline void _draw_filebrowser_line (int y, int x, int max_w,
    FileBrowserEntry *entry, int color, int search_match_color)
{
    char utf8[MAX_UTF8_CHAR_SIZE];
    char *next = 0;
    size_t len;
    int current_x = x;
    wmove (_win, y, current_x);
    gboolean is_too_long = FALSE;
    if (!_show_search_hilight ||
        !(entry->flags & FILEBROWSER_ENTRY_FLAG_SEARCH_MATCH)) {
        ncurses_colors_pair_set (_win, color);
        for (const char *c = entry->name; *c; ) {
            if (current_x == max_w) {
                is_too_long = TRUE;
                break;
            }
            next = g_utf8_find_next_char (c, NULL);
            len = (size_t)(next - c);
            memcpy (utf8, c, len);
            utf8[len] = '\0';
            wprintw (_win, "%s", utf8);
            c = next;
            current_x++;
        }
    } else { /* Search match */
        unsigned int index = 0;
        for (const char *c = entry->name; *c;) {
            if (current_x == max_w) {
                is_too_long = TRUE;
                break;
            }
            int color_to_use;
            if (!_get_entry_char_hilight (entry->hilight, index)) {
                color_to_use = color;
            } else {
                color_to_use = search_match_color;
            }
            ncurses_colors_pair_set (_win, color_to_use);
            next = g_utf8_find_next_char (c, NULL);
            len = (size_t)(next - c);
            memcpy (utf8, c, len);
            utf8[len] = '\0';
            wprintw (_win, "%s", utf8);
            c = next;
            current_x++;
            index += len;
        }
    }
    if (!is_too_long || max_w < 4) {
        return;
    }
    ncurses_colors_pair_set (_win, color);
    int num_dots = max_w < 3 ? max_w : 3;
    int dot_start = max_w - num_dots;
    wmove (_win, y, dot_start);
    for (int i = dot_start; i <= max_w; ++i) {
        waddch (_win, '.');
    }
}

static int _sort_mode_name_callback (const void *a, const void *b)
{
    const FileBrowserEntry *entry_a = a;
    const FileBrowserEntry *entry_b = b;
    if (entry_a->flags & FILEBROWSER_ENTRY_FLAG_IS_DIRECTORY) {
        if (!(entry_b->flags & FILEBROWSER_ENTRY_FLAG_IS_DIRECTORY)) {
            return -1;
        }
    } else if (entry_b->flags & FILEBROWSER_ENTRY_FLAG_IS_DIRECTORY) {
        return 1;
    }
    return strcoll (entry_a->name, entry_b->name);
}

static void _sort (gboolean print_user_info)
{
    if (_entries.num < 2) /* . and .. */ {
        return;
    }
    const gchar *msg = "";
    int (*sort_callback) (const void *, const void *);
    switch (_sort_mode)
    {
    case SORT_MODE_NAME:
        sort_callback = _sort_mode_name_callback;
        msg = _("Sorted alphabetically.");
        break;
    default:
        assert (0);
    };
    qsort (_entries.data + 2, _entries.num - 2,
        sizeof (_entries.data[0]), sort_callback);
    if (print_user_info) {
        ncurses_screen_set_user_info (msg);
    }
}

static void _update_last_directory_change_time(void)
{
    struct stat stat_buf = {0};
    if (stat (_current_directory, &stat_buf) == -1) {
        LOG_ERROR ("stat() failed for directory '%s': %s", _current_directory,
            strerror (errno));
    }
    _last_directory_change_time = stat_buf.st_mtim;
}
