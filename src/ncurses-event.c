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
#include <fcntl.h>
#include <unistd.h>

#include "ncurses-event.h"
#include "ncurses-screen.h"
#include "ncurses-common.h"

gboolean ncurses_event_idle (gpointer data)
{
    static const gchar *end;
    static gint pos = 0;
    static NCursesEvent e; /* NULL terminated utf8 */
    gboolean send = FALSE;

    while (1) {
        int ch = getch ();
        /* ncurses mouse */
        if (ch == KEY_MOUSE) {
            if (getmouse (&e.mevent) == OK) {
                e.type = NCURSES_EVENT_TYPE_MOUSE;
                send = TRUE;
            }
        /* utf8 */
        } if (ch >= 0 && ch <= 0xff) { /* FIXME: check is this enough!!! */
            e.utf8[pos++] = (char)ch;
            if (pos > 6) { /* Should not happen, but */
                pos = 0;
            }
            e.utf8[pos] = '\0';
            if (g_utf8_validate (e.utf8, -1, &end) == TRUE) {
                if (pos > 1) {
                    e.type = NCURSES_EVENT_TYPE_UTF8;
                } else { // one byte as ch
                    e.type = NCURSES_EVENT_TYPE_CH;
                    e.ch = (int)e.utf8[0];
                }
                e.size = pos;
                send = TRUE;
                pos = 0;
            } else {
                continue; /* no need for sleep */
            }
        /* ncurses keys */
        } else if (ch != -1) {
            e.type = NCURSES_EVENT_TYPE_CH;
            e.ch = ch;
            e.size = 2;
            send = TRUE;
            pos = 0;
        }
        break;
    }
    if (send == TRUE) {
        ncurses_screen_event (&e);
    }
    return TRUE;
}

