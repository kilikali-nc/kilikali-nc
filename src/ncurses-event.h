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

#ifndef _NCURSES_EVENT_H_
#define _NCURSES_EVENT_H_

#include <glib.h>
#include <ncurses.h>
#include "util.h"

typedef enum {
    NCURSES_EVENT_TYPE_CH,
    NCURSES_EVENT_TYPE_UTF8,
    NCURSES_EVENT_TYPE_MOUSE,
    NCURSES_EVENT_TYPE_LAST
} NCursesEventType;

typedef struct {
    NCursesEventType type;
    union {
        int ch;
        char utf8[MAX_UTF8_CHAR_SIZE]; /* max utf8 + NULL-termination */
        MEVENT mevent;
    };
    gint size; /* bytes used with utf8 */
} NCursesEvent;

gboolean ncurses_event_idle (gpointer data);

#endif
