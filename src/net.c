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

#include "net.h"
#include "net-common.h"
#include "net-lyrics.h"

#include <curl/curl.h>

static GThread *_thread = NULL;

void net_free (void)
{
    NetLyricsThreadData *data = NULL;
    if (_thread != NULL) {
        data = (NetLyricsThreadData *)g_thread_join (_thread);
        net_lyrics_free_data (data);
        _thread = NULL;
    }
}

gchar *net_get (const gchar *url)
{
    NetCommonDownload dl = {0,};
    CURLcode res;
    CURL *curl = curl_easy_init();
    if (curl == NULL) return NULL;

    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, net_common_curl_write_data);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, &dl);

    /* not very secure, but probably makes life again easier */
    curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0L);
    res = curl_easy_perform (curl);

    curl_easy_cleanup (curl);

    dl.data = g_realloc (dl.data, dl.size + 1);
    dl.data[dl.size] = '\0';

    (void)res;
    return dl.data;
}


gint net_lyrics_get (const gchar *artist, const gchar *title, NetLyricsService service, GSourceFunc cb)
{
    NetLyricsThreadData *data = NULL;
    gint ret = 0;
    if (cb == NULL) {
        return 1;
    }

    /* Only one thread at the time */
    if (_thread != NULL) {
        data = (NetLyricsThreadData *)g_thread_join (_thread);
        net_lyrics_free_data (data);
        _thread = NULL;
    }

    switch (service) {
        case NET_LYRICS_SERVICE_CHARTLYRICS:
        {
            data = g_new0 (NetLyricsThreadData, 1);
            if (data == NULL) goto lyrics_error;
            data->artist = g_strdup (artist);
            if (data->artist == NULL) goto lyrics_error;
            data->title = g_strdup (title);
            if (data->title == NULL) goto lyrics_error;
            data->cb = cb;
            _thread = g_thread_new ("ChartLyricsThread", net_lyrics_chartlyrics_thread, (gpointer)data);
            if (_thread == NULL) goto lyrics_error;
        }
        break;
        case NET_LYRICS_SERVICE_NONE:
        default:
        {
            return 2;
        }
        break;
    }
    return ret;
lyrics_error:
    net_lyrics_free_data (data);
    return 100;
}

