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
#include <assert.h>

#include "ncurses-common.h"
#include "ncurses-colors.h"
#include "ncurses-window-command-prompt.h"
#include "ncurses-key-sequence.h"
#include "config.h"
#include "playlist.h"
#include "player.h"
#include "util.h"

/* ncurses */
static gint _width = 0;
static gint _height = 0;

static WINDOW *_win = NULL;

gboolean ncurses_window_command_prompt_init (void)
{
    return TRUE;
}

gboolean ncurses_window_command_prompt_resize (gint width, gint height, gint x, gint y)
{
    _width = width;
    _height = height;

    _win = newwin (_height, _width, y, x);
    if (_win == NULL) goto resize_error;

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_window_command_prompt_delete (void)
{
    if (_win != NULL) delwin (_win);
    _win = NULL;
}

void ncurses_window_command_prompt_clear (void)
{
    wclear (_win);
}

void ncurses_window_command_prompt_update (NCursesWindowCommandPromptMode mode, const gchar *line, gint cursor_position)
{
    ncurses_colors_pair_set (_win, COLOR_PAIR_WHITE_BLACK);
    if (mode == NCURSES_WINDOW_COMMAND_PROMPT_MODE_COMMAND) {
        assert(config.key_command_mode.num_keys > 0);
        mvwprintw (_win, 0, 1, "%s%-*s", config.key_command_mode.keys[0],
            _width, line);
    } else if (mode == NCURSES_WINDOW_COMMAND_PROMPT_MODE_SEARCH) {
        assert(config.key_search_mode.num_keys > 0);
        mvwprintw (_win, 0, 1, "%s%-*s", config.key_search_mode.keys[0], _width,
            line);
    } else { // _NONE
        mvwprintw (_win, 0, 1, "%-*s", _width, " ");
        const gchar *key_seq = ncurses_key_sequence_str ();
        size_t key_seq_len = g_utf8_strlen (key_seq, -1);
        ncurses_colors_pair_set (_win, COLOR_PAIR_WHITE_BLACK);
        mvwprintw (_win, 0, _width - key_seq_len, "%-*s", _width, key_seq);
    }

    if (cursor_position > -1) {
        wmove (_win, 0, 2 + cursor_position);
    } else {
        wmove (_win, 0, 1);
    }

    wrefresh (_win);
}
