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
#include <ctype.h>

#include "ncurses-common.h"
#include "ncurses-colors.h"
#include "ncurses-window-user-info.h"
#include "playlist.h"
#include "player.h"
#include "util.h"
#include "cmdline.h"

#define LIST_SPACING 2
/* ncurses */
static gint _width = 0;
static gint _height = 0;

static WINDOW *_win = NULL;

/* cmdline stuff */
static GSList *_cmdline_options = NULL; /* possible options */
static gint _cmdline_options_index = -1; /* current index */
static gint _cmdline_options_first_visible_index = 0; /* first visible option */
static gint _cmdline_options_visible_count = 0; /* how many visible */

static void _find_page (gint index, gint len);
static void _calculate_visible_count_from_index (gint index, gint len);

gboolean ncurses_window_user_info_init (void)
{
    return TRUE;
}

gboolean ncurses_window_user_info_resize (gint width, gint height, gint x, gint y)
{
    _width = width;
    _height = height;

    if (_win != NULL) ncurses_window_user_info_delete ();
    _win = newwin (_height, _width, y, x);
    if (_win == NULL) goto resize_error;

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_user_info_delete (void)
{
    if (_win != NULL) delwin (_win);
    _win = NULL;
}

void ncurses_window_user_info_clear (void)
{
    wclear (_win);
}

void ncurses_window_user_info_update (const gchar *info)
{
    if (_cmdline_options != NULL && _cmdline_options_visible_count > -1 && _cmdline_options_index > -1) {
        gint len = g_slist_length (_cmdline_options) - 1;
        gint pos = 0;
        if (_cmdline_options_first_visible_index > 0) {
            ncurses_colors_pair_set(_win, COLOR_PAIR_BLACK_WHITE);
            mvwprintw (_win, 0, pos, "< ");
            pos += 2;
        }
        for (gint i = _cmdline_options_first_visible_index; i < (_cmdline_options_first_visible_index + _cmdline_options_visible_count); i++) {
            CmdlineItem *item = (CmdlineItem *)g_slist_nth_data (_cmdline_options, i);
            if (item->str == NULL) continue;
            gint len = g_utf8_strlen (item->str, -1);
            ncurses_colors_pair_set(_win, COLOR_PAIR_BLACK_WHITE);
            if (i == _cmdline_options_index) {
                ncurses_colors_pair_set(_win, COLOR_PAIR_BLACK_YELLOW);
            }
            mvwprintw (_win, 0, pos, "%s", item->str);
            pos = pos + len;
            ncurses_colors_pair_set(_win, COLOR_PAIR_BLACK_WHITE);
            mvwprintw (_win, 0, pos, "  ");
            pos += LIST_SPACING;
        }
        mvwprintw (_win, 0, pos, "%-*s", _width-pos, " ");
        if (len-1 > _cmdline_options_first_visible_index + _cmdline_options_visible_count) {
            mvwprintw (_win, 0, _width-2, " >");
            pos += 2;
        }
    } else {
        ncurses_colors_pair_set(_win, COLOR_PAIR_WHITE_BLACK);
        if (info != NULL) mvwprintw (_win, 0, 0, "%-*s", _width, info);
        else mvwprintw (_win, 0, 0, "%-*s", _width, " ");
    }
    wrefresh (_win);
}

void ncurses_window_user_info_set_cmdline_options (GSList *options)
{
    _cmdline_options = options;
}

void ncurses_window_user_info_set_cmline_options_index (gint index)
{
    _cmdline_options_index = index;
    if (_cmdline_options_index > -1 && _cmdline_options != NULL){
        /* paging */
        gint len = g_slist_length (_cmdline_options) - 1;
        _find_page (index, len);
        _calculate_visible_count_from_index (_cmdline_options_first_visible_index, len);
    } else { /* unset */
        _cmdline_options_first_visible_index = 0;
        _cmdline_options_visible_count = 0;
    }
}

static void _find_page (gint index, gint len)
{
    glong strlen = 4; /* FIXME: take space for < and > */
    gint items_added_count = 0;
    _cmdline_options_first_visible_index = 0;

    for (gint pos = 0; pos < len; pos++) {
        CmdlineItem *item = (CmdlineItem *)g_slist_nth_data (_cmdline_options, pos);
        strlen += g_utf8_strlen (item->str, -1);
        if (_width < strlen || pos == index) {
            if (_width < strlen && items_added_count > 0) {
                pos--;
                items_added_count--;
            }
            if (pos == index) break;
            /* start new page */
            _cmdline_options_first_visible_index += (items_added_count + 1);
            items_added_count = 0;
            strlen = 4;
        } else {
            strlen += LIST_SPACING;
            items_added_count++;
        }
    }
}

static void _calculate_visible_count_from_index (gint index, gint len)
{
    /* check how many items fits to line */
    /* at least one item is visible, but as many as possible */
    glong strlen = 4;
    for (_cmdline_options_visible_count = 0;
            _cmdline_options_visible_count + index < len;
            _cmdline_options_visible_count++) {
        CmdlineItem *item = (CmdlineItem *)g_slist_nth_data (_cmdline_options, index + _cmdline_options_visible_count);
        strlen += g_utf8_strlen (item->str, -1);
        if (_cmdline_options_visible_count == 0 && _width < g_utf8_strlen (item->str, -1)) {
            _cmdline_options_visible_count = 1;
            break;
        }
        if (_width < strlen) break;
        strlen += LIST_SPACING;
    }
}
