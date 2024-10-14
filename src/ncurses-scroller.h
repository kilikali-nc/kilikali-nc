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

#ifndef _NCURSES_SCROLLER_H_
#define _NCURSES_SCROLLER_H_

#include <glib.h>
/*
 * Struct to keep track of scrolling and selection of lines of view.
 */
typedef struct ncurses_scroller
{
    gint selection_start_index; /* Selection start index. Differs from selection_end_index when multiple lines are selected */
    gint selection_end_index;   /* Selection end index. Current selected index. Page index follows this one */
    gboolean selection_toggle;  /* Selection toggle mode. When on start index is not updated while moving */
    gint page_start_index;      /* Page start index. What is index page starts from. */
    gint page_end_index;        /* Page end indes. What is the index page ends. */
    gint page_max_index;        /* Page max index. Number of lines - 1 */
    gint page_height;           /* Page height. Lines in the the one page */
} NCursesScroller;

/*
 * Init sets page height, max page index and everything else to zero
 */
void ncurses_scroller_init (NCursesScroller *s, gint page_height, gint max_index);

/*
 * Resize changes page height
 * Might change page index or selections.
 */
void ncurses_scroller_resize (NCursesScroller *s, gint page_height);
/*
 * Sets new max index for current view. Remove line from current view is one case.
 * Might change page index.
 */
void ncurses_scroller_page_max_index (NCursesScroller *s, gint max_index);

/*
 * Sets page height. Use only if at resize page max index is not known
 */
void ncurses_scroller_page_height (NCursesScroller *s, gint height);

/* Select one or mode lines. */
void ncurses_scroller_selection_start_and_end (NCursesScroller *s, gint start_index, gint end_index);
/* Selection mode setter */
void ncurses_scroller_selection_toggle (NCursesScroller *s, gboolean value);

/* Makes sure that page start index is in range */
void ncurses_scroller_ensure_page_start_index (NCursesScroller *s);

/* Move selection (current) index. Possibly selects lines if selection toggle is true */
void ncurses_scroller_up (NCursesScroller *s);
void ncurses_scroller_down (NCursesScroller *s);
void ncurses_scroller_up_half_page (NCursesScroller *s);
void ncurses_scroller_down_half_page (NCursesScroller *s);
void ncurses_scroller_up_full_page (NCursesScroller *s);
void ncurses_scroller_down_full_page (NCursesScroller *s);
void ncurses_scroller_top (NCursesScroller *s);
void ncurses_scroller_bottom (NCursesScroller *s);

/* Move pages index. Handles also selection (current) index */
void ncurses_scroller_scroll_up (NCursesScroller *s);
void ncurses_scroller_scroll_down (NCursesScroller *s);
void ncurses_scroller_scroll_up_half_page (NCursesScroller *s);
void ncurses_scroller_scroll_down_half_page (NCursesScroller *s);
void ncurses_scroller_scroll_up_full_page (NCursesScroller *s);
void ncurses_scroller_scroll_down_full_page (NCursesScroller *s);
void ncurses_scroller_scroll_top (NCursesScroller *s);
void ncurses_scroller_scroll_bottom (NCursesScroller *s);
void ncurses_scroller_scroll_center_to_cursor (NCursesScroller *s);
void ncurses_scroller_jump (NCursesScroller *s, gint index);

/* Move without changing selection position */
void ncurses_scroller_page_start (NCursesScroller *s, gint page_start);

/* For returning to the old position after cancelling a search. */
void ncurses_scroller_move_to (NCursesScroller *s, gint page_start_index,
    gint page_end_index, gint selection_start_index, gint selection_end_index);

#endif
