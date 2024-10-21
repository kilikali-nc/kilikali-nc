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

#ifndef _KK_GST_PLAYER_
#define _KK_GST_PLAYER_

#include "song.h"

#define PLAYER_VOLUME_MIN 0
#define PLAYER_VOLUME_MAX 100

typedef enum {
    PLAYER_STATE_NULL,    /* uninitialized */
    PLAYER_STATE_PLAYING,
    PLAYER_STATE_PAUSED,
    PLAYER_STATE_STOPPED,
    PLAYER_STATE_ERROR    /* realloc player */
} PlayerState;

typedef enum {
    PLAYER_MESSAGE_UNKNOWN,
    PLAYER_MESSAGE_ERROR,
    PLAYER_MESSAGE_ABOUT_TO_FINISH,
    PLAYER_MESSAGE_EOS,
    PLAYER_MESSAGE_TAG
} PlayerMessage;

typedef void (*PlayerStatusUpdateFunc)(PlayerMessage mgs, gpointer data);

void player_preinit (int *argc, char **argv[]);

gboolean player_init (PlayerStatusUpdateFunc status_update_func);
void player_free (void);

gint player_set_song (Song *s);

PlayerState player_play (void);
PlayerState player_toggle_playpause (void);
PlayerState player_pause (void);
PlayerState player_stop (void);
gboolean player_seek (gint64 ms); /* Additive seek (+/-) */
gboolean player_seek_to (gint64 ms);
gboolean player_get_duration (gint64 *ms); /* Duration of current track */

/* volume 0-100 */
guint8 player_set_volume (guint8 volume);
guint8 player_volume_up (guint8 num);
guint8 player_volume_down (guint8 num);
guint8 player_volume (void);

PlayerState player_state (void);
gint64 player_get_current_time (void);

gint player_set_sid_tune (gint tune);
#endif

