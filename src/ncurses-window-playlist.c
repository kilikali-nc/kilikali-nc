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
#include <assert.h>
#include <ctype.h>
#include <libintl.h>
#define _(String) gettext (String)

#include "ncurses-window-playlist.h"
#include "ncurses-common.h"
#include "ncurses-scroller.h"
#include "ncurses-key-sequence.h"

#include "playlist.h"
#include "playlist-pls.h"
#include "playlist-line.h"
#include "song.h"
#include "util.h"
#include "log.h"
#include "search.h"

static void _print_playlist_line (Song *ol, gint index, gint list_len, gint page_line);
static void _print_edit_line (Song *ol, gint index, gint list_len, gint page_line);
static inline void _update (void);
static inline void _draw_playlist_line (int y, int max_w, const char *line, int color, int search_match_color, Song *o, int x_start);

static char _tmp[ABSOLUTELY_MAX_LINE_LEN];

static NCursesScroller _scroller;
static gint _pages = 0;

static NCursesWindowPlaylistMode _mode = NCURSES_WINDOW_PLAYLIST_MODE_NORMAL;

/* ncurses */
static int _width;
static int _height;

static WINDOW *_win = NULL;

static gboolean _show_search_hilight;

gboolean ncurses_window_playlist_init (void)
{
    _mode = NCURSES_WINDOW_PLAYLIST_MODE_NORMAL;
    _show_search_hilight = FALSE;
    ncurses_scroller_init (&_scroller, 0, 0);
    return TRUE;
}

gboolean ncurses_window_playlist_resize (gint width, gint height, gint x, gint y)
{
    gint list_len = playlist_length ();
    if (_win != NULL) ncurses_window_playlist_delete ();
    _win = newwin (height, width, y, x);
    if (_win == NULL) goto resize_error;

    _width = width;
    _height = height;

    ncurses_scroller_resize (&_scroller, height-1);
    ncurses_scroller_page_max_index (&_scroller, list_len-1);

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_playlist_delete (void)
{
    if (_win != NULL) {
        delwin (_win);
    }
    _win = NULL;
}

void ncurses_window_playlist_clear (void)
{
    wclear(_win);
}

void ncurses_window_playlist_mode_set (NCursesWindowPlaylistMode mode)
{
    _mode = mode;
}

NCursesWindowPlaylistMode ncurses_window_playlist_mode (void)
{
    return _mode;
}


void ncurses_window_playlist_color_set (NCursesColorPair color)
{
    ncurses_colors_pair_set (_win, color);
}

void ncurses_window_playlist_unselect_all (void)
{
    ncurses_scroller_selection_start_and_end (&_scroller, 0, 0);
}

void ncurses_window_playlist_selections_set (gint start, gint end)
{
    /* move to scroller ??? */
    gint list_len = playlist_length ();
    if (end < 0 && list_len > 0) {
        end = 0;
        start = end;
    } else if (end > list_len - 1) {
        end = list_len - 1;
        start = end;
    }
    ncurses_scroller_selection_start_and_end (&_scroller, start, end);
}

gint ncurses_window_playlist_selection_start (void)
{
    return _scroller.selection_start_index;
}

gint ncurses_window_playlist_selection_end (void)
{
    return _scroller.selection_end_index;
}

gint ncurses_window_playlist_selection_min (void)
{
    return _scroller.selection_end_index < _scroller.selection_start_index ? _scroller.selection_end_index : _scroller.selection_start_index;
}

gint ncurses_window_playlist_selection_max (void)
{
    return _scroller.selection_end_index < _scroller.selection_start_index ? _scroller.selection_start_index : _scroller.selection_end_index;
}


void ncurses_window_playlist_update (void)
{
    playlist_search_index_set (_scroller.selection_end_index);
    _update ();
}

void ncurses_window_playlist_ensure_page_start_index (void)
{
    ncurses_scroller_ensure_page_start_index (&_scroller);
}

void ncurses_window_playlist_up (void)
{
    ncurses_scroller_up (&_scroller);
}

void ncurses_window_playlist_down (void)
{
    ncurses_scroller_down (&_scroller);
}

void ncurses_window_playlist_select_multiple_up (void)
{
#if 0
    if (_selection_end_index > 0) {
        _selection_end_index--;
    }
    _test_and_set_page_up (1);
#endif
}

void ncurses_window_playlist_select_multiple_down (void)
{
#if 0
    gint list_len = playlist_length ();
    if (_selection_end_index < list_len-1) {
        _selection_end_index++;
    }
    _test_and_set_page_down (1);
#endif
}

void ncurses_window_playlist_select_all (void)
{
    ncurses_scroller_selection_start_and_end (&_scroller, 0, _scroller.page_max_index);
}

void ncurses_window_playlist_scroll_down (void)
{
    ncurses_scroller_scroll_down (&_scroller);
}

void ncurses_window_playlist_scroll_up (void)
{
    ncurses_scroller_scroll_up (&_scroller);
}

void ncurses_window_playlist_down_half_page (void)
{
    ncurses_scroller_down_half_page (&_scroller);
}

void ncurses_window_playlist_up_half_page (void)
{
    ncurses_scroller_up_half_page (&_scroller);
}

void ncurses_window_playlist_down_full_page (void)
{
    ncurses_scroller_down_full_page (&_scroller);
}

void ncurses_window_playlist_up_full_page (void)
{
    ncurses_scroller_up_full_page (&_scroller);
}

void ncurses_window_playlist_top (void)
{
    ncurses_scroller_top (&_scroller);
}

void ncurses_window_playlist_bottom (void)
{
    ncurses_scroller_bottom (&_scroller);
}

void ncurses_window_playlist_scroll_center_to_cursor (void)
{
    ncurses_scroller_scroll_center_to_cursor (&_scroller);
}

void ncurses_window_playlist_toggle_select_set (gboolean value)
{
    ncurses_scroller_selection_toggle (&_scroller, value);
}

gboolean ncurses_window_playlist_toggle_select (void)
{
    return _scroller.selection_toggle;
}

void ncurses_window_playlist_jump (gint index)
{
    ncurses_scroller_jump (&_scroller, index);
}

void ncurses_window_playlist_search_next (void)
{
    gint found_index = playlist_search_next ();
    if (found_index > -1) {
        ncurses_window_playlist_selections_set (found_index, found_index);
    }
    _show_search_hilight = TRUE;
}

void ncurses_window_playlist_search_prev (void)
{
    gint found_index = playlist_search_prev ();
    if (found_index > -1) {
        ncurses_window_playlist_selections_set (found_index, found_index);
    }
    _show_search_hilight = TRUE;
}

void ncurses_window_playlist_clear_search (void)
{
    _show_search_hilight = FALSE;
}

void ncurses_window_playlist_show_search_hilight (void)
{
    _show_search_hilight = TRUE;
}

static inline void _draw_playlist_line (int y, int max_w, const char *line, int color, int search_match_color, Song *o, int x_start)
{
    char utf8[ABSOLUTELY_MAX_LINE_LEN];
    char *next = 0;
    int x = x_start;
    gboolean is_too_long = FALSE;
    const gchar *hit_str = playlist_search_string ();
    gint next_index = o->search_hit;

    if (line == NULL) {
        return;
    }
    wmove(_win, y, x_start);

    if (!_show_search_hilight || hit_str == NULL || o->search_hit < 0) {
        ncurses_colors_pair_set(_win, color);
        for (const char *c = line; *c; ) {
            size_t len = 0;
            if (x == max_w) {
                is_too_long = TRUE;
                break;
            }
            next = g_utf8_find_next_char (c, NULL);
            len = (size_t)(next - c);
            memcpy(utf8, c, len);
            utf8[len] = '\0';
            wprintw (_win, "%s", utf8);
            c = next;
            x++;
        }
    } else { /* Search match */
        unsigned int index = 0;
        size_t cur_sm = 0;
        SearchMatchType sm;
        if (playlist_search_get_search_match (o, &sm) == FALSE || sm.num_matches < 1) {
           next_index = -1;
        } else {
           next_index = sm.results[cur_sm].start;
        }
        for (const char *c = line; *c;) {
            size_t len = 0;
            gboolean hit = FALSE;
            if (x == max_w) {
                is_too_long = TRUE;
                break;
            }
            int color_to_use;
            if (next_index != index) {
                color_to_use = color;
            } else {
                hit = TRUE;
                color_to_use = search_match_color;
            }
            ncurses_colors_pair_set(_win, color_to_use);
            if (hit == TRUE) {
                len = sm.results[cur_sm].end - sm.results[cur_sm].start;
                x += (g_utf8_pointer_to_offset(line, &line[sm.results[cur_sm].end]) -
                    g_utf8_pointer_to_offset(line, &line[sm.results[cur_sm].start]));
            } else {
                next = g_utf8_find_next_char (c, NULL);
                len = (size_t)(next - c);
                x++;
            }
            memcpy(utf8, c, len);
            utf8[len] = '\0';
            wprintw (_win, "%s", utf8);
            c += len;
            if (next_index > -1 && hit == TRUE) {
                next_index = -1;
		if (++cur_sm < sm.num_matches) {
		    next_index = sm.results[cur_sm].start;
		}
            }
            index += len;
        }
    }
    if (is_too_long && max_w > 4) {
        ncurses_colors_pair_set(_win, color);
        int num_dots = max_w < 3 ? max_w : 3;
        int dot_start = max_w + 1 - num_dots;
        wmove(_win, y, dot_start);
        for (int i = dot_start; i <= max_w; ++i) {
            waddch(_win, '.');
            x++;
        }
    }

    ncurses_colors_pair_set(_win, color);
    unsigned int len = _width - x;
    mvwprintw (_win, y, x, "%-*s", len, " ");
}

static void _print_playlist_line (Song *ol, gint index, gint list_len, gint page_line)
{
    gint numbers = 2;
    gchar line[ABSOLUTELY_MAX_LINE_LEN] = {0,};
    Song *current_song = playlist_get_current_song ();
    gint current_index = playlist_get_song_index (current_song);
    int color = COLOR_PAIR_BLACK_LIGHTGREY;
    int selected_color = COLOR_PAIR_BLACK_WHITE;
    int selected_match_color = COLOR_PAIR_BLACK_YELLOW;
    int match_color = COLOR_PAIR_BLACK_YELLOW;
    int numb_color = COLOR_PAIR_GREY_BLACK;
    if (ol != NULL) {
        if ((_scroller.selection_start_index < _scroller.selection_end_index &&
            index >= _scroller.selection_start_index &&
            index <= _scroller.selection_end_index) ||
            (_scroller.selection_start_index > _scroller.selection_end_index &&
            index <= _scroller.selection_start_index &&
            index >= _scroller.selection_end_index)) {
            /* Selected */
            color = COLOR_PAIR_BLACK_WHITE;
            selected_color = COLOR_PAIR_BLACK_WHITE;
            match_color = COLOR_PAIR_BLACK_YELLOW;
        } else if (index == _scroller.selection_end_index) {
            /* Not selected */
            color = COLOR_PAIR_BLACK_WHITE;
            match_color = COLOR_PAIR_BLACK_YELLOW;
        } else {
            /* Not selected */
            color = COLOR_PAIR_LIGHTGREY_BLACK;
            selected_color = COLOR_PAIR_WHITE_BLACK;
        }
        if (list_len > 999999999) {
            numbers = 10;
        } else if (list_len > 99999999) {
            numbers = 9;
        } else if (list_len > 9999999) {
            numbers = 8;
        } else if (list_len > 999999) {
            numbers = 7;
        } else if (list_len > 99999) {
            numbers = 6;
        } else if (list_len > 9999) {
            numbers = 5;
        } else if (list_len > 999) {
            numbers = 4;
        } else if (list_len > 99) {
            numbers = 3;
        } else {
            numbers = 2;
        }
        assert(numbers <= KEY_SEQUENCE_MAX_DIGITS);

        gchar index_str[12];
        g_snprintf (index_str, sizeof(index_str), "%d", index+1);

        if (index == current_index) {
            color = selected_color;
            match_color = selected_match_color;
        }

        ncurses_colors_pair_set (_win, numb_color);
        playlist_line_create (line, ABSOLUTELY_MAX_LINE_LEN - numbers - 4, ol);
        if (playlist_search_with_tags() == FALSE && ol->search_hit > -1) {
            mvwprintw (_win, page_line, 0, "%*.*s ", numbers, numbers, index_str);
            _draw_playlist_line (page_line, _width, line, color, match_color, ol, numbers + 1);
        } else {
            gint utf8_extra_bytes = (gint)(strlen(line) - g_utf8_strlen(line, -1));
            mvwprintw (_win, page_line, 0, "%*.*s ", numbers, numbers, index_str);
            g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s",_width-numbers+utf8_extra_bytes, line);
            if (ol->search_hit > -1) {
                ncurses_colors_pair_set (_win, match_color);
            } else {
                ncurses_colors_pair_set (_win, color);
            }
            mvwprintw (_win, page_line, numbers+1, "%s", _tmp);
        }
    } else {
        g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width, "");
        ncurses_colors_pair_set (_win, COLOR_PAIR_WHITE_BLACK);
        mvwprintw (_win, page_line, 0, "%s", _tmp);
    }
    ncurses_colors_pair_set (_win, COLOR_PAIR_BLACK_WHITE);
}

static void _print_edit_line (Song *ol, gint index, gint list_len, gint page_line)
{
    gint utf8_extra_bytes = 0;
    gint numbers = 2;
    gchar line[ABSOLUTELY_MAX_LINE_LEN] = {0,};
    gchar selected = ' ';
    gchar index_str[12];
    g_snprintf (index_str, sizeof(index_str), "%d", index+1);
    int numb_color = COLOR_PAIR_GREY_BLACK;
    int color = COLOR_PAIR_BLACK_LIGHTGREY;
    int playing_color = COLOR_PAIR_BLACK_WHITE;
    Song *current_song = playlist_get_current_song ();
    gint current_index = playlist_get_song_index (current_song);

    if (ol != NULL) {
        if (_scroller.selection_start_index < _scroller.selection_end_index &&
            index >= _scroller.selection_start_index &&
            index <= _scroller.selection_end_index) {
            color = COLOR_PAIR_BLACK_LIGHTGREY;
            playing_color = COLOR_PAIR_BLACK_WHITE;
        } else if (_scroller.selection_start_index > _scroller.selection_end_index &&
            index <= _scroller.selection_start_index &&
            index >= _scroller.selection_end_index) {
            color = COLOR_PAIR_BLACK_LIGHTGREY;
            playing_color = COLOR_PAIR_BLACK_WHITE;
        } else if (index == _scroller.selection_end_index) {
             color = COLOR_PAIR_BLACK_LIGHTGREY;
            playing_color = COLOR_PAIR_BLACK_WHITE;
        } else {
            color = COLOR_PAIR_LIGHTGREY_BLACK;
            playing_color = COLOR_PAIR_WHITE_BLACK;
        }
        if (list_len > 999999999) {
            numbers = 10;
        } else if (list_len > 99999999) {
            numbers = 9;
        } else if (list_len > 9999999) {
            numbers = 8;
        } else if (list_len > 999999) {
            numbers = 7;
        } else if (list_len > 99999) {
            numbers = 6;
        } else if (list_len > 9999) {
            numbers = 5;
        } else if (list_len > 999) {
            numbers = 4;
        } else if (list_len > 99) {
            numbers = 3;
        } else {
            numbers = 2;
        }

        if (index == current_index) {
            color = playing_color;
        }

        playlist_line_create (line, ABSOLUTELY_MAX_LINE_LEN - numbers, ol);
        utf8_extra_bytes = (gint)(strlen(line) - g_utf8_strlen(line, -1));
        selected = ol->selected==TRUE?'x':' ';
        ncurses_colors_pair_set (_win, COLOR_PAIR_LIGHTGREY_BLACK);
        mvwprintw (_win, page_line, 0, "[%c] ", selected);
        ncurses_colors_pair_set (_win, numb_color);
        mvwprintw (_win, page_line, 4, "%*.*s ", numbers, numbers, index_str);
        ncurses_colors_pair_set (_win, color);
        mvwprintw (_win, page_line, 5+numbers, "%-*s", _width-6-numbers+utf8_extra_bytes, line);

    } else {
        g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width, "");
        ncurses_colors_pair_set (_win, COLOR_PAIR_WHITE_BLACK);
        mvwprintw (_win, page_line, 0, "%s", _tmp);
    }
}

static inline void _update (void)
{
    /* playlist */
    gint i = 0;
    gint index = 0;
    gint list_len = playlist_length ();
    ncurses_scroller_page_max_index (&_scroller, list_len-1);

    ncurses_colors_pair_set (_win, COLOR_PAIR_WHITE_BLACK);
    _pages = 1 + list_len / (_scroller.page_height + 1);

    /* make sure current page is in range */
    ncurses_window_playlist_ensure_page_start_index ();

    if (list_len > 0) {
        char total_str[STR_MAX_LEN];
        Song *ol;
        for (i = 0; i < _scroller.page_height; i++) {
            index = i + _scroller.page_start_index;
            ol = playlist_get_nth_song_no_set (index);
            if (_mode == NCURSES_WINDOW_PLAYLIST_MODE_NORMAL) {
                _print_playlist_line (ol, index, list_len, i);
            } else if (_mode == NCURSES_WINDOW_PLAYLIST_MODE_EDIT) {
                _print_edit_line (ol, index, list_len, i);
            } else {
                g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width, "");
                mvwprintw (_win, i, 0, "%s", _tmp);
            }
        }
        ncurses_colors_pair_set (_win, COLOR_PAIR_RED_BLACK);
        g_snprintf (total_str, STR_MAX_LEN, _("Total: %d"), list_len);
        g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%s %-*c", total_str, (_width-2-(int)g_utf8_strlen(total_str, -1)), ' ');
        mvwprintw (_win, i, 0, "%s", _tmp);
    } else {
        g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width, "");
        for (i = 0; i < _scroller.page_height+1; i++) {
            mvwprintw (_win, i, 0, "%s", _tmp);
        }
    }
    wrefresh (_win);
}

