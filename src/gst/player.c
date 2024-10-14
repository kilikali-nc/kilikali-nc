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

    #include <gst/audio/gstaudiodecoder.h>
    #include <stdint.h>

    #include "player.h"
    #include "common.h"
    #include "typefind-hack.h"

    #include "../config.h"
    #include "../sid.h"

    static PlayerState _state_change (GstState state);
    static void _on_element_added (GstBin *p0, GstBin *p1, GstElement *e, gpointer data);
    static gboolean _message_handler (GstBus *b, GstMessage *m, gpointer data);

    static GstElement *_playbin = NULL;
    static GstElement *_output = NULL;
    static GstBus *_bus = NULL;
    static PlayerState _state = PLAYER_STATE_NULL;
    static Song *_song = NULL;
    static gint _sid_tune_index = 0;
    static GstElement *_siddec = NULL;
    static guint8 _volume = 100;

    static PlayerStatusUpdateFunc _status_update_func = NULL;

    /* #define DEBUG_GST_PLAYER 1*/

    void player_preinit (int *argc, char **argv[])
    {
        gst_init (argc, argv);
        gst_typefind_hack_init ();
    }

    gboolean player_init (PlayerStatusUpdateFunc status_update_func)
    {
        gint flags = 0;
        _state = PLAYER_STATE_NULL;
        _status_update_func = status_update_func;
        if (_status_update_func == NULL) goto error;

        _playbin = gst_element_factory_make ("playbin", "playbin");
        if (_playbin == NULL) {
            goto error;
        }
        if (strncmp (config.output, "alsa", 4) == 0) {
            _output = gst_element_factory_make ("alsasink", "alsasink");
            if (_output != NULL) {
                if (config.alsa_device != NULL) {
                    g_object_set (_output, "device", config.alsa_device, NULL);
                }
                g_object_set (_playbin, "audio-sink", _output, NULL);
            }
        } else if (strncmp (config.output, "pulse", 5) == 0) {
            _output = gst_element_factory_make ("pulsesink", "pulsesink"); 
            g_object_set (_playbin, "audio-sink", _output, NULL);
        }

        g_object_get (_playbin, "flags", &flags, NULL);
        flags |= GST_PLAYBIN_FLAGS_AUDIO;
        flags &= ~GST_PLAYBIN_FLAGS_VIDEO;
        flags &= ~GST_PLAYBIN_FLAGS_SUBS;
        g_object_set (_playbin, "flags", flags, NULL);

        _bus = gst_element_get_bus (_playbin);
        gst_bus_add_watch (_bus, _message_handler, NULL);

        g_signal_connect (GST_BIN (_playbin), "deep-element-added", G_CALLBACK (_on_element_added), NULL);
        _volume = player_volume ();
        return TRUE;
    error:
        player_free ();
        return FALSE;
    }

    void player_free (void)
    {
        if (_bus != NULL) gst_object_unref (_bus);
        if (_playbin != NULL) {
            gst_element_set_state (_playbin, GST_STATE_NULL);
            gst_object_unref (_playbin);
        }
    _status_update_func = NULL;
}

gint player_set_song (Song *s)
{
    if (s == NULL) return 1;
    if (s->uri == NULL) return 2;
    if (_playbin == NULL) return 3; 

    _song = s;
    (void)player_stop ();

    g_object_set (_playbin, "uri", _song->uri, NULL);

    return 0;
}

PlayerState player_play (void)
{
    return _state_change (GST_STATE_PLAYING);
}

PlayerState player_toggle_playpause (void)
{
    PlayerState state = PLAYER_STATE_ERROR;
    if (_state == PLAYER_STATE_PLAYING) state = player_pause ();
    else if (_state == PLAYER_STATE_PAUSED) state = player_play ();
    return state;
}

PlayerState player_pause (void)
{
    return _state_change (GST_STATE_PAUSED);
}

PlayerState player_stop (void)
{
    return _state_change (GST_STATE_NULL);
}

gboolean player_seek (gint64 ms)
{
    gint64 ns = 0;
    gint64 len = 0;
    if (player_state () != PLAYER_STATE_PLAYING) return FALSE;
    else if (gst_element_query_position (_playbin, GST_FORMAT_TIME, &ns) == FALSE) return FALSE;
    else if (gst_element_query_duration (_playbin, GST_FORMAT_TIME, &len) == FALSE) return FALSE;

    ns = ns + (ms*1000000);
    if (ns < 0) ns = 0;
    else if (ns > len) ns = len;

    return gst_element_seek_simple (_playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT, ns);
}

gboolean player_seek_to (gint64 ms) {
    gint64 ns = ms * 1000000;
    return gst_element_seek_simple (_playbin, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH|GST_SEEK_FLAG_KEY_UNIT, ns);
}

gboolean player_get_duration (gint64 *ms)
{
    gint64 ns;
    gboolean ret = gst_element_query_duration (_playbin, GST_FORMAT_TIME, &ns);
    if (ret) {
        *ms = ns / 1000000;
    }
    return ret;
}

guint8 player_set_volume (guint8 volume)
{
    if (volume > PLAYER_VOLUME_MAX) {
        volume = PLAYER_VOLUME_MAX;
    }
    _volume = volume;
    if (_playbin != NULL) {
        g_object_set (_playbin, "volume", (_volume/100.0), NULL);
    }
    return _volume;
}

guint8 player_volume_up (guint8 num)
{
    guint8 new_volume;
    if ((int)_volume + (int)num > PLAYER_VOLUME_MAX) {
        new_volume = PLAYER_VOLUME_MAX;
    } else {
        new_volume = _volume + num;
    }
    return player_set_volume (new_volume);
}

guint8 player_volume_down (guint8 num)
{
    guint8 new_volume;
    if ((int)_volume - num < PLAYER_VOLUME_MIN) {
        new_volume = PLAYER_VOLUME_MIN;
    } else {
        new_volume = _volume - num;
    }
    return player_set_volume (new_volume); 
}

guint8 player_volume (void)
{
    return _volume;
}

static PlayerState _state_change (GstState state)
{
    gint ret;
    if (_playbin == NULL) return PLAYER_STATE_ERROR;
    ret = gst_element_set_state (_playbin, state);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        _state = PLAYER_STATE_ERROR;
        return _state;
    }

    if (state == GST_STATE_PLAYING) {
        _state = PLAYER_STATE_PLAYING;
    } else if (state == GST_STATE_PAUSED) {
        _state = PLAYER_STATE_PAUSED;
    } else if (state == GST_STATE_NULL) {
        _state = PLAYER_STATE_STOPPED;
    } else {
        _state = PLAYER_STATE_ERROR;
    }
    return _state;
}

PlayerState player_state (void)
{
    return _state;
}

gint64 player_get_current_time (void)
{
    gint64 c = 0;
    gint64 s = -1;
    PlayerState state = player_state ();
    if (state != PLAYER_STATE_PLAYING && state != PLAYER_STATE_PAUSED) return 0;
    if (gst_element_query_position (_playbin, GST_FORMAT_TIME, &c) == TRUE) s = c/1000000;
    return s;
}

gint player_set_sid_tune (gint tune)
{
    if (_song == NULL) return 0;
    else if (tune < 0) tune = 0;
    else if (tune > _song->tunes-1) tune = _song->tunes - 1;
    if (_song->tunes == 0) tune = 0; /* special case, plays only default one*/
    _sid_tune_index = tune;
    return _sid_tune_index;
}

/* This probably is not the best way to get hands to siddec, but siddec is needed to get rid of errors */
static void _on_element_added (GstBin *p0, GstBin *p1, GstElement *e, gpointer data)
{
    gchar *name = gst_element_get_name (e);
#if defined (DEBUG_GST_PLAYER)
    g_critical ("Element: %s", name);
#endif
    if (g_str_has_prefix (name, "siddecfp") == TRUE) {
        _siddec = e;
        g_object_set (G_OBJECT (e),
            "tune", _sid_tune_index,
            "filter", config.sid_filter,
            "sid-model", config.sid_sid_model,
            "force-sid-model", config.sid_force_sid_model,
            "c64-model", config.sid_c64_model,
            "force-c64-model", config.sid_force_c64_model,
            "cia-model", config.sid_cia_model,
            "digi-boost", config.sid_digiboost,
            "sampling-method", config.sid_sampling_method,
            "filter-bias", config.sid_filter_bias,
            "filter-curve-6581", config.sid_filter_curve_6581,
            "filter-curve-8580", config.sid_filter_curve_8580,
            "basic", sid_basic (),
            "kernal", sid_kernal (),
            "chargen", sid_chargen (),
            NULL);
        if (_sid_tune_index > -1) _song->duration = _song->tune_duration[_sid_tune_index];
    } else if (g_str_has_prefix (name, "siddec") == TRUE) {
        _siddec = e;
        g_object_set (G_OBJECT (e), "tune", _sid_tune_index, NULL);
        if (_sid_tune_index > -1) _song->duration = _song->tune_duration[_sid_tune_index];
    }
    g_free (name);
}


static gboolean _message_handler (GstBus *b, GstMessage *m, gpointer data)
{
    PlayerMessage msg = PLAYER_MESSAGE_UNKNOWN;
    gpointer d = NULL;
    Song o;
    memset (&o, 0, sizeof (Song));
    switch (GST_MESSAGE_TYPE (m)) {
        case GST_MESSAGE_ERROR: {
            d = "Error: GStreamer";
            msg = PLAYER_MESSAGE_ERROR;
            break;
        }
        case GST_MESSAGE_EOS: {
            msg = PLAYER_MESSAGE_EOS;
            d = (gpointer)&o; /* dummy */
            break;
        }
        case GST_MESSAGE_TAG: {
            msg = PLAYER_MESSAGE_TAG;
            gst_common_parse_tags (m, &o);
            d = (gpointer)&o;
            break;
        }
        default:
            break;
    }
    if (_status_update_func != NULL && d != NULL) {
         _status_update_func (msg, d);
    }
    return TRUE;
}

