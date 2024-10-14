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

#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <libintl.h>
#define _(String) gettext (String)

#include "playlist.h"
#include "playlist-pls.h"
#include "playlist-m3u.h"
#include "util.h"
#include "inspector.h"
#include "playlist-line.h"
#include "ncurses-common.h"

static GList **_list = NULL;
static GList *_playlist = NULL;
static GList *_current = NULL;
static PlaylistMode _mode = PLAYLIST_MODE_STANDARD;
static GList *_sufflelist = NULL;
static gboolean _loop = FALSE;
static gint _search_index = -1;
static GList *_pastelist = NULL;
static gboolean _with_tags = FALSE;

#define ABSOLUTELY_MAX_STR_LEN 4096

static void _pastelist_free (void);
static void _sufflelist_free (void);
static void _free_song_list_items (gpointer data, gpointer user_data);
static gint _playlist_random (void);
static void _generate_sufflelist (void);
static gint _search_from_index (gint start_index, gboolean backwards);
static gboolean _search_test_line (const gchar *line);
static gboolean _search_is_match (Song *o);
static gboolean _add_list (GList *l);
static gboolean _add_playlist_file (const char *filepath);

static gint _length = 0;
static gboolean _search_use_case_sensitive = FALSE;

static SearchType _search;
static SearchMatchType _search_match;

void playlist_init (void)
{
    srand (time (NULL));
    _list = &_playlist;
    (void)search_init (&_search);
}

GList *playlist_get (void)
{
    return _playlist;
}

static void _pastelist_free (void)
{
    if (_pastelist != NULL) {
        g_list_foreach (_pastelist, _free_song_list_items, NULL);
        g_list_free (_pastelist);
        _pastelist = NULL;
    }
}

static void _sufflelist_free (void)
{
    if (_sufflelist != NULL) {
        g_list_foreach (_sufflelist, _free_song_list_items, NULL);
        g_list_free (_sufflelist);
        _sufflelist = NULL;
    }
}

void playlist_free (void)
{
    _pastelist_free ();
    _sufflelist_free ();
    if (_playlist != NULL) {
        g_list_foreach (_playlist, _free_song_list_items, NULL);
        g_list_free (_playlist);
    }
    search_free (&_search);
}

gboolean playlist_add (gchar *path)
{
    if (path == NULL) return FALSE;
    GList *l = inspector_run (path);
    gboolean ret = TRUE;
    if (l != NULL) {
        if (_add_list (l) == FALSE) {
            ret = FALSE;
        }
    } else if (_add_playlist_file (path) == FALSE){
        ret = FALSE; /* fail if can not add */
    }

    if (ret == TRUE && _search.current != NULL) {
        (void)_search_from_index (_search_index, FALSE);
    }

    return ret;
}


gboolean playlist_mode_set (PlaylistMode mode)
{
    _mode = mode;
    if (mode == PLAYLIST_MODE_SUFFLE) {
        _generate_sufflelist ();
        _list = &_sufflelist;
    } else {
        _list = &_playlist;
    }
    _length = g_list_length (*_list);
    _current = *_list;
    return TRUE;
}

gboolean playlist_mode_next (void)
{
    PlaylistMode mode = _mode + 1;
    if (mode > PLAYLIST_MODE_RANDOM) mode = PLAYLIST_MODE_STANDARD;
    playlist_mode_set (mode);
    return TRUE;
}


PlaylistMode playlist_mode (void)
{
    return _mode;
}

const gchar *playlist_mode_to_string (void)
{
    if (_mode == PLAYLIST_MODE_STANDARD) return "";
    else if (_mode == PLAYLIST_MODE_SUFFLE) return _("Suffle");
    else if (_mode == PLAYLIST_MODE_RANDOM) return _("Random");
    return _("Error");
}

void playlist_loop_set (gboolean loop)
{
    _loop = loop;
}

gboolean playlist_loop (void)
{
    return _loop;
}

gboolean playlist_loop_toggle (void)
{
    if (_loop == FALSE) _loop = TRUE;
    else _loop = FALSE;
    return _loop;
}

Song *playlist_remove_list (GSList *remove_list)
{
    GList *current = _current;

    for (GSList *l0 = remove_list; l0 != NULL; l0 = l0->next) {
        GList *l = g_list_find (*_list, l0->data);
        if (l != NULL) {
            Song *s = (Song *)l->data;
            if (current != NULL && s == (Song *)current->data) { /* change _current song if removed */
                GList *new_cur = current->next;
                if (new_cur == NULL) {
                    new_cur = current->prev;
                }
                current = new_cur;
            }
            *_list = g_list_remove (*_list, s);
            if (_list == &_playlist && s != NULL) song_delete (s); /* remove songs only from real playlist */
        }
    }

    _current = current;
    _length = g_list_length (*_list);
    if (_current == NULL) return NULL;
    return _current->data;
}


Song *playlist_get_first_song (void)
{
    if (*_list == NULL) return NULL;
    _current = *_list;
    return (Song *)_current->data;
}

Song *playlist_get_current_song (void)
{
    if (_current == NULL) return NULL;
    return (Song *)_current->data;
}

Song *playlist_get_next_song (void)
{
    GList *list = _mode==PLAYLIST_MODE_SUFFLE ? _sufflelist : _playlist;
    if (_current == NULL) return NULL;
    if (_mode == PLAYLIST_MODE_STANDARD || PLAYLIST_MODE_SUFFLE) {
        _current = _current->next;
        if (_loop == TRUE && _current == NULL) {
            _current = list;
        }
        if (_current == NULL) return NULL;
    } else if (_mode == PLAYLIST_MODE_RANDOM) {
        gint index = _playlist_random ();
        return  playlist_get_nth_song (index);
    }
    return (Song *)_current->data;
}

Song *playlist_get_nth_song_no_set (gint index)
{
    if (*_list == NULL) return NULL;
    return (Song *)g_list_nth_data (*_list, index);
}

Song *playlist_get_nth_song (gint index)
{
    if (*_list == NULL) return NULL;
    _current = g_list_nth (*_list, index);
    if (_current == NULL) return NULL;
    return (Song *)_current->data;
}

gint playlist_get_song_index (Song *o)
{
    if (*_list == NULL) return -1;
    return g_list_index (*_list, o);
}

gint playlist_length (void)
{
    if (*_list == NULL) return 0;
    return _length;
}

gint playlist_reorder (PlaylistOrder o)
{
    /* FIXME: !!!!!!! */
    (void)o;
    return 0;
}

gint playlist_search_set (const gchar *search, gint start_index, gboolean backwards, gboolean with_tags)
{
    _with_tags = with_tags;
    if (search_set (&_search, search) != 0) {
        return -100;
    }
    if (_search.current == NULL) return 0;
    return _search_from_index (start_index, backwards);
}

static gint _search_from_index (gint start_index, gboolean backwards)
{
    gint i;
    gint len = playlist_length ();
    gint ret = -1;
    if (_search.current == NULL) return -1;
    /* first part */
    if (backwards == TRUE) {
        if (start_index < 0) start_index = len - 1;
    } else {
        if (start_index > len - 1) start_index = 0;
    }
    i = start_index;
    while (1) {
        if (_search_is_match (playlist_get_nth_song_no_set (i)) == TRUE) {
            if (ret < 0) ret = i;
        }
        if (backwards == TRUE) {
            i--;
            if (i < 0) break;
        } else {
            i++;
            if (i > len - 1) break;
        }
    }
    /* second part */
    if (backwards == TRUE) {
        i = len - 1;
    } else {
        i = 0;
    }
    while (1) {
        if (_search_is_match (playlist_get_nth_song_no_set (i)) == TRUE) {
            if (ret < 0) ret = i;
        }
        if (backwards == TRUE) {
            i--;
            if (i < start_index + 1) break;
        } else {
            i++;
            if (i > start_index - 1) break;
        }
    }

    _search_index = ret;

    return ret;
}

gint playlist_search_next (void)
{
    gint ret;
    if (_search.current == NULL) return -1;
    ret = _search_from_index (++_search_index, FALSE);
    return ret;
}

gint playlist_search_prev (void)
{
    gint ret;
    if (_search.current == NULL) return -1;
    ret = _search_from_index (--_search_index, TRUE);
    return ret;
}

void playlist_search_index_set (gint index)
{
    _search_index = index;
}

const gchar *playlist_search_string (void)
{
    return _search.current;
}

gboolean playlist_search_with_tags (void)
{
    return _with_tags;
}

gboolean playlist_search_use_case_sensitive (void)
{
    return _search_use_case_sensitive;
}

void playlist_search_free (void)
{

    gint i, len = playlist_length ();
    Song *o;
    search_free (&_search);
    for (i = 0; i < len; i++) {
        o = playlist_get_nth_song_no_set (i);
        if (o != NULL) o->search_hit = -1;
    }
}


gboolean playlist_toggle_select_range (gint first_index, gint second_index)
{
    gint i;
    gint min = MIN (first_index, second_index);
    gint max = (min==first_index)?second_index:first_index;
    gint last_index = playlist_length () - 1;

    if (last_index < 0) return FALSE;
    if (max > last_index) max = last_index;
    if (min < 0) min = 0;

    for (i = max; i > min - 1; i--) {
        GList *l = g_list_nth (*_list, i);
        if (l != NULL) {
            Song *s = (Song *)l->data;
            if (s == NULL) continue;
            s->selected = s->selected==TRUE?FALSE:TRUE;
        }
    }
    return TRUE;
}

gboolean playlist_copy_range (gint first_index, gint second_index)
{
    gint i;
    gint min = MIN (first_index, second_index);
    gint max = (min==first_index)?second_index:first_index;
    gint last_index = playlist_length () - 1;

    if (last_index < 0) return FALSE;
    if (max > last_index) max = last_index;
    if (min < 0) min = 0;
    _pastelist_free ();

    for (i = max; i > min - 1; i--) {
        Song *s = g_list_nth_data (*_list, i);
        if (s == NULL) continue;
        Song *s_new = song_clone (s);
        if (s_new == NULL) continue;
        _pastelist = g_list_prepend (_pastelist, s_new);
    }
    return TRUE;
}

gboolean playlist_cut_range (gint first_index, gint second_index)
{
    gint i;
    gint min = MIN (first_index, second_index);
    gint max = (min==first_index)?second_index:first_index;
    gint last_index = playlist_length () - 1;

    if (last_index < 0) return FALSE;
    if (max > last_index) max = last_index;
    if (min < 0) min = 0;
    _pastelist_free ();

    for (i = max; i > min - 1; i--) {
        GList *l = g_list_nth (*_list, i);
        if (l == NULL) continue;
        Song *s = (Song *)l->data;
        if (s == NULL) continue;
        *_list = g_list_remove_link (*_list, l);
        _pastelist = g_list_prepend (_pastelist, s);
    }
    _length = g_list_length (*_list);
    return TRUE;
}

gboolean playlist_copy_selected ()
{
    gint i;
    gint max = playlist_length () - 1;

    if (max < 0) return FALSE;
    _pastelist_free ();

    for (i = max; i > -1; i--) {
        Song *s = g_list_nth_data (*_list, i);
        if (s == NULL) continue;
        if (s->selected == TRUE) {
            Song *s_new = song_clone (s);
            if (s_new == NULL) continue;
            _pastelist = g_list_prepend (_pastelist, s_new);
        }
    }
    return TRUE;
}

gboolean playlist_cut_selected ()
{
    gint i;
    gint max = playlist_length () - 1;

    if (max < 0) return FALSE;
    _pastelist_free ();

    for (i = max; i > -1; i--) {
        GList *l = g_list_nth (*_list, i);
        if (l == NULL) continue;
        Song *s = (Song *)l->data;
        if (s == NULL) continue;
        if (s->selected == TRUE) {
            *_list = g_list_remove_link (*_list, l);
            _pastelist = g_list_prepend (_pastelist, s);
        }
    }
    _length = g_list_length (*_list);
    return TRUE;
}

gboolean playlist_paste_to (gint index)
{
    gint i;
    gint last_index = -1;
    if (_pastelist == NULL) return FALSE;
    last_index = g_list_length (_pastelist) - 1;

    for (i = last_index; i > -1; i--) {
        Song *s = (Song *)g_list_nth_data (_pastelist, i);
        if (s == NULL) continue;
        Song *s_new = song_clone (s);
        if (s_new == NULL) continue;
        *_list = g_list_insert (*_list, s_new, index);
    }
    _length = g_list_length (*_list);
    return TRUE;
}

gint playlist_num_to_paste (void)
{
    if (_pastelist == NULL) {
        return 0;
    }
    return g_list_length (_pastelist);
}

static void _free_song_list_items (gpointer data, gpointer user_data)
{
    if (data == NULL) return;
    Song *s = (Song *)data;
    song_delete (s);
}

/* reorder stuff */
gint song_sort_by_path (gconstpointer p1, gconstpointer p2)
{
  const Song *s1 = (const Song *)p1;
  const Song *s2 = (const Song *)p2;

  if (s2 == NULL) return -1;
  else if (s1 == NULL) return 1;
  else if (s2->uri == NULL) return -1;
  else if (s1->uri == NULL) return 1;

  return g_ascii_strncasecmp (s1->uri, s2->uri, ABSOLUTELY_MAX_STR_LEN);
}

static gboolean _add_playlist_file (const char *filepath)
{
    GList *l;
    if (filepath == NULL) return FALSE;
    if (util_is_possibly_supported_file (filepath) == FALSE) return FALSE;

    l = playlist_pls_load (filepath);
    if (l == NULL) l = playlist_m3u_load (filepath);

    if (l != NULL) {
        if (_add_list (l) == FALSE) {
            return FALSE;
        }
    }
    return TRUE;
}

static gboolean _add_list (GList *l)
{
    if (l == NULL) return FALSE;
    if (playlist_length () + g_list_length (l) > INT_MAX) return FALSE;
    if (_playlist == NULL) _playlist = l;
    else _playlist = g_list_concat (_playlist, l);
    _length = g_list_length (_playlist);
    if (_mode == PLAYLIST_MODE_SUFFLE) {
        _generate_sufflelist ();
    }
    return TRUE;
}

static gint _playlist_random ()
{
     return ((rand()+1)%(playlist_length ()));
}

static void _generate_sufflelist (void)
{
    GList *o = NULL;
    gint pos0 = 0;
    gint len = playlist_length ();

    _sufflelist_free ();
    if (len == 0) return;

    o = g_list_last (_playlist);
    while (o != NULL) {
        Song *s = song_clone (o->data);
        _sufflelist = g_list_prepend (_sufflelist, s);
        o = g_list_previous (o);
    }
    for (gint i = 0; i < len*2; i++) {
        pos0 = (rand() + 1) % len;
        o = g_list_nth (_sufflelist, pos0);
        _sufflelist = g_list_remove_link (_sufflelist, o);
        _sufflelist = g_list_concat (o, _sufflelist);
    }
}

gboolean playlist_search_get_search_match (Song *o, SearchMatchType *sm)
{
    if (o == NULL) return FALSE;
    else if (sm == NULL) return FALSE;
    sm->num_matches = 0;
    if (_search.current == NULL) return FALSE;
    if (_search_is_match (o) == FALSE) return FALSE;
    memcpy (sm, &_search_match, sizeof (SearchMatchType));
    return TRUE;
}

static gboolean _search_test_line (const gchar *line)
{
    _search_match.num_matches = 0;
    if (search_match (&_search, line, &_search_match) != 0) {
        return FALSE;
    }
    if (_search_match.num_matches > 0) {
        return TRUE;
    }
    return FALSE;
}

static gboolean _search_is_match (Song *o)
{
    static gchar tmp[ABSOLUTELY_MAX_LINE_LEN];
    if (o == NULL) return FALSE;

    o->search_hit = 0;
    if (playlist_line_create (tmp, ABSOLUTELY_MAX_LINE_LEN-1, o) == FALSE)
    {
        o->search_hit = -1;
	return FALSE;
    }
    if (_with_tags) {
	gboolean ret = FALSE;
        if (_search_test_line (o->basename) == TRUE) {
            ret = TRUE;
        } else if (_search_test_line (o->title) == TRUE) {
            ret = TRUE;
        } else if (_search_test_line (o->stream_title) == TRUE) {
            ret = TRUE;
        } else if (_search_test_line (o->artist) == TRUE) {
            ret = TRUE;
        } else if (_search_test_line (o->album) == TRUE) {
            ret = TRUE;
        } else if (_search_test_line (o->uri) == TRUE) {
            ret = TRUE;
        }
	if (ret == TRUE) {
            /* fake whole line */
            _search_match.num_matches = 1;
            _search_match.results[0].start = 0;
	    _search_match.results[0].end = strlen (tmp);
            return TRUE;
	}
        
    } else {
        if (_search_test_line (tmp) == TRUE) {
            return TRUE;
        }
    }
    o->search_hit = -1;
    return FALSE;
}
