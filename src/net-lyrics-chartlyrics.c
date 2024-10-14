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

#include <curl/curl.h>

#include "net-lyrics.h"
#include "net-common.h"
#include "util.h"

gchar *_get_chartlyrics (const gchar *url, const gchar *a, const gchar *t, gboolean use_and);

gchar *_get_chartlyrics (const gchar *url, const gchar *a, const gchar *t, gboolean use_and)
{
    GString *artist = NULL;
    GString *title = NULL;
    NetCommonDownload dl = {0,};
    gchar *aurl = NULL;
    char *enca = NULL;
    char *enct = NULL;
    CURLcode res;
    CURL *curl = curl_easy_init();
    if (curl == NULL) return NULL;

    enca = curl_easy_escape(curl, a, strlen (a));
    if (enca == NULL) goto _get_chartlyrics_error;
    enct = curl_easy_escape(curl, t, strlen (t));
    if (enct == NULL) goto _get_chartlyrics_error;

    artist = g_string_new (enca);
    if (artist == NULL) goto _get_chartlyrics_error;
    title = g_string_new (enct);
    if (title == NULL) goto _get_chartlyrics_error;

    /* This does not work with every search. ...and some search does not work with this. */
    if (TRUE == use_and) {
        util_g_string_replace (artist, "%20", "&&", 0);
        util_g_string_replace (title, "%20", "&&", 0);
    }
    aurl = g_strdup_printf (url, artist->str, title->str);
    if (aurl == NULL) goto _get_chartlyrics_error;

    curl_easy_setopt (curl, CURLOPT_URL, aurl);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, net_common_curl_write_data);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &dl);
    res = curl_easy_perform (curl);

_get_chartlyrics_error:
    if (artist != NULL) g_string_free (artist, TRUE);
    if (title != NULL) g_string_free (title, TRUE);
    if (enca != NULL) {
        curl_free (enca);
    }
    if (enct != NULL) {
        curl_free (enct);
    }
    g_free (aurl);
    curl_easy_cleanup (curl);

    dl.data = g_realloc (dl.data, dl.size + 1);
    dl.data[dl.size] = '\0';

    (void)res;
    return dl.data;
}

gpointer net_lyrics_chartlyrics_thread (gpointer user_data)
{
    gchar *result = NULL;
    gchar *lyrics = NULL;
    gboolean found = FALSE;
    if (user_data == NULL) return user_data;
    NetLyricsThreadData *data = (NetLyricsThreadData *)user_data;

    /* and-search */
    result = _get_chartlyrics ("http://api.chartlyrics.com/apiv1.asmx/SearchLyricDirect?artist=%s&song=%s", data->artist, data->title, TRUE);
    if (result != NULL && *result != '\0' ) {
        /* if and-search does not work result is some weird string */
        gchar *str = result;
        gboolean found = FALSE;
        do {
            if (!strncmp(str, "<Lyric>", 7)) {
                found = TRUE;
                break;
            }
        } while (*str != '\0' && ((str = g_utf8_find_next_char (str, NULL)) != NULL));
        if (found == FALSE) {
            g_free (result);
            /* or-search */
            result = _get_chartlyrics ("http://api.chartlyrics.com/apiv1.asmx/SearchLyricDirect?artist=%s&song=%s", data->artist, data->title, FALSE);
        }
    } else if (result == NULL || *result == '\0' ) {
        g_free (result);
        /* or-search */
        result = _get_chartlyrics ("http://api.chartlyrics.com/apiv1.asmx/SearchLyricDirect?artist=%s&song=%s", data->artist, data->title, FALSE);
    }
    if (result == NULL || *result == '\0' ) {
        goto _thread_chartlyrics_error;
    }

    gchar *str = result;
    do {
        if (*str == '\r') {
            *str = ' ';
        } else if (!strncmp(str, "<Lyric>", 7)) {
            str += 7;
            lyrics = str;
        } else if (!strncmp(str, "</Lyric>", 8)) {
            memset(str, 0, 8);
            found = TRUE;
            break;
        }
    } while (*str != '\0' && ((str = g_utf8_find_next_char (str, NULL)) != NULL));
    if (found == FALSE) lyrics = NULL;

    if (lyrics != NULL && *lyrics != '\0') {
        for (size_t i = strlen (lyrics) - 1; i >= 0; i--) {
            if (lyrics[i] == ' ' || lyrics[i] == '\n') {
                lyrics[i] = '\0';
            } else {
                break;
            }
        }
    }

    g_idle_add (data->cb, (gpointer)g_strdup(lyrics));

_thread_chartlyrics_error:
    g_free (result);
    return user_data;
}
