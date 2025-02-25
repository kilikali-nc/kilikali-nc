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
#include "ncurses-window-lyrics.h"
#include "ncurses-subwindow-textview.h"

#include "net.h"

static gboolean _got_lyrics (gpointer data);

/* ncurses */
static gint _width = 0;
static gint _height = 0;
static gint _x = 0;
static gint _subheight = 0;
static gint _suby = 0;

static WINDOW *_win = NULL;

static gchar _title[ABSOLUTELY_MAX_LINE_LEN] = "Lyrics";

static NCursesSubwindowTextview _textview;

GSourceFunc _update;

gboolean ncurses_window_lyrics_init (GSourceFunc update)
{
    _update = update;
    ncurses_subwindow_textview_init (&_textview);
    return TRUE;
}

gboolean ncurses_window_lyrics_resize (gint width, gint height, gint x, gint y)
{
    _width = width;
    _height = 2;
    _subheight = height - 2;
    _suby = y + 2;
    _x = x;

    ncurses_subwindow_textview_resize (&_textview, _width, _subheight, _x, _suby);

    if (_win != NULL) ncurses_window_lyrics_delete ();
    _win = newwin (_height, _width, y, x);
    if (_win == NULL) goto resize_error;

    ncurses_colors_pair_set (_win, COLOR_PAIR_BLUE_BLACK);

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_lyrics_delete (void)
{
    ncurses_subwindow_textview_delete (&_textview);

    if (_win != NULL) delwin (_win);
    _win = NULL;
}

void ncurses_window_lyrics_clear (void)
{
    wclear (_win);
    ncurses_subwindow_textview_clear (&_textview);
}

void ncurses_window_lyrics_update (void)
{
    mvwprintw (_win, 0, 2, "%s", _title);
    wrefresh (_win);

    ncurses_subwindow_textview_setup (&_textview);
    ncurses_subwindow_textview_update (&_textview);
}

void ncurses_window_lyrics_down (void)
{
    ncurses_subwindow_textview_down (&_textview);
}

void ncurses_window_lyrics_up (void)
{
    ncurses_subwindow_textview_up (&_textview);
}

void ncurses_window_lyrics_down_half_page (void)
{
    ncurses_subwindow_textview_down_half_page (&_textview);
}

void ncurses_window_lyrics_up_half_page (void)
{
    ncurses_subwindow_textview_up_half_page (&_textview);
}

void ncurses_window_lyrics_down_full_page (void)
{
    ncurses_subwindow_textview_down_full_page (&_textview);
}

void ncurses_window_lyrics_up_full_page (void)
{
    ncurses_subwindow_textview_up_full_page (&_textview);
}

gboolean ncurses_window_lyrics_fetch (const gchar *artist, const gchar *title, NetLyricsService service)
{
    if (service < NET_LYRICS_SERVICE_FIRST || service >= NET_LYRICS_SERVICE_END) return FALSE;
    (void)ncurses_window_lyrics_init (_update); /* reset to defaults */
    if (artist == NULL || title == NULL) {
        /* TODO: error message? */
        return FALSE;
    }
    g_snprintf (_title, ABSOLUTELY_MAX_LINE_LEN, "%s - %-*s", artist, _width-6-(int)g_utf8_strlen (artist, 0)-(int)g_utf8_strlen (title, 0), title);
    if (net_get_lyrics (artist, title, service, _got_lyrics) != 0) {
        return FALSE;
    }
    return TRUE;
}

static gboolean _got_lyrics (gpointer data)
{
    gchar *lyrics = (gchar *)data;

    if (lyrics != NULL && *lyrics == '\0') {
        g_free (lyrics);
        lyrics = NULL;
    }
    if (lyrics == NULL) {
        lyrics = g_strdup (_("Lyrics not found."));
    }
    ncurses_subwindow_textview_resize (&_textview, _width, _subheight, _x, _suby);
    ncurses_subwindow_textview_text (&_textview, lyrics, FALSE, TRUE);

    g_idle_add (_update, NULL);

    net_join_lyrics_thread ();

    return FALSE;
}

