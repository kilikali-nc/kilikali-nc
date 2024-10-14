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

#include <gst/gst.h>
#include <libintl.h>
#define _(String) gettext (String)

#include "common.h"
#include "../inspector.h"
#include "../playlist.h"
#include "../playlist-pls.h"
#include "../net.h"
#include "../util.h"
#include "../sid.h"

/*#define DEBUG_GST_INSPECTOR 1
 */

#define ABSOLUTELY_MAX_STR_LEN 4096

gchar _status_str[ABSOLUTELY_MAX_STR_LEN] = "";

const gchar *_supported_streams[] = { "http://", "https://", "smb://", "ftp://", "ssh://" };

static void _update_status (GList *l);
static GList *_run_path (const gchar *path);
static GList *_add_dir (const gchar *dirpath, GList **dirs);
static GList *_try_add_file (const gchar *filepath);
static GList *_add_uri (const gchar *uri);
static void _on_new_pad (GstElement *src_element, GstPad *pad, GstElement *sink_element);
static void _on_element_added (GstBin *p0, GstBin *p1, GstElement *e, gpointer data);
static void _check_is_sid (const GstCaps *caps, Song *s);


#if defined(DEBUG_GST_INSPECTOR)
static gint _count = 0;
#endif
static GstElement *_pipeline = NULL;
static GstElement *_decoder = NULL;
static GstElement *_fakesink = NULL;
static GstCaps *_caps = NULL;
static GstBus *_bus = NULL;
static gulong _signal_handle = 0;

static ScreenStatusUpdateFunc _status_update_func = NULL;

static GstElement *_siddec = NULL;
static gboolean _is_siddecfp = FALSE;

gboolean inspector_init (ScreenStatusUpdateFunc status_update_func)
{
    sid_init ();
    _status_update_func = status_update_func;

    _caps = gst_caps_new_simple ("audio/x-raw", "rate", GST_TYPE_INT_RANGE, 1, 2147483647, NULL); /* audio */
    if (_caps == NULL) goto error;

    _pipeline = gst_pipeline_new ("pipeline");
    if (_pipeline == NULL) goto error;

    _bus = gst_element_get_bus (_pipeline);

    _decoder = gst_element_factory_make ("uridecodebin", "uridecodebin");
    if (_decoder == NULL) {
        g_print ("make sure you have installed gst-plugins-base (uridecoder)\n");
        goto error;
    }
    gst_bin_add (GST_BIN (_pipeline), _decoder);

    g_signal_connect (_decoder, "deep-element-added", G_CALLBACK (_on_element_added), NULL);

    _fakesink = gst_element_factory_make ("fakesink", "fakesink");
    if (_fakesink == NULL) goto error; 
    gst_bin_add (GST_BIN (_pipeline), _fakesink);

    _signal_handle = g_signal_connect (_decoder, "pad-added", G_CALLBACK (_on_new_pad), _fakesink);

    return TRUE;
error:
    inspector_free ();
    return FALSE;
}

void inspector_free (void)
{
    if (_decoder != NULL) g_signal_handler_disconnect (_decoder, _signal_handle);
    if (_pipeline != NULL) {
        gst_element_set_state (_pipeline, GST_STATE_NULL);
        gst_object_unref (_pipeline);
    }
    _pipeline = _decoder = _fakesink = NULL;
    if (_caps != NULL) gst_caps_unref (_caps);
    _caps = NULL;
    sid_free ();
}

const gchar *inspector_status (void)
{
    return _status_str;
}

static void _update_status (GList *l)
{
    guint len = g_list_length (l);
    if (len > 1) g_snprintf (_status_str, ABSOLUTELY_MAX_STR_LEN-1, _("%d items added to the playlist."), len);
    else if (len > 0) g_snprintf (_status_str, ABSOLUTELY_MAX_STR_LEN-1, _("One item added to the playlist."));
    else g_snprintf (_status_str, ABSOLUTELY_MAX_STR_LEN-1, " ");
    if (_status_update_func != NULL) _status_update_func ();
}

GList *inspector_run (gchar *path)
{
    gint i;
    GList *l = NULL;
    const gchar *p = g_strstrip (path); /* path must be null terminated string */
    for (i = 0; i < sizeof (_supported_streams) / sizeof (char *); i++) {
        if (g_str_has_prefix (p, _supported_streams[i]) == TRUE) {
            if (g_str_has_suffix (p, ".pls") == TRUE) {
                gchar *content = net_get ((const gchar *)p);
                l = playlist_pls_parse_raw (content);
                g_free (content);
            } else {
                l = _add_uri (p);
            }
            _update_status (l);
            return l;
        }
    }
    l = _run_path (p);
    _update_status (l);
    return l;
}

GList *inspector_add_no_check (gchar *path)
{
    gboolean bret;
    gint i;
    gchar *uri = NULL;
    GList *l = NULL;
    if (gst_uri_is_valid (path)) {
        uri = g_strdup (path);
    } else {
        uri = gst_filename_to_uri (path, NULL);
    }
    if (uri == NULL) return NULL;
    Song *s = song_new (uri);
    if (s == NULL) goto no_check_error;
    const gchar *p = g_strstrip (uri); /* path must be null terminated string */
    for (i = 0; i < sizeof (_supported_streams) / sizeof (char *); i++) {
        if (g_str_has_prefix (p, _supported_streams[i]) == TRUE) {
            song_set_type (s, SONG_TYPE_STREAM);
            l = g_list_append (l, s);
            g_free (uri);
            return l;
        }
    }
    /* for normal files normal check */
    song_set_type (s, SONG_TYPE_FILE);
    bret = inspector_try_uri (uri, s);
    if (bret == FALSE) goto no_check_error;
    else l = g_list_append (l, s);
    g_free (uri);
    return l;
no_check_error:
    g_free (uri);
    if (s != NULL) song_delete (s);
    return NULL;
}

static void _check_is_sid (const GstCaps *caps, Song *s)
{
    if (caps == NULL) return;
    else if (s == NULL) return;
    else if (gst_caps_is_any (caps) || gst_caps_is_empty (caps)) return;
    else if (gst_caps_get_size (caps) != 1) return;

    GstStructure *gs = gst_caps_get_structure (caps, 0);
    if (gs == NULL) return;
    const gchar *name = gst_structure_get_name (gs);
    if (name == NULL) return;
//    g_critical ("%s", name);
    if (strncmp (name, "audio/x-sid", 12) == 0 || strncmp (name, "audio/x-rsid", 12) == 0) {
        song_set_type (s, SONG_TYPE_SID);
    }
}

gboolean inspector_try_uri (gchar *uri, Song *s)
{
    gboolean bret = FALSE;
    gint ret;
    gint64 duration = 0;
    GstMessage *msg = NULL;

    _siddec = NULL;
    g_object_set (_decoder, "uri", uri, "caps", _caps, NULL);

    gst_element_set_state (_pipeline, GST_STATE_PAUSED);

    while (TRUE) {
        if (msg != NULL) gst_message_unref (msg);
        msg = gst_bus_timed_pop_filtered (GST_ELEMENT_BUS (_pipeline),
                GST_CLOCK_TIME_NONE,
                GST_MESSAGE_ASYNC_DONE | GST_MESSAGE_TAG | GST_MESSAGE_ERROR);
        if (msg == NULL) break;
        if (s->type != SONG_TYPE_STREAM && GST_MESSAGE_TYPE (msg) == GST_MESSAGE_TAG) {
            gst_common_parse_tags (msg, s);
        }
        break;
    }

    if (msg != NULL && GST_MESSAGE_TYPE (msg) != GST_MESSAGE_ERROR) {
        bret = TRUE;
        if (s->type == SONG_TYPE_FILE) {
            (void)gst_element_query_duration (_pipeline, GST_FORMAT_TIME, &duration);
            ret = song_set_duration (s, duration/1000000);
        }
        /* check is file sid */
        GstElement *e = gst_bin_get_by_name (GST_BIN (_decoder), "typefind");
        if (e != NULL) {
            GstCaps *caps = NULL;
            g_object_get (e, "caps", &caps, NULL);
            _check_is_sid (caps, s);
            if (s->type == SONG_TYPE_SID) {
                guint tunes = 0;
                if (_is_siddecfp == TRUE && _siddec != NULL) {
                    g_object_get (G_OBJECT (_siddec), "n-tunes", &tunes, NULL);
                }
                sid_setup_song (s, tunes);
            }
        }
#if defined(DEBUG_GST_INSPECTOR)
        g_print ("success: %d, URI: %s\n", ++_count, uri);
#endif
    }
#if defined(DEBUG_GST_INSPECTOR)
    else {
        g_print ("fail: %d, URI: %s\n", ++_count, uri);
    }
#endif
    gst_element_set_state (_pipeline, GST_STATE_NULL);

    if (msg != NULL) gst_message_unref (msg);
    (void)ret;
    return bret;
}


static GList *_add_uri (const gchar *uri)
{
    GList *l = NULL;
    gchar *ptitle;
    gchar *tmp = NULL;
    gboolean bret = FALSE;
    size_t len = strlen (uri);
    if (len < 4) goto uri_error; /* is long enough */
    tmp = g_strdup (uri);
    if (tmp == NULL) goto uri_error; 
    ptitle = tmp;
    while (*ptitle != ' ' && *ptitle != '\0') ptitle++; /* find next space */
    if (*ptitle == ' ') {
        *ptitle = '\0'; /* actual uri */
        ptitle++;
    }
    while (*ptitle == ' ' && *ptitle != '\0') ptitle++; /* removes extra spaces */
    if (*ptitle == '\0') ptitle = tmp; /* use uri as title */

    if (gst_uri_is_valid (tmp)) {
        Song *s = song_new (tmp);
        if (s == NULL) return NULL;
        song_set_type (s, SONG_TYPE_STREAM);
        bret = inspector_try_uri (tmp, s);
        song_set_stream_title (s, ptitle);
        if (bret == FALSE) song_delete (s);
        else l = g_list_prepend (l, s);
    }
uri_error:
    if (tmp != NULL) g_free (tmp);
    return l;
}

static GList *_run_path (const gchar *path)
{
    GList *l = NULL;
    GList *l0 = NULL;
    GList *dirs = NULL;
    GList *dir = NULL;

    if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
        dirs = g_list_prepend (dirs, g_strdup (path));
        for (dir = g_list_last (dirs); dir != NULL; dir = dir->prev) {
            g_snprintf (_status_str, ABSOLUTELY_MAX_STR_LEN-1, _("%c Scanning dir: %s"), util_progress (), (gchar *)dir->data);
            if (_status_update_func != NULL) _status_update_func ();
            l0 = _add_dir (dir->data, &dirs);
            g_free (dir->data);
            if (l0 != NULL) l = g_list_concat (l, l0);
        }
        g_list_free (dirs);
    } else {
        l0 = _try_add_file (path);
        if (l0 != NULL) l = g_list_concat (l, l0);
    }
    l = g_list_sort (l, song_sort_by_path); /* FIXME: !!!*/
    return l;
}

static GList *_add_dir (const gchar *dirpath, GList **dirs)
{
    GList *l = NULL;
    GList *l0 = NULL;
    const gchar *name;
    GDir *dir;
    GError *error = NULL;

    dir = g_dir_open (dirpath, 0, &error);
    if (error != NULL) { /* FIXME: Files also !!!*/
        return NULL;
    }

    while ((name = g_dir_read_name (dir)) != NULL) {
        gchar path[ABSOLUTELY_MAX_STR_LEN];
        // FIXME: is there better way to concat paths
        g_snprintf (path, ABSOLUTELY_MAX_STR_LEN, "%s%s%s", dirpath, G_DIR_SEPARATOR_S, name);
        if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
            *dirs = g_list_prepend (*dirs, g_strdup (path));
        } else {
            l0 = _try_add_file (path);
            if (l0 != NULL) {
                l = g_list_concat (l, l0);
            }
        }
    }

    g_dir_close (dir);

    return l;
}


static void _on_new_pad (GstElement *src_element, GstPad *pad, GstElement *sink_element)
{
    GstPad *sinkpad = gst_element_get_static_pad (sink_element, "sink");
    if (!gst_pad_is_linked (sinkpad)) {
        if (gst_pad_link (pad, sinkpad) != GST_PAD_LINK_OK)
            g_error ("Failed to link pads!");
    }
    gst_object_unref (sinkpad);
}

static GList *_try_add_file (const gchar *filepath0)
{
    gchar *uri = NULL;
    GList *l = NULL;
    Song *s = NULL;
    gint ret;
    gboolean bret = FALSE;
    gchar filepath[PATH_MAX] = "";
    if (FALSE == util_expand_tilde (filepath0, filepath)) return NULL;

    /* use libmagic to check mime */
    if (util_is_possibly_supported_file (filepath) == FALSE) goto try_add_file_error;

    if (gst_uri_is_valid (filepath)) {
        uri = g_strdup (filepath);
    } else {
        uri = gst_filename_to_uri (filepath, NULL);
    }
    if (uri == NULL) goto try_add_file_error;

    s = song_new (uri);
    if (s == NULL) goto try_add_file_error;

    ret = song_set_type (s, SONG_TYPE_FILE);
    bret = inspector_try_uri (uri, s);
try_add_file_error:
    if (bret == FALSE) song_delete (s);
    else if (s != NULL) l = g_list_prepend (l, s);
    g_free (uri);
    (void)ret;
    return l;
}

#if defined (DEBUG_GST_INSPECTOR)
static void _on_have_type (GstElement * typefind, guint probability, GstCaps * caps, gpointer udata)
{
  guint i;

  g_return_if_fail (caps != NULL);

  if (gst_caps_is_any (caps)) {
    g_critical ("ANY\n");
    return;
  }
  if (gst_caps_is_empty (caps)) {
    g_critical ("EMPTY\n");
    return;
  }

  for (i = 0; i < gst_caps_get_size (caps); i++) {
    GstStructure *structure = gst_caps_get_structure (caps, i);
    g_critical ("%s\n", gst_structure_get_name (structure));
  }
}
#endif

static void _on_element_added (GstBin *p0, GstBin *p1, GstElement *e, gpointer data)
{
    gchar *name = gst_element_get_name (e);
#if defined (DEBUG_GST_INSPECTOR)
    g_critical ("Element inspector: %s", name);
#endif
    if (g_str_has_prefix (name, "siddecfp") == TRUE) {
        _siddec = e;
        g_object_set (G_OBJECT (e),
            "basic", sid_basic (),
            "kernal", sid_kernal (),
            "chargen", sid_chargen (),
            NULL);
        _is_siddecfp = TRUE;
    } else if (g_str_has_prefix (name, "siddec") == TRUE) {
        _siddec = e;
        _is_siddecfp = FALSE;
#if defined (DEBUG_GST_INSPECTOR)
    } if (g_str_has_prefix (name, "typefind") == TRUE) {
        g_signal_connect (e, "have-type", G_CALLBACK (_on_have_type), NULL);
#endif
    }
    g_free (name);
}

