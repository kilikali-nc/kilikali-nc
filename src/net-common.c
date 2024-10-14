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

#include "net-common.h"

#include <curl/curl.h>

size_t net_common_curl_write_data (void *contents, size_t size, size_t nmemb, void *user_data)
{
    NetCommonDownload *dl = (NetCommonDownload *)user_data;
    size_t total = size * nmemb;
    dl->data = g_realloc (dl->data, dl->size + total);
    memcpy (&dl->data[dl->size], contents, total);
    dl->size += total;
    return size * nmemb;
}

