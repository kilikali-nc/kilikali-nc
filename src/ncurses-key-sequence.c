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

#include <inttypes.h>
#include <assert.h>

#include "ncurses-key-sequence.h"
#include "config.h"
#include "log.h"

static char _str_buffer[KEY_SEQUENCE_MAX_LEN + 1];
static uint32_t _str_buffer_len;

const char *ncurses_key_sequence_add (NCursesEvent *event, uint32_t *out_num_repeats,
    gboolean *out_is_num_repeats_specified)
{
    *out_num_repeats = 0;
    *out_is_num_repeats_specified = FALSE;
    if (event->type == NCURSES_EVENT_TYPE_CH) {
        const char *key_name = keyname (event->ch);
        size_t key_name_len = strlen(key_name);
        if (_str_buffer_len + key_name_len >= sizeof(_str_buffer)) {
            ncurses_key_sequence_reset ();
            return NULL;
        }
        if (config_option_key_in_reserved_list (event->ch) == TRUE) {
            ncurses_key_sequence_reset ();
            return NULL;
        }
        memcpy(_str_buffer + _str_buffer_len, key_name, key_name_len);
        _str_buffer_len += key_name_len;
        _str_buffer[_str_buffer_len] = 0;
    } else if  (event->type == NCURSES_EVENT_TYPE_UTF8 && event->size > 1) {
        uint32_t new_str_buffer_len = _str_buffer_len + (uint32_t)event->size;
        if (new_str_buffer_len > sizeof(_str_buffer) - 1) {
            ncurses_key_sequence_reset ();
            return NULL;
        }
        memcpy (&_str_buffer[_str_buffer_len], event->utf8, event->size);
        _str_buffer_len += event->size;
        _str_buffer[_str_buffer_len] = 0;
    } else {
        ncurses_key_sequence_reset ();
        return NULL;
    }
    gboolean only_contains_digits = TRUE;
    uint32_t num_digits = 0;
    for (const char *c = _str_buffer; *c; c = g_utf8_find_next_char(c, NULL)) {
        if (g_unichar_isdigit(g_utf8_get_char(c))) {
            num_digits += 1;
        } else {
            only_contains_digits = FALSE;
            break;
        }
    }
    if (!only_contains_digits) {
        const char *key_combo_str = _str_buffer + num_digits;
        if (config_option_key_prefix_found (key_combo_str) == FALSE) {
            ncurses_key_sequence_reset ();
            return NULL;
        }
    } else {
        return NULL;
    }
    if (num_digits > 0) {
        char digit_buf[sizeof(_str_buffer)];
        memcpy(digit_buf, _str_buffer, num_digits * sizeof(char));
        digit_buf[num_digits] = 0;
        if  (sscanf(digit_buf, "%" PRIu32, out_num_repeats) != 1) {
            ncurses_key_sequence_reset ();
            return NULL;
        }
        *out_is_num_repeats_specified = TRUE;
    } else {
        *out_num_repeats = 1;
    }
    return _str_buffer + num_digits;
}

const gchar *ncurses_key_sequence_str (void)
{
    return _str_buffer;
}

void ncurses_key_sequence_reset (void)
{
    _str_buffer[0] = '\0';
    _str_buffer_len = 0;
}
