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

#ifndef _NCURSES_KEY_SEQUENCE_H_
#define _NCURSES_KEY_SEQUENCE_H_

#include <glib.h>
#include "ncurses-event.h"
#include "config.h"

#define KEY_SEQUENCE_MAX_KEYS CONFIG_MAX_KEY_NAME_LEN
#define KEY_SEQUENCE_MAX_DIGITS 10
#define KEY_SEQUENCE_MAX_LEN (KEY_SEQUENCE_MAX_KEYS + KEY_SEQUENCE_MAX_DIGITS)

/* Returns NULL if no key match is found. Otherwise, returns the keybind name
 * and the number of repeats in out_num_repeats. out_is_num_repeats_specified
 * will be set to TRUE if the number of repeats was defined by the user, and
 * will otherwise be set to false with out_num_repeats defaulting to 1. */
const char *ncurses_key_sequence_add (NCursesEvent *e, uint32_t *out_num_repeats,
    gboolean *out_is_num_repeats_specified);
const gchar *ncurses_key_sequence_str (void);
void ncurses_key_sequence_reset (void);

#endif
