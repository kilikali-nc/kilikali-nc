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

#include <ctype.h>

#include "ncurses-subwindow-textview.h"
#include "ncurses-common.h"
#include "ncurses-colors.h"

/* ncurses */
static gchar _tmp[ABSOLUTELY_MAX_LINE_LEN];
static gchar _tmp2[ABSOLUTELY_MAX_LINE_LEN];

static gint _calculate_total_lines (NCursesSubwindowTextview *t);
static gboolean _get_running_line (NCursesSubwindowTextview *t, gchar *text, gint *len);
static void _find_new_position_to_line (NCursesSubwindowTextview *t, gint line);
static gboolean _find_prev_running_line (NCursesSubwindowTextview *t);
static gboolean _find_next_running_line (NCursesSubwindowTextview *t);

gboolean ncurses_subwindow_textview_init (NCursesSubwindowTextview *t)
{
    memset (t, 0, sizeof (NCursesSubwindowTextview));
    return TRUE;
}

gboolean ncurses_subwindow_textview_resize (NCursesSubwindowTextview *t, gint width, gint height, gint x, gint y)
{
    t->width = width;
    t->height = height;
    t->total_lines = _calculate_total_lines(t);
    t->last_line = t->total_lines - t->height;

    ncurses_scroller_page_max_index (&t->scroller, t->total_lines - 1);
    ncurses_scroller_resize (&t->scroller, t->height);

    if (t->win != NULL) ncurses_subwindow_textview_delete (t);
    t->win = newwin (t->height, t->width, y, x);
    if (t->win == NULL) goto resize_error;

    ncurses_colors_pair_set (t->win, COLOR_PAIR_WHITE_BLACK);

    return TRUE;
resize_error:
    return FALSE;
}

void ncurses_subwindow_textview_delete (NCursesSubwindowTextview *t)
{
    if (t->free_text == TRUE) {
        g_free (t->text);
    }
    if (t->win != NULL) delwin (t->win);
    t->win = NULL;
}

void ncurses_subwindow_textview_clear (NCursesSubwindowTextview *t)
{
    wclear (t->win);
}

void ncurses_subwindow_textview_update (NCursesSubwindowTextview *t)
{
    gint len = 0;
    gint y = 0;
    gint i = 0;

    if (t->text == NULL) return;
    else if (t->total_lines < 1) return;
    else if (t->height == 0) return;

    gint line = t->scroller.page_start_index + 1;
    if (line > t->last_line) line = t->last_line;
    _find_new_position_to_line (t, line);
    t->current_line = line;

    for (; i < t->height; i++) {
        if (_get_running_line (t, _tmp, &len) == FALSE) break;
        g_snprintf (_tmp2, ABSOLUTELY_MAX_LINE_LEN, "%-*s", t->width, _tmp);
        mvwprintw (t->win, y++, 0, "%s", _tmp2);
    }
    for (; i < t->height; i++) {
        g_snprintf (_tmp2, ABSOLUTELY_MAX_LINE_LEN, "%-*s", t->width, " ");
        mvwprintw (t->win, y++, 0, "%s", _tmp2);
    }
    wrefresh (t->win);
}

void ncurses_subwindow_textview_setup (NCursesSubwindowTextview *t)
{
    if (t->text == NULL) return;
    else if (t->height == 0) return;

    ncurses_scroller_page_height (&t->scroller, t->height);
    t->running = t->current;

}

void ncurses_subwindow_textview_down (NCursesSubwindowTextview *t)
{
    ncurses_scroller_scroll_down (&t->scroller);
}

void ncurses_subwindow_textview_up (NCursesSubwindowTextview *t)
{
    ncurses_scroller_scroll_up (&t->scroller);
}

void ncurses_subwindow_textview_down_half_page (NCursesSubwindowTextview *t)
{
    ncurses_scroller_scroll_down_half_page (&t->scroller);
}

void ncurses_subwindow_textview_up_half_page (NCursesSubwindowTextview *t)
{
    ncurses_scroller_scroll_up_half_page (&t->scroller);
}

void ncurses_subwindow_textview_down_full_page (NCursesSubwindowTextview *t)
{
    ncurses_scroller_scroll_down_full_page (&t->scroller);
}

void ncurses_subwindow_textview_up_full_page (NCursesSubwindowTextview *t)
{
    ncurses_scroller_scroll_up_full_page (&t->scroller);
}

gint _calculate_total_lines (NCursesSubwindowTextview *t)
{
    if (t->text == NULL) return 0;
    else if (t->width < 1 || t->height < 1) return 0;
    gint lines = 0;
    gboolean not_end;
    t->running = t->current = t->text;
    do {
        not_end = _find_next_running_line (t);
        lines++;
    } while (not_end == TRUE);
    return lines;
}

gboolean _get_running_line (NCursesSubwindowTextview *t, char *text, gint *len)
{
    const gchar *c = t->running;
    gint w = 0;
    size_t s = strlen(t->text);
    if (s < 1) return FALSE;
    const gchar *end = &t->text[s];
    if (c == end) return FALSE;
    *len = 0;
    for (; c != end && w < t->width; c++) {
        if (*c == '\n') {
            c++;
            break;
        }
        text[w++] = (gchar)*c;
    }
    t->running = c;
    *len = w;
    text[w] = '\0';
    if (c == end) return FALSE;
    return TRUE;
}

void _find_new_position_to_line (NCursesSubwindowTextview *t, gint line)
{
    t->running = t->current;
    gint delta = line - t->current_line;
    if (delta == 0) return;
    else if (delta > 0) { // forward
        for (gint i = 0; i < delta; i++) {
            if (_find_next_running_line (t) == FALSE) {
                break;
            }
        }
    } else if (delta < 0) { // backwards
        for (gint i = 0; i < -delta; i++) {
            if (_find_prev_running_line (t) == FALSE) {
                break;
            }
        }
    }
    t->current = t->running;
}

gboolean _find_next_running_line (NCursesSubwindowTextview *t)
{
    gboolean ret = TRUE;
    const gchar *c = t->running;
    gint w = 0;
    size_t len = strlen(t->text);
    if (len < 1) return FALSE;
    const gchar *end = &t->text[len];
    if (c == end) ret = FALSE;
    for (; c != end && w < t->width; c++) {
        w++;
        if (*c == '\n') {
            c++;
            break;
        }
    }
    if (c == end) ret = FALSE;
    t->running = c;
    return ret;
}

gboolean _find_prev_running_line (NCursesSubwindowTextview *t)
{
    gboolean ret = TRUE;
    const gchar *c = t->running - 2;
    gint w = 0;
    const gchar *start = &t->text[0];
    if (c <= start) {
        t->running = start;
        return FALSE;
    }
    for (; c != start && w < t->width; c--) {
        w++;
        if (*c == '\n') {
            c++;
            break;
        }
    }
    if (c == start) ret = FALSE;
    t->running = c;
    return ret;
}

gboolean ncurses_subwindow_textview_text (NCursesSubwindowTextview *t, const gchar *text, gboolean alloc_text, gboolean free_text)
{
    if (text == NULL) goto set_text_error;

    if (t->free_text == TRUE) {
        g_free (t->text);
    }
    t->free_text = FALSE;

    if (alloc_text == TRUE) {
        t->text = g_strdup (text);
        if (t->text == NULL) goto set_text_error;
        t->free_text = TRUE;
    } else {
        t->text = (gchar *)text;
        t->free_text = free_text;
    }

    ncurses_scroller_init (&t->scroller, 0, 0);
    t->current = t->text;
    t->running = t->current;
    t->total_lines = _calculate_total_lines(t);
    t->last_line = t->total_lines - t->height;
    ncurses_scroller_page_max_index (&t->scroller, t->total_lines - 1);

    return TRUE;
set_text_error:
    return FALSE;
}

