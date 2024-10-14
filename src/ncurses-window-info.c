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
#include "ncurses-window-info.h"
#include "playlist.h"
#include "player.h"
#include "util.h"

static char _tmp[ABSOLUTELY_MAX_LINE_LEN] = "--";

/* ncurses */
static gint _width = 0;
static gint _height = 0;

static WINDOW *_win = NULL;

gboolean ncurses_window_info_init (void)
{
    return TRUE;
}

gboolean ncurses_window_info_resize (gint width, gint height, gint x, gint y)
{
    _width = width;
    _height = height;

    if (_win != NULL) ncurses_window_info_delete ();
    _win = newwin (_height, _width, y, x);
    if (_win == NULL) goto resize_error;

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_info_delete (void)
{
    if (_win != NULL) delwin (_win);
    _win = NULL;
}

void ncurses_window_info_clear (void)
{
    wclear (_win);
}

void ncurses_window_info_update (gint sid_tune_index)
{
    gchar *t = NULL;
    /* song information */
    Song *current_song = playlist_get_current_song ();

    ncurses_colors_pair_set (_win, COLOR_PAIR_GREEN_BLACK);
    if (current_song != NULL) {
        if (current_song->type == SONG_TYPE_STREAM) t = current_song->stream_title;
        else if (current_song->title != NULL) t = current_song->title;
        else if (current_song->basename != NULL) t = current_song->basename;
        else t = "-";
        if (current_song->type == SONG_TYPE_SID && current_song->tunes > 1) {
            gint extra = 0;
            gint sid_tune = sid_tune_index + 1;
            if (sid_tune < 10) extra = extra + 1;
            else if (sid_tune < 100) extra = extra + 2;
            else if (sid_tune < 1000) extra = extra + 3;
            if (current_song->tunes < 10) extra = extra + 1;
            else if (current_song->tunes < 100) extra = extra + 2;
            else if (current_song->tunes < 1000) extra = extra + 3;
            g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%s (%d/%d%-*s", t, sid_tune, current_song->tunes, _width-6-extra, ")");
        } else {
            g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, t);
        }
        wattron (_win, A_BOLD);
        mvwprintw(_win, 0, 0, "%s", _tmp);
        wattroff (_win, A_BOLD);
        if (current_song->type == SONG_TYPE_STREAM) {
            g_snprintf(_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, "-");
            if (current_song->title != NULL) {
                g_snprintf(_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, current_song->title);
            }
            mvwprintw(_win, 1, 0, "%s", _tmp);
            g_snprintf(_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, "-");
            mvwprintw(_win, 2, 0, "%s", _tmp);
        } else {
            if (current_song->artist != NULL) g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, current_song->artist);
            else g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, "-");
            mvwprintw(_win, 1, 0, "%s", _tmp);
            if (current_song->album != NULL) g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, current_song->album);
            else if (current_song->type == SONG_TYPE_SID && current_song->copyright != NULL)
                g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, current_song->copyright);
            else g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, "-");
            mvwprintw(_win, 2, 0, "%s", _tmp);
        }

    } else {
        g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, "-");
        mvwprintw (_win, 0, 0, "%s", _tmp);
        mvwprintw (_win, 1, 0, "%s", _tmp);
        mvwprintw (_win, 2, 0, "%s", _tmp);
    }
    wrefresh (_win);
}
