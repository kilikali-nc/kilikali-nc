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

#include <assert.h>
#include "ncurses-scroller.h"

static void _scroll_page (NCursesScroller *s, gint steps);

static void _test_and_set_page_up (NCursesScroller *s, gint steps);
static void _test_and_set_page_down (NCursesScroller *s, gint steps);

void ncurses_scroller_init (NCursesScroller *s, gint page_height, gint max_index)
{
    memset (s, 0, sizeof (NCursesScroller));
    s->page_max_index = max_index;
    s->page_height = page_height;
    s->page_end_index = page_height-1;
    s->selection_toggle = FALSE;
}

void ncurses_scroller_resize (NCursesScroller *s, gint page_height)
{
    gint jump_to_index = 0;
    s->page_height = 0;
    if (s->selection_end_index > -1) {
        jump_to_index = s->selection_end_index;
    } else {
        jump_to_index = s->page_start_index;
    }
    s->page_start_index = 0; /* default */

    if ((s->page_max_index + 1) > 0) {
        s->page_height = page_height;
        s->page_start_index = jump_to_index;
    }
    s->page_end_index = s->page_start_index + s->page_height - 1;
}

void ncurses_scroller_page_max_index (NCursesScroller *s, gint max_index)
{
    s->page_max_index = max_index;
}

void ncurses_scroller_page_height (NCursesScroller *s, gint height)
{
    s->page_height = height;
}

void ncurses_scroller_ensure_page_start_index (NCursesScroller *s)
{
    if (s->page_height >= (s->page_max_index + 1)) {
        s->page_start_index = 0;
        return;
    }
    if (s->selection_end_index < 0) {
        s->page_start_index = s->selection_end_index;
    } else if (s->selection_end_index < s->page_start_index) {
        s->page_start_index = s->selection_end_index;
    } else if (s->selection_end_index > s->page_max_index) {
        s->page_start_index = s->page_max_index;
    }
    s->page_end_index = s->page_start_index + s->page_height - 1;
}

void ncurses_scroller_selection_start_and_end (NCursesScroller *s, gint start_index, gint end_index)
{
    gint old_end_index = s->selection_end_index;
    s->selection_start_index = start_index;
    s->selection_end_index = end_index;
    if (old_end_index < end_index) {
        _test_and_set_page_down (s, end_index - old_end_index);
    } else if (old_end_index > end_index) {
        _test_and_set_page_up (s, old_end_index - end_index);
    }
}

void ncurses_scroller_selection_toggle (NCursesScroller *s, gboolean value)
{
    s->selection_toggle = value;
    if (!value) {
        s->selection_start_index = s->selection_end_index;
    }
}

void ncurses_scroller_up (NCursesScroller *s)
{
    if (s->selection_end_index > 0) {
        s->selection_end_index--;
    }
    if (s->selection_toggle == FALSE) {
        s->selection_start_index = s->selection_end_index;
    }
    _test_and_set_page_up (s, 1);
}

void ncurses_scroller_down (NCursesScroller *s)
{
    if (s->selection_end_index < s->page_max_index) {
        s->selection_end_index++;
    }
    if (s->selection_toggle == FALSE) {
        s->selection_start_index = s->selection_end_index;
    }
    _test_and_set_page_down (s, 1);
}

void ncurses_scroller_up_half_page (NCursesScroller *s)
{
    int half_page = s->page_height / 2;
    s->selection_end_index -= half_page;
    if (s->selection_end_index < 0) {
        s->selection_end_index = 0;
    }
    if (s->selection_toggle == FALSE) {
        s->selection_start_index = s->selection_end_index;
    }
    s->page_start_index -= half_page;
    s->page_end_index -= half_page;
    _test_and_set_page_up (s, 0);
}

void ncurses_scroller_down_half_page (NCursesScroller *s)
{
    int half_page = s->page_height / 2;
    s->selection_end_index += half_page;
    if (s->selection_end_index > s->page_max_index) {
        s->selection_end_index = s->page_max_index;
    }
    if (s->selection_toggle == FALSE) {
        s->selection_start_index = s->selection_end_index;
    }
    s->page_start_index += half_page;
    s->page_end_index += half_page;
    int max = s->page_max_index + 1 - s->page_height;
    if (s->page_start_index > max) {
        s->page_start_index = max;
        s->page_end_index = s->page_start_index + s->page_height;
    }
}

void ncurses_scroller_up_full_page (NCursesScroller *s)
{
    s->selection_end_index -= s->page_height;
    if (s->selection_end_index < 0) {
        s->selection_end_index = 0;
    }
    if (s->selection_toggle == FALSE) {
        s->selection_start_index = s->selection_end_index;
    }
    _test_and_set_page_up (s, s->page_height);
}

void ncurses_scroller_down_full_page (NCursesScroller *s)
{
    s->selection_end_index += s->page_height;
    if (s->selection_end_index > s->page_max_index) {
        s->selection_end_index = s->page_max_index;
    }
    if (s->selection_toggle == FALSE) {
        s->selection_start_index = s->selection_end_index;
    }
    _test_and_set_page_down (s, s->page_height);
}

void ncurses_scroller_top (NCursesScroller *s)
{
    s->selection_end_index = 0;
    if (s->selection_toggle == FALSE) {
        s->selection_start_index = 0;
    }
    _test_and_set_page_up (s, s->page_max_index + 1);
}

void ncurses_scroller_bottom (NCursesScroller *s)
{
    s->selection_end_index = s->page_max_index;
    if (s->selection_toggle == FALSE) {
        s->selection_start_index = s->selection_end_index;
    }
    if (s->page_max_index + 1 <= s->page_height) {
        return;
    }
    s->page_start_index = s->page_max_index + 1 - s->page_height;
    assert(s->page_start_index >= 0);
}

void ncurses_scroller_scroll_up (NCursesScroller *s)
{
    _scroll_page (s, -1);
}

void ncurses_scroller_scroll_down (NCursesScroller *s)
{
    _scroll_page (s, 1);
}

void ncurses_scroller_scroll_up_half_page (NCursesScroller *s)
{
    _scroll_page (s, -s->page_height/2);
}

void ncurses_scroller_scroll_down_half_page (NCursesScroller *s)
{
    _scroll_page (s, s->page_height/2);
}

void ncurses_scroller_scroll_up_full_page (NCursesScroller *s)
{
    _scroll_page (s, -s->page_height);
}

void ncurses_scroller_scroll_down_full_page (NCursesScroller *s)
{
    _scroll_page (s, s->page_height);
}

void ncurses_scroller_scroll_top (NCursesScroller *s)
{
    _scroll_page (s, -s->page_max_index);
}

void ncurses_scroller_scroll_bottom (NCursesScroller *s)
{
    _scroll_page (s, s->page_max_index);
}

void ncurses_scroller_scroll_center_to_cursor (NCursesScroller *s)
{
    gint new_start_index = s->selection_end_index - s->page_height/2;
    if (new_start_index < 0) {
        new_start_index = 0;
    }
    s->page_start_index = new_start_index;
    s->page_end_index = s->page_start_index + s->page_height - 1;
}

void ncurses_scroller_jump (NCursesScroller *s, gint index)
{
    if (index < 0) {
        index = 0;
    } else if (index > s->page_max_index) {
        index = s->page_max_index;
    }
    if (s->selection_toggle) {
        s->selection_end_index = index;
    } else {
        s->selection_start_index = index;
        s->selection_end_index = index;
    }
    if (s->selection_end_index > s->page_end_index) {
        s->page_start_index = s->selection_end_index + 1 - s->page_height;
    } else if (s->selection_end_index < s->page_start_index) {
        s->page_start_index = s->selection_end_index;
    }
    s->page_end_index = s->page_start_index + s->page_height - 1;
    if (s->page_end_index < 0) {
        s->page_end_index = 0;
    }
}

void ncurses_scroller_move_to(NCursesScroller *s, gint page_start_index,
    gint page_end_index, gint selection_start_index, gint selection_end_index)
{
    s->page_start_index = page_start_index;
    s->page_end_index = page_end_index;
    s->selection_start_index = selection_start_index;
    s->selection_end_index = selection_end_index;
}

void ncurses_scroller_page_start (NCursesScroller *s, gint page_start)
{
    s->page_start_index = page_start;
    s->page_end_index = page_start + s->page_height - 1;
    if (s->page_end_index < 0) {
        s->page_end_index = 0;
    }
}

static void _scroll_page (NCursesScroller *s, gint steps)
{
    if (steps == 0) {
        return;
    }
    s->page_start_index += steps;
    //s->page_end_index += steps;
    if (steps < 0) {
        _test_and_set_page_up (s, 0);
        if (s->selection_end_index > s->page_end_index) {
            s->selection_end_index = s->page_end_index;
        }
    } else if (steps > 0) {
        _test_and_set_page_down (s, 0);
        if (s->selection_end_index < s->page_start_index) {
            s->selection_end_index = s->page_start_index;
        }
    }
    if (s->selection_toggle == FALSE) {
        s->selection_start_index = s->selection_end_index;
    }
}

static void _test_and_set_page_down (NCursesScroller *s, gint steps)
{
    /* if whole page/list fits to screen */
    if (s->page_max_index < s->page_height) {
        return;
    }
    /* if selection end index (max) is greater than what fits to current page */
    if (s->selection_end_index > s->page_end_index /* (s->page_start_index + s->page_height - 1)*/) {
        s->page_start_index += steps;
    }
    /* is pages start index is greater than max index of whole list */
    if (s->page_start_index > s->page_max_index) {
        s->page_start_index = s->page_max_index;
    }
    /* update pages end index  */
    s->page_end_index = s->page_start_index + s->page_height - 1;
}

static void _test_and_set_page_up (NCursesScroller *s, gint steps)
{
    /* if whole page fits to screen */
    if (s->page_max_index < s->page_height) {
        return;
    }
    if (s->selection_end_index < s->page_start_index) {
        s->page_start_index -= steps;
    }
    if (s->page_start_index < 0) {
        s->page_start_index = 0;
    }
    s->page_end_index = s->page_start_index + s->page_height - 1;
}
