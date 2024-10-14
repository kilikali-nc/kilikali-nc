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
#include "ncurses-window-volume-and-mode.h"
#include "playlist.h"
#include "player.h"
#include "util.h"

static char _tmp[ABSOLUTELY_MAX_LINE_LEN] = "--";

/* ncurses */
static gint _width = 0;
static gint _height = 0;

static WINDOW *_win = NULL;

gboolean ncurses_window_volume_and_mode_init (void)
{
    return TRUE;
}

gboolean ncurses_window_volume_and_mode_resize (gint width, gint height, gint x, gint y)
{
    _width = width;
    _height = height;

    if (_win != NULL) ncurses_window_volume_and_mode_delete ();
    _win = newwin (_height, _width, y, x);
    if (_win == NULL) goto resize_error;

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_volume_and_mode_delete (void)
{
    if (_win != NULL) delwin (_win);
    _win = NULL;
}

void ncurses_window_volume_and_mode_clear (void)
{
    wclear (_win);
}

void ncurses_window_volume_and_mode_update (void)
{
    gchar volume_str[STR_MAX_LEN];
    gchar loop_str[STR_MAX_LEN];
    if (playlist_loop()==TRUE) g_snprintf (loop_str, STR_MAX_LEN, "(%s)", _("loop"));
    else loop_str[0] = '\0';
    int loop_strlen = (int)g_utf8_strlen(loop_str, -1);
    PlaylistMode pl_mode = playlist_mode ();
    int vol = (int)player_volume ();
    ncurses_colors_pair_set (_win, COLOR_PAIR_RED_BLACK);
    const gchar *mode_str = playlist_mode_to_string();
    g_snprintf (volume_str, STR_MAX_LEN, _("Volume: %3d"), vol);
    if (pl_mode == PLAYLIST_MODE_RANDOM) {
        if (strlen(mode_str) > 0) {
            g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%s | %-*s",
                volume_str,
                _width-2-3-(int)g_utf8_strlen(volume_str, -1)-(int)g_utf8_strlen (mode_str, -1),
                mode_str);
        } else {
            g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%s %-*c",
                volume_str,
                _width-2-(int)g_utf8_strlen(volume_str, -1)-1-1,
                ' ');
        }
    } else {
        if (strlen(mode_str) > 0) {
            g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%s | %s %-*s",
                volume_str,
                mode_str,
                _width-2-(int)g_utf8_strlen(volume_str, -1)-3-(int)g_utf8_strlen (mode_str, -1)-loop_strlen-2,
                loop_str);
        } else {
            g_snprintf (_tmp, ABSOLUTELY_MAX_LINE_LEN, "%s %-*s",
                volume_str,
                _width-2-(int)g_utf8_strlen(volume_str, -1)-1-loop_strlen-1,
                loop_str);
        }
    }
    mvwprintw (_win, 0, 0, "%s", _tmp);
    wrefresh (_win);
}
