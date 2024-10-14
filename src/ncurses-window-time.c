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
#include <libintl.h>
#define _(String) gettext (String)

#include "ncurses-common.h"
#include "ncurses-colors.h"
#include "ncurses-window-time.h"
#include "playlist.h"
#include "player.h"
#include "util.h"

#define TIME_MIN_HEIGHT 1

static char _tmp[ABSOLUTELY_MAX_LINE_LEN] = "--";

/* ncurses */
static gint _width = 0;
static gint _height = 0;

static WINDOW *_win = NULL;

gboolean ncurses_window_time_init (void)
{
    return TRUE;
}

gboolean ncurses_window_time_resize (gint width, gint height, gint x, gint y)
{
    _width = width;
    _height = height;

    if (_win != NULL) ncurses_window_time_delete ();
    _win = newwin (_height, _width, y, x);
    if (_win == NULL) goto resize_error;

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_time_delete (void)
{
    if (_win != NULL) delwin (_win);
    _win = NULL;
}

void ncurses_window_time_clear (void)
{
    wclear (_win);
}

void ncurses_window_time_update (Song *o, gint sid_tune_index, gint64 ms)
{
    PlayerState state;
    char current_position[MAX_TIME_STR_LEN];
    char total_time[MAX_TIME_STR_LEN];

    ncurses_colors_pair_set (_win, COLOR_PAIR_GREEN_BLACK);
    if (_height < TIME_MIN_HEIGHT || _width < SCREEN_MIN_WIDTH) return;

    if (o == NULL) {
        g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, " ");
    } else {
        state = player_state ();
        if (state == PLAYER_STATE_PLAYING || state == PLAYER_STATE_PAUSED) {
            gint64 duration = 0;
            if (o->type == SONG_TYPE_SID) {
                duration = o->tune_duration[sid_tune_index];
            } else {
                duration = o->duration;
            }
            ms = player_get_current_time ();
            if (ms > -1) util_time_to_string (current_position, MAX_TIME_STR_LEN, ms);
            util_time_to_string (total_time, MAX_TIME_STR_LEN, duration);

            gchar *paused = "";
            if (state == PLAYER_STATE_PAUSED) {
                paused = _("Paused");
            }

            if (duration == 0 && ms > -1) {
                g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%s %-*s",current_position, _width-2, paused);
            } else if (duration > 0 && ms > -1) {
                g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%s/%s %-*s", current_position, total_time, _width-2, paused);
            } else {
                g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, paused);
            }
        } else if (state == PLAYER_STATE_STOPPED) {
            g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, " ");
        } else {
            g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%-*s", _width-2, _("Unexpected state"));
        }
    }
    mvwprintw (_win, 0, 0, "%s", _tmp);
    wrefresh (_win);
}
